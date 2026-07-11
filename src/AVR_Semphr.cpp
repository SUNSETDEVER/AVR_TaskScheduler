// AVR_Semphr.cpp
#include "AVR_Semphr.h"
#include <stdlib.h>

// Internal function declarations
static bool prvIsValidSemaphore(xAVRSemaphoreHandle *pxSemaphore);
static void prvAddTaskToWaitingList(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask);
static void prvRemoveTaskFromWaitingList(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask);
static xAVRTaskHandle *prvGetNextWaitingTask(xAVRSemaphoreHandle *pxSemaphore);
static void prvWakeWaitingTask(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask);
static bool prvCanTaskTakeMutex(xAVRSemaphoreHandle *pxMutex, xAVRTaskHandle *pxTask);

/// @brief Create a semaphore
/// @param eType Type of semaphore to create
/// @param lInitialCount Initial count value
/// @param lMaxCount Maximum count value
/// @param pcName Name of the semaphore
/// @return Pointer to the created semaphore handle, or NULL on failure
xAVRSemaphoreHandle *xAVRSemaphoreCreate(eAVRSemaphoreType eType, 
                                        int32_t lInitialCount, 
                                        int32_t lMaxCount,
                                        const char *pcName) {
    // Parameter validation
    if (lInitialCount < 0 || lMaxCount <= 0 || lInitialCount > lMaxCount) {
        return NULL;
    }

    // Allocate semaphore control block
    xAVRSemaphoreHandle *pxNewSemaphore = 
        (xAVRSemaphoreHandle*)malloc(sizeof(xAVRSemaphoreHandle));

    if (pxNewSemaphore == NULL) {
        return NULL;
    }

    // Initialize semaphore control block
    pxNewSemaphore->eType = eType;
    pxNewSemaphore->lCount = lInitialCount;
    pxNewSemaphore->lMaxCount = lMaxCount;
    pxNewSemaphore->pxWaitingTasks = NULL;
    pxNewSemaphore->pcSemaphoreName = pcName;
    pxNewSemaphore->pxMutexHolder = NULL;
    pxNewSemaphore->ucRecursionCount = 0;

    #ifdef AVR_SEMAPHORE_DEBUG
    Serial.print("Semaphore '");
    Serial.print(pcName);
    Serial.print("' created with count ");
    Serial.println(lInitialCount);
    #endif

    return pxNewSemaphore;
}

/// @brief Delete a semaphore
/// @param pxSemaphore Pointer to the semaphore handle to delete
void vAVRSemaphoreDelete(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return;
    }

    #ifdef AVR_SEMAPHORE_DEBUG
    Serial.print("Deleting semaphore '");
    Serial.print(pxSemaphore->pcSemaphoreName);
    Serial.println("'");
    #endif

    // Wake up all waiting tasks
    xAVRTaskHandle *pxTask = pxSemaphore->pxWaitingTasks;
    while (pxTask != NULL) {
        xAVRTaskHandle *pxNextTask = pxTask->pxNextTask;
        prvWakeWaitingTask(pxSemaphore, pxTask);
        pxTask = pxNextTask;
    }

    // Free semaphore control block
    free(pxSemaphore);
}

/// @brief Take a semaphore (blocking)
/// @param pxSemaphore Pointer to the semaphore handle
/// @param ulTimeoutMs Timeout in milliseconds (0 = no wait, UINT32_MAX = infinite wait)
/// @return Result of the take operation
eAVRSemaphoreTakeResult xAVRSemaphoreTake(xAVRSemaphoreHandle *pxSemaphore, 
                                         uint32_t ulTimeoutMs) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return AVR_SEMAPHORE_TAKE_ERROR;
    }

    xAVRTaskHandle *pxCurrentTask = pxAVRGetCurrentTaskHandle();
    if (pxCurrentTask == NULL) {
        return AVR_SEMAPHORE_TAKE_ERROR;
    }

    // Check if semaphore can be taken directly
    if (pxSemaphore->lCount > 0) {
        if (pxSemaphore->eType == AVR_SEMAPHORE_MUTEX) {
            if (!prvCanTaskTakeMutex(pxSemaphore, pxCurrentTask)) {
                // Mutex already held by current task, recursive take
                if (pxSemaphore->pxMutexHolder == pxCurrentTask) {
                    if (pxSemaphore->ucRecursionCount < 255) {
                        pxSemaphore->ucRecursionCount += 1;
                        #ifdef AVR_SEMAPHORE_DEBUG
                        Serial.print("Mutex recursive take by ");
                        Serial.print(pxCurrentTask->pcTaskName);
                        Serial.print(", recursion count: ");
                        Serial.println(pxSemaphore->ucRecursionCount);
                        #endif
                        return AVR_SEMAPHORE_TAKE_SUCCESS;
                    }
                }
                // Cannot take, need to wait
            } else {
                // Can take mutex
                pxSemaphore->lCount -= 1;
                pxSemaphore->pxMutexHolder = pxCurrentTask;
                pxSemaphore->ucRecursionCount = 1;
                #ifdef AVR_SEMAPHORE_DEBUG
                Serial.print("Mutex taken by ");
                Serial.println(pxCurrentTask->pcTaskName);
                #endif
                return AVR_SEMAPHORE_TAKE_SUCCESS;
            }
        } else {
            // Regular semaphore can be taken directly
            pxSemaphore->lCount -= 1;
            #ifdef AVR_SEMAPHORE_DEBUG
            Serial.print("Semaphore taken by ");
            Serial.print(pxCurrentTask->pcTaskName);
            Serial.print(", count: ");
            Serial.println(pxSemaphore->lCount);
            #endif
            return AVR_SEMAPHORE_TAKE_SUCCESS;
        }
    }

    // Semaphore not available, need to wait
    if (ulTimeoutMs == 0) {
        return AVR_SEMAPHORE_TAKE_TIMEOUT; // Don't wait
    }

    // Add current task to waiting list
    prvAddTaskToWaitingList(pxSemaphore, pxCurrentTask);

    #ifdef AVR_SEMAPHORE_DEBUG
    Serial.print("Task ");
    Serial.print(pxCurrentTask->pcTaskName);
    Serial.print(" waiting for semaphore '");
    Serial.print(pxSemaphore->pcSemaphoreName);
    Serial.print("', timeout: ");
    Serial.println(ulTimeoutMs);
    #endif

    // Set task delay (for timeout detection)
    uint32_t ulStartTime = millis();

    while (true) {
        // Check for timeout
        if (ulTimeoutMs != UINT32_MAX) { // UINT32_MAX means infinite wait
            if (AVRDeltaMlsCheck(ulTimeoutMs, ulStartTime)) {
                prvRemoveTaskFromWaitingList(pxSemaphore, pxCurrentTask);
                #ifdef AVR_SEMAPHORE_DEBUG
                Serial.print("Semaphore take timeout for ");
                Serial.println(pxCurrentTask->pcTaskName);
                #endif
                return AVR_SEMAPHORE_TAKE_TIMEOUT;
            }
        }

        // Check if semaphore is available
        if (pxSemaphore->lCount > 0) {
            if (pxSemaphore->eType == AVR_SEMAPHORE_MUTEX) {
                if (prvCanTaskTakeMutex(pxSemaphore, pxCurrentTask)) {
                    pxSemaphore->lCount -= 1;
                    pxSemaphore->pxMutexHolder = pxCurrentTask;
                    pxSemaphore->ucRecursionCount = 1;
                    prvRemoveTaskFromWaitingList(pxSemaphore, pxCurrentTask);
                    #ifdef AVR_SEMAPHORE_DEBUG
                    Serial.print("Mutex acquired after wait by ");
                    Serial.println(pxCurrentTask->pcTaskName);
                    #endif
                    return AVR_SEMAPHORE_TAKE_SUCCESS;
                }
            } else {
                // Regular semaphore
                pxSemaphore->lCount -= 1;
                prvRemoveTaskFromWaitingList(pxSemaphore, pxCurrentTask);
                #ifdef AVR_SEMAPHORE_DEBUG
                Serial.print("Semaphore acquired after wait by ");
                Serial.println(pxCurrentTask->pcTaskName);
                #endif
                return AVR_SEMAPHORE_TAKE_SUCCESS;
            }
        }

        // Yield CPU time slice
        vAVRTaskDelay(1);
    }
}

/// @brief Take a semaphore (non-blocking)
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was taken, false otherwise
bool xAVRSemaphoreTakeNonBlocking(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return false;
    }

    if (pxSemaphore->lCount > 0) {
        if (pxSemaphore->eType == AVR_SEMAPHORE_MUTEX) {
            xAVRTaskHandle *pxCurrentTask = pxAVRGetCurrentTaskHandle();
            if (prvCanTaskTakeMutex(pxSemaphore, pxCurrentTask)) {
                pxSemaphore->lCount -= 1;
                pxSemaphore->pxMutexHolder = pxCurrentTask;
                pxSemaphore->ucRecursionCount = 1;
                return true;
            }
            return false;
        } else {
            pxSemaphore->lCount -= 1;
            return true;
        }
    }

    return false;
}

/// @brief Give a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was given, false otherwise
bool xAVRSemaphoreGive(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return false;
    }

    if (pxSemaphore->eType == AVR_SEMAPHORE_MUTEX) {
        xAVRTaskHandle *pxCurrentTask = pxAVRGetCurrentTaskHandle();

        // Check if current task owns the mutex
        if (pxSemaphore->pxMutexHolder != pxCurrentTask) {
            #ifdef AVR_SEMAPHORE_DEBUG
            Serial.println("Error: Mutex not owned by current task");
            #endif
            return false;
        }

        // Handle recursive release
        if (pxSemaphore->ucRecursionCount > 1) {
            pxSemaphore->ucRecursionCount -= 1;
            #ifdef AVR_SEMAPHORE_DEBUG
            Serial.print("Mutex recursive release by ");
            Serial.print(pxCurrentTask->pcTaskName);
            Serial.print(", recursion count: ");
            Serial.println(pxSemaphore->ucRecursionCount);
            #endif
            return true;
        }

        // Fully release mutex
        pxSemaphore->ucRecursionCount = 0;
        pxSemaphore->pxMutexHolder = NULL;
    }

    // Check if maximum count has been reached
    if (pxSemaphore->lCount >= pxSemaphore->lMaxCount) {
        #ifdef AVR_SEMAPHORE_DEBUG
        Serial.println("Error: Semaphore count at maximum");
        #endif
        return false;
    }

    pxSemaphore->lCount += 1;

    // Wake up the first task in the waiting list
    xAVRTaskHandle *pxWaitingTask = prvGetNextWaitingTask(pxSemaphore);
    if (pxWaitingTask != NULL) {
        prvWakeWaitingTask(pxSemaphore, pxWaitingTask);
    }

    #ifdef AVR_SEMAPHORE_DEBUG
    Serial.print("Semaphore released, count: ");
    Serial.println(pxSemaphore->lCount);
    #endif

    return true;
}

/// @brief Give a semaphore from ISR (Interrupt Service Routine)
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was given, false otherwise
bool xAVRSemaphoreGiveFromISR(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return false;
    }

    // Check if maximum count has been reached
    if (pxSemaphore->lCount >= pxSemaphore->lMaxCount) {
        return false;
    }

    pxSemaphore->lCount += 1;

    // Note: Cannot directly manipulate task state in interrupt context
    // Just increment count, waiting tasks will check on next schedule

    return true;
}

// ========== Dedicated creation functions ==========

/// @brief Create a mutex
/// @param ppxMutex Pointer to store the created mutex handle
/// @param pcName Name of the mutex
/// @return true if mutex was created, false otherwise
bool xAVRMutexCreate(xAVRSemaphoreHandle **ppxMutex, const char *pcName) {
    if (ppxMutex == NULL) {
        return false;
    }

    *ppxMutex = xAVRSemaphoreCreate(AVR_SEMAPHORE_MUTEX, 1, 1, pcName);
    return (*ppxMutex != NULL);
}

/// @brief Take a mutex
/// @param pxMutex Pointer to the mutex handle
/// @param ulTimeoutMs Timeout in milliseconds (0 = no wait, UINT32_MAX = infinite wait)
/// @return Result of the take operation
eAVRSemaphoreTakeResult xAVRMutexTake(xAVRSemaphoreHandle *pxMutex, uint32_t ulTimeoutMs) {
    if (pxMutex == NULL || pxMutex->eType != AVR_SEMAPHORE_MUTEX) {
        return AVR_SEMAPHORE_TAKE_ERROR;
    }

    return xAVRSemaphoreTake(pxMutex, ulTimeoutMs);
}

/// @brief Give a mutex
/// @param pxMutex Pointer to the mutex handle
/// @return true if mutex was given, false otherwise
bool xAVRMutexGive(xAVRSemaphoreHandle *pxMutex) {
    if (pxMutex == NULL || pxMutex->eType != AVR_SEMAPHORE_MUTEX) {
        return false;
    }

    return xAVRSemaphoreGive(pxMutex);
}

/// @brief Create a binary semaphore
/// @param ppxSemaphore Pointer to store the created semaphore handle
/// @param pcName Name of the semaphore
/// @param bInitialState Initial state (true = available, false = taken)
/// @return true if semaphore was created, false otherwise
bool xAVRBinarySemaphoreCreate(xAVRSemaphoreHandle **ppxSemaphore, 
                              const char *pcName, 
                              bool bInitialState) {
    if (ppxSemaphore == NULL) {
        return false;
    }

    *ppxSemaphore = xAVRSemaphoreCreate(AVR_SEMAPHORE_BINARY, 
                                       bInitialState ? 1 : 0, 
                                       1, 
                                       pcName);
    return (*ppxSemaphore != NULL);
}

/// @brief Create a counting semaphore
/// @param ppxSemaphore Pointer to store the created semaphore handle
/// @param pcName Name of the semaphore
/// @param lInitialCount Initial count value
/// @param lMaxCount Maximum count value
/// @return true if semaphore was created, false otherwise
bool xAVRCountingSemaphoreCreate(xAVRSemaphoreHandle **ppxSemaphore, 
                                const char *pcName, 
                                int32_t lInitialCount, 
                                int32_t lMaxCount) {
    if (ppxSemaphore == NULL) {
        return false;
    }

    *ppxSemaphore = xAVRSemaphoreCreate(AVR_SEMAPHORE_COUNTING, 
                                       lInitialCount, 
                                       lMaxCount, 
                                       pcName);
    return (*ppxSemaphore != NULL);
}

// ========== Information functions ==========

/// @brief Get the current count of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Current count value, or -1 on error
int32_t lAVRSemaphoreGetCount(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return -1;
    }
    return pxSemaphore->lCount;
}

/// @brief Get the name of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Pointer to the semaphore name string
const char* pcAVRSemaphoreGetName(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return "Invalid Semaphore";
    }
    return pxSemaphore->pcSemaphoreName;
}

/// @brief Get the type of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Semaphore type
eAVRSemaphoreType eAVRGetSemaphoreType(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return AVR_SEMAPHORE_BINARY;
    }
    return pxSemaphore->eType;
}

/// @brief Get the number of tasks waiting on a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Number of waiting tasks
uint8_t ucAVRGetSemaphoreWaitingTasks(xAVRSemaphoreHandle *pxSemaphore) {
    if (!prvIsValidSemaphore(pxSemaphore)) {
        return 0;
    }

    uint8_t ucCount = 0;
    xAVRTaskHandle *pxTask = pxSemaphore->pxWaitingTasks;
    while (pxTask != NULL) {
        ucCount += 1;
        pxTask = pxTask->pxNextTask;
    }

    return ucCount;
}

/// @brief Get the holder of a mutex
/// @param pxMutex Pointer to the mutex handle
/// @return Pointer to the task holding the mutex, or NULL
xAVRTaskHandle *pxAVRGetMutexHolder(xAVRSemaphoreHandle *pxMutex) {
    if (!prvIsValidSemaphore(pxMutex) || pxMutex->eType != AVR_SEMAPHORE_MUTEX) {
        return NULL;
    }
    return pxMutex->pxMutexHolder;
}

// ========== Internal function implementations ==========

/// @brief Check if semaphore is valid
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore is valid, false otherwise
static bool prvIsValidSemaphore(xAVRSemaphoreHandle *pxSemaphore) {
    return (pxSemaphore != NULL);
}

/// @brief Add task to waiting list
/// @param pxSemaphore Pointer to the semaphore handle
/// @param pxTask Pointer to the task handle
static void prvAddTaskToWaitingList(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask) {
    if (pxSemaphore == NULL || pxTask == NULL) {
        return;
    }

    // Set task state to suspended
    pxTask->eCurrentState = AVR_TASK_SUSPENDED;

    // Add to list head
    pxTask->pxNextTask = pxSemaphore->pxWaitingTasks;
    pxSemaphore->pxWaitingTasks = pxTask;
}

/// @brief Remove task from waiting list
/// @param pxSemaphore Pointer to the semaphore handle
/// @param pxTask Pointer to the task handle
static void prvRemoveTaskFromWaitingList(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask) {
    if (pxSemaphore == NULL || pxTask == NULL) {
        return;
    }

    // Remove task from list
    if (pxSemaphore->pxWaitingTasks == pxTask) {
        pxSemaphore->pxWaitingTasks = pxTask->pxNextTask;
    } else {
        xAVRTaskHandle *pxCurrent = pxSemaphore->pxWaitingTasks;
        while (pxCurrent != NULL && pxCurrent->pxNextTask != pxTask) {
            pxCurrent = pxCurrent->pxNextTask;
        }
        if (pxCurrent != NULL) {
            pxCurrent->pxNextTask = pxTask->pxNextTask;
        }
    }

    // Restore task state
    pxTask->eCurrentState = AVR_TASK_READY;
    pxTask->pxNextTask = NULL;
}

/// @brief Get next waiting task (FIFO order)
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Pointer to the next waiting task, or NULL
static xAVRTaskHandle *prvGetNextWaitingTask(xAVRSemaphoreHandle *pxSemaphore) {
    if (pxSemaphore == NULL) {
        return NULL;
    }
    return pxSemaphore->pxWaitingTasks;
}

/// @brief Wake up waiting task
/// @param pxSemaphore Pointer to the semaphore handle
/// @param pxTask Pointer to the task handle
static void prvWakeWaitingTask(xAVRSemaphoreHandle *pxSemaphore, xAVRTaskHandle *pxTask) {
    if (pxSemaphore == NULL || pxTask == NULL) {
        return;
    }

    prvRemoveTaskFromWaitingList(pxSemaphore, pxTask);

    #ifdef AVR_SEMAPHORE_DEBUG
    Serial.print("Waking task ");
    Serial.println(pxTask->pcTaskName);
    #endif
}

/// @brief Check if task can take mutex
/// @param pxMutex Pointer to the mutex handle
/// @param pxTask Pointer to the task handle
/// @return true if task can take mutex, false otherwise
static bool prvCanTaskTakeMutex(xAVRSemaphoreHandle *pxMutex, xAVRTaskHandle *pxTask) {
    if (pxMutex->pxMutexHolder == NULL) {
        return true; // Mutex not held
    }
    if (pxMutex->pxMutexHolder == pxTask) {
        return true; // Recursive take
    }
    return false; // Held by another task
}
