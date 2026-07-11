// AVR_TaskScheduler.h
#ifndef AVR_TASKSCHEDULER_H
#define AVR_TASKSCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// Version information
#define AVR_TASKSCHEDULER_VERSION_MAJOR 1
#define AVR_TASKSCHEDULER_VERSION_MINOR 1
#define AVR_TASKSCHEDULER_VERSION_PATCH 2  // Version number update

// Debug option
// #define AVR_TASKSCHEDULER_DEBUG 1

/// @brief Task state definitions
typedef enum {
    AVR_TASK_READY = 0,     ///< Task is ready to run
    AVR_TASK_RUNNING,       ///< Task is currently running
    AVR_TASK_SUSPENDED,     ///< Task is suspended
    AVR_TASK_DELETED        ///< Task has been deleted
} eAVRTaskState;

/// @brief Task callback function type
typedef void (*TaskFunction)(void *pvParameters);

/// @brief Task delay information structure
typedef struct {
    uint32_t ulDelayUntil;    ///< Time when delay ends
    bool bIsDelaying;         ///< Whether task is currently delaying
} xAVRTaskDelayInfo;

/// @brief Task control block structure
typedef struct avrTaskControlBlock {
    TaskFunction pvTaskCode;            ///< Task function pointer
    void *pvParameters;                 ///< Task parameters
    uint32_t ulInterval;                ///< Task execution interval (ms)
    uint32_t ulLastExecutionTime;       ///< Last execution time
    uint32_t ulExecutionCount;          ///< Execution count statistics
    eAVRTaskState eCurrentState;        ///< Current task state
    uint8_t ucPriority;                 ///< Task priority
    const char *pcTaskName;             ///< Task name
    xAVRTaskDelayInfo xDelayInfo;       ///< Task delay information
    struct avrTaskControlBlock *pxNextTask; ///< Next task pointer
} xAVRTaskHandle;

/// @brief Scheduler statistics structure
typedef struct {
    uint32_t ulTotalTasksExecuted;      ///< Total tasks executed
    uint32_t ulSchedulerCycles;         ///< Scheduler cycles count
    uint32_t ulMaxSchedulerTime;        ///< Maximum scheduler time
    uint32_t ulMinSchedulerTime;        ///< Minimum scheduler time
    uint32_t ulTotalDelayTime;          ///< Total delay time statistics
} xAVRSchedulerStats;

/// @brief Scheduler state structure
typedef struct {
    xAVRTaskHandle *pxFirstTask;        ///< First task in list
    xAVRTaskHandle *pxCurrentTask;      ///< Currently running task
    bool bSchedulerRunning;             ///< Scheduler running flag
    uint32_t ulSchedulerStartTime;      ///< Scheduler start time
    xAVRSchedulerStats xStats;          ///< Statistics
} xAVRSchedulerState;

typedef xAVRTaskHandle AVRTaskHandle_t;

// ========== Public API Functions ==========

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
                   uint32_t ulInterval = 0);

/// @brief Delete a task
/// @param pxTaskToDelete Pointer to the task handle to delete
void vAVRTaskDelete(xAVRTaskHandle *pxTaskToDelete);

/// @brief Suspend a task
/// @param pxTaskToSuspend Pointer to the task handle to suspend
void vAVRTaskSuspend(xAVRTaskHandle *pxTaskToSuspend);

/// @brief Resume a task
/// @param pxTaskToResume Pointer to the task handle to resume
void vAVRTaskResume(xAVRTaskHandle *pxTaskToResume);

/// @brief Get current tick count
/// @return Current tick count in milliseconds
uint32_t ulAVRTaskGetTickCount(void);

/// @brief Delay a task
/// @param ulDelayTime Delay time in milliseconds
void vAVRTaskDelay(uint32_t ulDelayTime);

/// @brief Delay a task until a specific time
/// @param pxLastWakeTime Pointer to last wake time
/// @param xFrequency Frequency in milliseconds
void vAVRTaskDelayUntil(uint32_t *pxLastWakeTime, uint32_t xFrequency);

/// @brief Check if a task is delaying
/// @param pxTask Pointer to the task handle
/// @return true if task is delaying, false otherwise
bool bAVRIsTaskDelaying(xAVRTaskHandle *pxTask);

/// @brief Get remaining delay time for a task
/// @param pxTask Pointer to the task handle
/// @return Remaining delay time in milliseconds
uint32_t ulAVRGetTaskRemainingDelay(xAVRTaskHandle *pxTask);

/// @brief Start the scheduler (blocking)
void vAVRTaskStartScheduler(void);

/// @brief Start the scheduler (non-blocking)
void vAVRTaskStartSchedulerNonBlocking(void);

/// @brief End the scheduler
void vAVRTaskEndScheduler(void);

/// @brief Schedule tasks
void vAVRTaskSchedule(void);

/// @brief Reset all task timers
void vAVRResetAllTaskTimers(void);

/// @brief Reset a specific task timer
/// @param pxTask Pointer to the task handle
void vAVRResetTaskTimer(xAVRTaskHandle *pxTask);

/// @brief Get current task handle
/// @return Pointer to the current task handle
xAVRTaskHandle *pxAVRGetCurrentTaskHandle(void);

/// @brief Get task state
/// @param pxTask Pointer to the task handle
/// @return Task state
eAVRTaskState eAVRGetTaskState(xAVRTaskHandle *pxTask);

/// @brief Get task name
/// @param pxTask Pointer to the task handle
/// @return Pointer to the task name string
const char* pcAVRGetTaskName(xAVRTaskHandle *pxTask);

/// @brief Get task execution count
/// @param pxTask Pointer to the task handle
/// @return Task execution count
uint32_t ulAVRGetTaskExecutionCount(xAVRTaskHandle *pxTask);

/// @brief Set task execution interval
/// @param pxTask Pointer to the task handle
/// @param ulNewInterval New interval in milliseconds
void vAVRSetTaskInterval(xAVRTaskHandle *pxTask, uint32_t ulNewInterval);

/// @brief Get task execution interval
/// @param pxTask Pointer to the task handle
/// @return Task execution interval in milliseconds
uint32_t ulAVRGetTaskInterval(xAVRTaskHandle *pxTask);

/// @brief Set task priority
/// @param pxTask Pointer to the task handle
/// @param ucNewPriority New priority value
void vAVRSetTaskPriority(xAVRTaskHandle *pxTask, uint8_t ucNewPriority);

/// @brief Get task priority
/// @param pxTask Pointer to the task handle
/// @return Task priority value
uint8_t ucAVRGetTaskPriority(xAVRTaskHandle *pxTask);

/// @brief Get number of tasks
/// @return Number of tasks
uint8_t ucAVRGetNumberOfTasks(void);

/// @brief Get scheduler uptime
/// @return Scheduler uptime in milliseconds
uint32_t ulAVRGetSchedulerUptime(void);

/// @brief Get scheduler statistics
/// @return Scheduler statistics structure
xAVRSchedulerStats xAVRGetSchedulerStats(void);

/// @brief Reset scheduler statistics
void vAVRResetSchedulerStats(void);

/// @brief IDLE task function
/// @param pvParameters Task parameters
void IDLETask(void* pvParameters);

/// @brief Get CPU usage
/// @return CPU usage percentage
float xGetCPUUsage(void);

/// @brief Initalize AVRScheduler
void AVRSchedulerInit(void);

/// @brief Check if millisecond time has elapsed
/// @param timeValue Time value to check
/// @param startTime Start time
/// @return true if time has elapsed, false otherwise
bool AVRDeltaMlsCheck(uint32_t timeValue, uint32_t startTime);

/// @brief Get elapsed milliseconds
/// @param startTime Start time
/// @return Elapsed time in milliseconds
uint32_t ulAVRDeltaMlsGet(uint32_t startTime);

/// @brief Check if microsecond time has elapsed
/// @param timeValue Time value to check
/// @param startTime Start time
/// @return true if time has elapsed, false otherwise
bool AVRDeltaMicroCheck(uint32_t timeValue, uint32_t startTime);

/// @brief Get elapsed microseconds
/// @param startTime Start time
/// @return Elapsed time in microseconds
uint32_t ulAVRDeltaMicroGet(uint32_t startTime);

// Global scheduler instance
extern xAVRSchedulerState xAVRScheduler;
#endif // AVR_TASKSCHEDULER_H
