// AVR_Semphr.h
#ifndef AVR_SEMPHORE_H
#define AVR_SEMPHORE_H

#include <stdint.h>
#include <stdbool.h>
#include "AVR_TaskScheduler.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// Version information
#define AVR_SEMAPHORE_VERSION_MAJOR 1
#define AVR_SEMAPHORE_VERSION_MINOR 0
#define AVR_SEMAPHORE_VERSION_PATCH 0

// Debug option
// #define AVR_SEMAPHORE_DEBUG 1

/// @brief Semaphore type definitions
typedef enum {
    AVR_SEMAPHORE_BINARY = 0,    ///< Binary semaphore
    AVR_SEMAPHORE_COUNTING,      ///< Counting semaphore
    AVR_SEMAPHORE_MUTEX          ///< Mutex semaphore
} eAVRSemaphoreType;

/// @brief Semaphore control block structure
typedef struct avrSemaphoreControlBlock {
    eAVRSemaphoreType eType;             ///< Semaphore type
    volatile int32_t lCount;             ///< Current count value
    int32_t lMaxCount;                   ///< Maximum count value
    xAVRTaskHandle *pxWaitingTasks;      ///< Waiting tasks list
    const char *pcSemaphoreName;         ///< Semaphore name
    xAVRTaskHandle *pxMutexHolder;       ///< Mutex holder (only for MUTEX type)
    uint8_t ucRecursionCount;            ///< Recursion count (only for MUTEX type)
} xAVRSemaphoreHandle;

/// @brief Take result enumeration
typedef enum {
    AVR_SEMAPHORE_TAKE_SUCCESS = 0,      ///< Take successful
    AVR_SEMAPHORE_TAKE_TIMEOUT,          ///< Take timed out
    AVR_SEMAPHORE_TAKE_ERROR             ///< Take error
} eAVRSemaphoreTakeResult;

// ========== Public API Functions ==========

/// @brief Create a semaphore
/// @param eType Type of semaphore to create
/// @param lInitialCount Initial count value
/// @param lMaxCount Maximum count value
/// @param pcName Name of the semaphore
/// @return Pointer to the created semaphore handle, or NULL on failure
xAVRSemaphoreHandle *xAVRSemaphoreCreate(eAVRSemaphoreType eType, 
                                        int32_t lInitialCount, 
                                        int32_t lMaxCount,
                                        const char *pcName);

/// @brief Delete a semaphore
/// @param pxSemaphore Pointer to the semaphore handle to delete
void vAVRSemaphoreDelete(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Take a semaphore (blocking)
/// @param pxSemaphore Pointer to the semaphore handle
/// @param ulTimeoutMs Timeout in milliseconds (0 = no wait, UINT32_MAX = infinite wait)
/// @return Result of the take operation
eAVRSemaphoreTakeResult xAVRSemaphoreTake(xAVRSemaphoreHandle *pxSemaphore, 
                                         uint32_t ulTimeoutMs);

/// @brief Take a semaphore (non-blocking)
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was taken, false otherwise
bool xAVRSemaphoreTakeNonBlocking(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Give a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was given, false otherwise
bool xAVRSemaphoreGive(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Give a semaphore from ISR (Interrupt Service Routine)
/// @param pxSemaphore Pointer to the semaphore handle
/// @return true if semaphore was given, false otherwise
bool xAVRSemaphoreGiveFromISR(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Get the current count of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Current count value, or -1 on error
int32_t lAVRSemaphoreGetCount(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Get the name of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Pointer to the semaphore name string
const char* pcAVRSemaphoreGetName(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Get the type of a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Semaphore type
eAVRSemaphoreType eAVRGetSemaphoreType(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Get the number of tasks waiting on a semaphore
/// @param pxSemaphore Pointer to the semaphore handle
/// @return Number of waiting tasks
uint8_t ucAVRGetSemaphoreWaitingTasks(xAVRSemaphoreHandle *pxSemaphore);

/// @brief Create a mutex
/// @param ppxMutex Pointer to store the created mutex handle
/// @param pcName Name of the mutex
/// @return true if mutex was created, false otherwise
bool xAVRMutexCreate(xAVRSemaphoreHandle **ppxMutex, const char *pcName);

/// @brief Take a mutex
/// @param pxMutex Pointer to the mutex handle
/// @param ulTimeoutMs Timeout in milliseconds (0 = no wait, UINT32_MAX = infinite wait)
/// @return Result of the take operation
eAVRSemaphoreTakeResult xAVRMutexTake(xAVRSemaphoreHandle *pxMutex, uint32_t ulTimeoutMs);

/// @brief Give a mutex
/// @param pxMutex Pointer to the mutex handle
/// @return true if mutex was given, false otherwise
bool xAVRMutexGive(xAVRSemaphoreHandle *pxMutex);

/// @brief Get the holder of a mutex
/// @param pxMutex Pointer to the mutex handle
/// @return Pointer to the task holding the mutex, or NULL
xAVRTaskHandle *pxAVRGetMutexHolder(xAVRSemaphoreHandle *pxMutex);

/// @brief Create a binary semaphore
/// @param ppxSemaphore Pointer to store the created semaphore handle
/// @param pcName Name of the semaphore
/// @param bInitialState Initial state (true = available, false = taken)
/// @return true if semaphore was created, false otherwise
bool xAVRBinarySemaphoreCreate(xAVRSemaphoreHandle **ppxSemaphore, 
                              const char *pcName, 
                              bool bInitialState);

/// @brief Create a counting semaphore
/// @param ppxSemaphore Pointer to store the created semaphore handle
/// @param pcName Name of the semaphore
/// @param lInitialCount Initial count value
/// @param lMaxCount Maximum count value
/// @return true if semaphore was created, false otherwise
bool xAVRCountingSemaphoreCreate(xAVRSemaphoreHandle **ppxSemaphore, 
                                const char *pcName, 
                                int32_t lInitialCount, 
                                int32_t lMaxCount);

#endif // AVR_SEMAPHORE_H
