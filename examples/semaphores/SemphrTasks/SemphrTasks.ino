#include "AVR_TaskScheduler.h"
#include "AVR_Semphr.h"

// 任务句柄声明
xAVRTaskHandle xTask1, xTask2, xTask3;

// 信号量声明
xAVRSemaphoreHandle *pxUartMutex;
xAVRSemaphoreHandle *pxDataSemaphore;

// 共享资源
volatile int iSharedCounter = 0;

void setup() {
    Serial.begin(9600);
    
    // 短暂的硬件稳定延迟
    delay(100);

    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== 修复后的信号量示例 ===");
    
    // 创建信号量
    xAVRMutexCreate(&pxUartMutex, "UART_Mutex");
    xAVRBinarySemaphoreCreate(&pxDataSemaphore, "Data_Semaphore", false);
    
    // 创建任务
    vAVRTaskCreate(vTask1, "Task1", NULL, 3, &xTask1, 1000);
    vAVRTaskCreate(vTask2, "Task2", NULL, 2, &xTask2, 1500);
    vAVRTaskCreate(vTask3, "Task3", NULL, 1, &xTask3, 2000);

    vAVRResetAllTaskTimers();
    
    // 非阻塞方式启动调度器
    vAVRTaskStartSchedulerNonBlocking();
}

void loop() {
    vAVRTaskSchedule();
}

// 任务函数
void vTask1(void *pvParameters) {
    static int iCount = 0;
    
    // 安全打印
    if (xAVRMutexTake(pxUartMutex, 100) == AVR_SEMAPHORE_TAKE_SUCCESS) {
        Serial.print("Task1: Count=");
        Serial.println(++iCount);
        xAVRMutexGive(pxUartMutex);
    }
    
    // 释放数据信号量
    xAVRSemaphoreGive(pxDataSemaphore);
}

void vTask2(void *pvParameters) {
    // 等待数据信号量
    if (xAVRSemaphoreTake(pxDataSemaphore, 2000) == AVR_SEMAPHORE_TAKE_SUCCESS) {
        if (xAVRMutexTake(pxUartMutex, 100) == AVR_SEMAPHORE_TAKE_SUCCESS) {
            Serial.println("Task2: Received data signal");
            xAVRMutexGive(pxUartMutex);
        }
    }
}

void vTask3(void *pvParameters) {
    if (xAVRMutexTake(pxUartMutex, 100) == AVR_SEMAPHORE_TAKE_SUCCESS) {
        Serial.println("Task3: Monitoring system");
        xAVRMutexGive(pxUartMutex);
    }
}
