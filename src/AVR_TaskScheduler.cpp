// AVR_TaskScheduler.cpp
#include "AVR_TaskScheduler.h"

// Global scheduler state
xAVRSchedulerState xAVRScheduler = {NULL, NULL, false, 0, {0, 0, 0, UINT32_MAX, 0}};
static float CPUUsage = 0.0;
static const uint16_t STD_TICKRATE = 1000;
static xAVRTaskHandle xIDLEHandle;

// Internal function declarations
static void prvAddTaskToList(xAVRTaskHandle *pxNewTask);
static void prvRemoveTaskFromList(xAVRTaskHandle *pxTaskToRemove);
static bool prvShouldTaskRun(xAVRTaskHandle *pxTask);
static void prvUpdateSchedulerStats(uint32_t ulSchedulerTime);

/// @brief Check if millisecond time has elapsed
/// @param timeValue Time value to check
/// @param startTime Start time
/// @return true if time has elapsed, false otherwise
bool AVRDeltaMlsCheck(uint32_t timeValue, uint32_t startTime) {
    if (startTime == 0) return false;
    uint32_t currentTime = ulAVRTaskGetTickCount();
    // Handle millisecond counter overflow
    if (currentTime < startTime) {
        return (UINT32_MAX - startTime + currentTime >= timeValue);
    }
    return (currentTime - startTime >= timeValue);
}

/// @brief Get elapsed milliseconds
/// @param startTime Start time
/// @return Elapsed time in milliseconds
uint32_t ulAVRDeltaMlsGet(uint32_t startTime) {
    if (startTime == 0) return 0;
    uint32_t currentTime = ulAVRTaskGetTickCount();
    if (currentTime < startTime) {
        return UINT32_MAX - startTime + currentTime;
    }
    return currentTime - startTime;
}

/// @brief Check if microsecond time has elapsed
/// @param timeValue Time value to check
/// @param startTime Start time
/// @return true if time has elapsed, false otherwise
bool AVRDeltaMicroCheck(uint32_t timeValue, uint32_t startTime) {
    if (startTime == 0) return false;
    uint32_t currentTime = micros();
    if (currentTime < startTime) {
        return (UINT32_MAX - startTime + currentTime >= timeValue);
    }
    return (currentTime - startTime >= timeValue);
}

/// @brief Get elapsed microseconds
/// @param startTime Start time
/// @return Elapsed time in microseconds
uint32_t ulAVRDeltaMicroGet(uint32_t startTime) {
    if (startTime == 0) return 0;
    uint32_t currentTime = micros();
    if (currentTime < startTime) {
        return UINT32_MAX - startTime + currentTime;
    }
    return currentTime - startTime;
}

/// @brief Get CPU usage
/// @return CPU usage percentage
float xGetCPUUsage(void) {
	return CPUUsage;
}

/// @brief Create a task
/// @param pvTaskCode Task function pointer
/// @param pcTaskName Task name
/// @param pvParameters Task parameters
/// @param uxPriority Task priority
/// @param pxCreatedTask Pointer to store created task handle
/// @param ulInterval Task execution interval in milliseconds
/// @return true if task was created, false otherwise
bool vAVRTaskCreate(TaskFunction pvTaskCode, 
                   const char *pcTaskName,
                   void *pvParameters,
                   uint8_t uxPriority,
                   xAVRTaskHandle *pxCreatedTask,
                   uint32_t ulInterval) {

    if (pvTaskCode == NULL || pxCreatedTask == NULL) {
        return false;
    }

    // Initialize task control block
    pxCreatedTask->pvTaskCode = pvTaskCode;
    pxCreatedTask->pvParameters = pvParameters;
    pxCreatedTask->ulInterval = ulInterval;
    pxCreatedTask->ulLastExecutionTime = 0; // Initially 0, waiting for reset
    pxCreatedTask->ulExecutionCount = 0;
    pxCreatedTask->eCurrentState = AVR_TASK_SUSPENDED; // Initially suspended
    pxCreatedTask->ucPriority = uxPriority;
    pxCreatedTask->pcTaskName = pcTaskName;
    pxCreatedTask->pxNextTask = NULL;

    // Initialize delay information
    pxCreatedTask->xDelayInfo.ulDelayUntil = 0;
    pxCreatedTask->xDelayInfo.bIsDelaying = false;

    // Add task to scheduler list
    prvAddTaskToList(pxCreatedTask);

    return true;
}

/// @brief Delete a task
/// @param pxTaskToDelete Pointer to the task handle to delete
void vAVRTaskDelete(xAVRTaskHandle *pxTaskToDelete) {
    if (pxTaskToDelete == NULL) return;

    if (pxTaskToDelete->eCurrentState == AVR_TASK_RUNNING) {
        pxTaskToDelete->eCurrentState = AVR_TASK_DELETED;
    } else {
        prvRemoveTaskFromList(pxTaskToDelete);
    }
}

/// @brief Suspend a task
/// @param pxTaskToSuspend Pointer to the task handle to suspend
void vAVRTaskSuspend(xAVRTaskHandle *pxTaskToSuspend) {
    if (pxTaskToSuspend != NULL) {
        pxTaskToSuspend->eCurrentState = AVR_TASK_SUSPENDED;
    }
}

/// @brief Resume a task
/// @param pxTaskToResume Pointer to the task handle to resume
void vAVRTaskResume(xAVRTaskHandle *pxTaskToResume) {
    if (pxTaskToResume != NULL) {
        pxTaskToResume->eCurrentState = AVR_TASK_READY;
        pxTaskToResume->ulLastExecutionTime = ulAVRTaskGetTickCount();
        // Reset delay state
        pxTaskToResume->xDelayInfo.bIsDelaying = false;
    }
}

/// @brief Get current tick count
/// @return Current tick count in milliseconds
uint32_t ulAVRTaskGetTickCount() {
	return millis();
}

/// @brief Delay a task
/// @param ulDelayTime Delay time in milliseconds
void vAVRTaskDelay(uint32_t ulDelayTime) {
    if (ulDelayTime == 0) return;

    xAVRTaskHandle *pxCurrentTask = pxAVRGetCurrentTaskHandle();
    if (pxCurrentTask != NULL) {
        pxCurrentTask->xDelayInfo.ulDelayUntil = ulAVRTaskGetTickCount() + ulDelayTime;
        pxCurrentTask->xDelayInfo.bIsDelaying = true;

        // Update statistics
        xAVRScheduler.xStats.ulTotalDelayTime += ulDelayTime;

        #ifdef AVR_TASKSCHEDULER_DEBUG
        Serial.print("Task ");
        Serial.print(pxCurrentTask->pcTaskName);
        Serial.print(" delaying for ");
        Serial.print(ulDelayTime);
        Serial.println(" ms");
        #endif
    }
}

/// @brief Delay a task until a specific time
/// @param pxLastWakeTime Pointer to last wake time
/// @param xFrequency Frequency in milliseconds
void vAVRTaskDelayUntil(uint32_t *pxLastWakeTime, uint32_t xFrequency) {
    if (pxLastWakeTime == NULL || xFrequency == 0) return;

    uint32_t ulCurrentTime = ulAVRTaskGetTickCount();

    // Calculate next wake time
    uint32_t ulNextWakeTime = *pxLastWakeTime + xFrequency;

    // If next wake time has passed (possibly due to long task execution), adjust to next cycle
    if (ulCurrentTime > ulNextWakeTime) {
        ulNextWakeTime = ulCurrentTime + xFrequency;
    }

    xAVRTaskHandle *pxCurrentTask = pxAVRGetCurrentTaskHandle();
    if (pxCurrentTask != NULL) {
        pxCurrentTask->xDelayInfo.ulDelayUntil = ulNextWakeTime;
        pxCurrentTask->xDelayInfo.bIsDelaying = true;

        uint32_t ulDelayTime = ulNextWakeTime - ulCurrentTime;
        xAVRScheduler.xStats.ulTotalDelayTime += ulDelayTime;

        #ifdef AVR_TASKSCHEDULER_DEBUG
        Serial.print("Task ");
        Serial.print(pxCurrentTask->pcTaskName);
        Serial.print(" delaying until ");
        Serial.print(ulNextWakeTime);
        Serial.print(" (");
        Serial.print(ulDelayTime);
        Serial.println(" ms from now)");
        #endif
    }

    // Update last wake time
    *pxLastWakeTime = ulNextWakeTime;
}

/// @brief Check if a task is delaying
/// @param pxTask Pointer to the task handle
/// @return true if task is delaying, false otherwise
bool bAVRIsTaskDelaying(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return false;
    return pxTask->xDelayInfo.bIsDelaying;
}

/// @brief Get remaining delay time for a task
/// @param pxTask Pointer to the task handle
/// @return Remaining delay time in milliseconds
uint32_t ulAVRGetTaskRemainingDelay(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL || !pxTask->xDelayInfo.bIsDelaying) {
        return 0;
    }

    uint32_t ulCurrentTime = ulAVRTaskGetTickCount();
    if (ulCurrentTime >= pxTask->xDelayInfo.ulDelayUntil) {
        return 0;
    }

    return pxTask->xDelayInfo.ulDelayUntil - ulCurrentTime;
}

/// @brief Reset all task timers
void vAVRResetAllTaskTimers(void) {
    xAVRTaskHandle *pxTask = xAVRScheduler.pxFirstTask;
    uint32_t ulCurrentTime = ulAVRTaskGetTickCount();

    while (pxTask != NULL) {
        pxTask->ulLastExecutionTime = ulCurrentTime;
        pxTask->eCurrentState = AVR_TASK_READY;
        // Reset delay state
        pxTask->xDelayInfo.bIsDelaying = false;
        pxTask = pxTask->pxNextTask;
    }
}

/// @brief Reset a specific task timer
/// @param pxTask Pointer to the task handle
void vAVRResetTaskTimer(xAVRTaskHandle *pxTask) {
    if (pxTask != NULL) {
        pxTask->ulLastExecutionTime = ulAVRTaskGetTickCount();
        pxTask->eCurrentState = AVR_TASK_READY;
        pxTask->xDelayInfo.bIsDelaying = false;
    }
}

/// @brief Start the scheduler (blocking)
void vAVRTaskStartScheduler(void) {
    vAVRResetAllTaskTimers(); // Reset all task times
    xAVRScheduler.bSchedulerRunning = true;
    xAVRScheduler.ulSchedulerStartTime = ulAVRTaskGetTickCount();
	vAVRTaskCreate(IDLETask, "IDLE TASK", NULL, 1, &xIDLEHandle);

    while (xAVRScheduler.bSchedulerRunning) {
        vAVRTaskSchedule();
    }
}

/// @brief Start the scheduler (non-blocking)
void vAVRTaskStartSchedulerNonBlocking(void) {
    vAVRResetAllTaskTimers(); // Reset all task times
    xAVRScheduler.bSchedulerRunning = true;
    xAVRScheduler.ulSchedulerStartTime = ulAVRTaskGetTickCount();
	vAVRTaskCreate(IDLETask, "IDLE TASK", NULL, 1, &xIDLEHandle);
}

/// @brief End the scheduler
void vAVRTaskEndScheduler(void) {
    xAVRScheduler.bSchedulerRunning = false;
}

/// @brief Schedule tasks
void vAVRTaskSchedule(void) {
    if (!xAVRScheduler.bSchedulerRunning) return;

    uint32_t ulStartTime = micros();
    xAVRTaskHandle *pxTask = xAVRScheduler.pxFirstTask;

    while (pxTask != NULL) {
        // Check if task should run
        if (prvShouldTaskRun(pxTask)) {
            xAVRScheduler.pxCurrentTask = pxTask;
            pxTask->eCurrentState = AVR_TASK_RUNNING;
            pxTask->ulLastExecutionTime = ulAVRTaskGetTickCount();

            // Execute task function
            if (pxTask->pvTaskCode != NULL) {
                pxTask->pvTaskCode(pxTask->pvParameters);
            }

            pxTask->ulExecutionCount++;
            pxTask->eCurrentState = AVR_TASK_READY;

            // Check if task was marked for deletion
            if (pxTask->eCurrentState == AVR_TASK_DELETED) {
                xAVRTaskHandle *pxTaskToDelete = pxTask;
                pxTask = pxTask->pxNextTask;
                prvRemoveTaskFromList(pxTaskToDelete);
                continue;
            }
        }

        pxTask = pxTask->pxNextTask;
    }

    xAVRScheduler.pxCurrentTask = NULL;
    xAVRScheduler.xStats.ulSchedulerCycles++;

    // Update statistics
    uint32_t ulSchedulerTime = micros() - ulStartTime;
    prvUpdateSchedulerStats(ulSchedulerTime);
}

/// @brief Get current task handle
/// @return Pointer to the current task handle
xAVRTaskHandle *pxAVRGetCurrentTaskHandle(void) {
    return xAVRScheduler.pxCurrentTask;
}

/// @brief Get task state
/// @param pxTask Pointer to the task handle
/// @return Task state
eAVRTaskState eAVRGetTaskState(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return AVR_TASK_DELETED;
    return pxTask->eCurrentState;
}

/// @brief Get task name
/// @param pxTask Pointer to the task handle
/// @return Pointer to the task name string
const char* pcAVRGetTaskName(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return "Invalid Task";
    return pxTask->pcTaskName;
}

/// @brief Get task execution count
/// @param pxTask Pointer to the task handle
/// @return Task execution count
uint32_t ulAVRGetTaskExecutionCount(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return 0;
    return pxTask->ulExecutionCount;
}

/// @brief Set task execution interval
/// @param pxTask Pointer to the task handle
/// @param ulNewInterval New interval in milliseconds
void vAVRSetTaskInterval(xAVRTaskHandle *pxTask, uint32_t ulNewInterval) {
    if (pxTask != NULL) {
        pxTask->ulInterval = ulNewInterval;
    }
}

/// @brief Get task execution interval
/// @param pxTask Pointer to the task handle
/// @return Task execution interval in milliseconds
uint32_t ulAVRGetTaskInterval(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return 0;
    return pxTask->ulInterval;
}

/// @brief Set task priority
/// @param pxTask Pointer to the task handle
/// @param ucNewPriority New priority value
void vAVRSetTaskPriority(xAVRTaskHandle *pxTask, uint8_t ucNewPriority) {
    if (pxTask != NULL) {
        pxTask->ucPriority = ucNewPriority;
    }
}

/// @brief Get task priority
/// @param pxTask Pointer to the task handle
/// @return Task priority value
uint8_t ucAVRGetTaskPriority(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL) return 0;
    return pxTask->ucPriority;
}

/// @brief Get number of tasks
/// @return Number of tasks
uint8_t ucAVRGetNumberOfTasks(void) {
    uint8_t ucCount = 0;
    xAVRTaskHandle *pxTask = xAVRScheduler.pxFirstTask;

    while (pxTask != NULL) {
        ucCount++;
        pxTask = pxTask->pxNextTask;
    }

    return ucCount;
}

/// @brief Get scheduler uptime
/// @return Scheduler uptime in milliseconds
uint32_t ulAVRGetSchedulerUptime(void) {
    return ulAVRDeltaMlsGet(xAVRScheduler.ulSchedulerStartTime);
}

/// @brief Get scheduler statistics
/// @return Scheduler statistics structure
xAVRSchedulerStats xAVRGetSchedulerStats(void) {
    return xAVRScheduler.xStats;
}

/// @brief Reset scheduler statistics
void vAVRResetSchedulerStats(void) {
    xAVRScheduler.xStats.ulTotalTasksExecuted = 0;
    xAVRScheduler.xStats.ulSchedulerCycles = 0;
    xAVRScheduler.xStats.ulMaxSchedulerTime = 0;
    xAVRScheduler.xStats.ulMinSchedulerTime = UINT32_MAX;
    xAVRScheduler.xStats.ulTotalDelayTime = 0;
}

// ========== Internal function implementations ==========

/// @brief Add task to list
/// @param pxNewTask Pointer to the task handle to add
static void prvAddTaskToList(xAVRTaskHandle *pxNewTask) {
    if (xAVRScheduler.pxFirstTask == NULL) {
        xAVRScheduler.pxFirstTask = pxNewTask;
    } else {
        xAVRTaskHandle *pxCurrent = xAVRScheduler.pxFirstTask;
        while (pxCurrent->pxNextTask != NULL) {
            pxCurrent = pxCurrent->pxNextTask;
        }
        pxCurrent->pxNextTask = pxNewTask;
    }
    pxNewTask->pxNextTask = NULL;
}

/// @brief Remove task from list
/// @param pxTaskToRemove Pointer to the task handle to remove
static void prvRemoveTaskFromList(xAVRTaskHandle *pxTaskToRemove) {
    if (xAVRScheduler.pxFirstTask == NULL || pxTaskToRemove == NULL) {
        return;
    }

    if (xAVRScheduler.pxFirstTask == pxTaskToRemove) {
        xAVRScheduler.pxFirstTask = pxTaskToRemove->pxNextTask;
    } else {
        xAVRTaskHandle *pxCurrent = xAVRScheduler.pxFirstTask;
        while (pxCurrent->pxNextTask != NULL && pxCurrent->pxNextTask != pxTaskToRemove) {
            pxCurrent = pxCurrent->pxNextTask;
        }

        if (pxCurrent->pxNextTask == pxTaskToRemove) {
            pxCurrent->pxNextTask = pxTaskToRemove->pxNextTask;
        }
    }

    pxTaskToRemove->pxNextTask = NULL;
    pxTaskToRemove->eCurrentState = AVR_TASK_DELETED;
}

/// @brief Check if task should run
/// @param pxTask Pointer to the task handle
/// @return true if task should run, false otherwise
static bool prvShouldTaskRun(xAVRTaskHandle *pxTask) {
    if (pxTask == NULL || pxTask->eCurrentState != AVR_TASK_READY) {
        return false;
    }

    // If time base is 0, task is not initialized and should not run
    if (pxTask->ulLastExecutionTime == 0) {
        return false;
    }

    // Check delay state
    if (pxTask->xDelayInfo.bIsDelaying) {
        uint32_t ulCurrentTime = ulAVRTaskGetTickCount();
        if (ulCurrentTime >= pxTask->xDelayInfo.ulDelayUntil) {
            // Delay completed
            pxTask->xDelayInfo.bIsDelaying = false;
            #ifdef AVR_TASKSCHEDULER_DEBUG
            Serial.print("Task ");
            Serial.print(pxTask->pcTaskName);
            Serial.println(" delay completed");
            #endif
        } else {
            // Still delaying
            return false;
        }
    }

    // Normal execution time check
    return AVRDeltaMlsCheck(pxTask->ulInterval, pxTask->ulLastExecutionTime);
}

/// @brief Update scheduler statistics
/// @param ulSchedulerTime Scheduler time in microseconds
static void prvUpdateSchedulerStats(uint32_t ulSchedulerTime) {
    xAVRSchedulerStats *pxStats = &xAVRScheduler.xStats;

    pxStats->ulTotalTasksExecuted++;

    if (ulSchedulerTime > pxStats->ulMaxSchedulerTime) {
        pxStats->ulMaxSchedulerTime = ulSchedulerTime;
    }

    if (ulSchedulerTime < pxStats->ulMinSchedulerTime) {
        pxStats->ulMinSchedulerTime = ulSchedulerTime;
    }
}

/// @brief IDLE task function
/// @param pvParameters Task parameters
void IDLETask(void* pvParameters) {
  static uint16_t TICKS;
  static uint32_t lastUpdTime = ulAVRTaskGetTickCount();
  const uint32_t STD_TICKDELAY = 1000 / STD_TICKRATE;

  if(!AVRDeltaMlsCheck(STD_TICKRATE, lastUpdTime)){
    TICKS += 1;
  }else {
    uint16_t CPUfreeTime = TICKS;
    CPUUsage = (STD_TICKRATE - CPUfreeTime) / 10;
    TICKS = 0;
    lastUpdTime = ulAVRTaskGetTickCount();
  }
  vAVRTaskDelay(STD_TICKDELAY);
}

void AVRSchedulerInit(void){
  Serial.begin(115200);
  delay(100);
  vAVRTaskStartScheduler();
}
