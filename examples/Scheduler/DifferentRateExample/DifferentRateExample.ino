// BasicExample.ino
#include <AVR_TaskScheduler.h>

#define LED_PIN1 2 //One extern LED
#define LED_PIN2 LED_BUILTIN //Builtin LED

// 任务句柄
AVRTaskHandle_t xTask1, xTask2, xTask3;

void vTaskBlinkLED1(void *pvParameters) {
    static bool ledState = false;
    digitalWrite(LED_PIN1, ledState);
    ledState = !ledState;
    
    #ifdef AVR_TASKSCHEDULER_DEBUG
    static uint32_t count = 0;
    Serial.print("LED Task1 executed: ");
    Serial.println(count++);
    #endif
    vAVRTaskDelay(100);
}

void vTaskBlinkLED2(void *pvParameters) {
    static bool ledState = false;
    digitalWrite(LED_PIN2, ledState);
    ledState = !ledState;
    
    #ifdef AVR_TASKSCHEDULER_DEBUG
    static uint32_t count = 0;
    Serial.print("LED Task2 executed: ");
    Serial.println(count++);
    #endif
    vAVRTaskDelay(500);
}

void vTaskPrintCPUUsage(void *pvParameters) {
    char ret[10];
    sprintf(ret, "CPU: %.1f", xGetCPUUsage());
    Serial.println(ret);
    vAVRTaskDelay(500);
}

void setup() {
    // 先初始化硬件
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    
    // 短暂的硬件稳定延迟
    delay(100);
    
    Serial.println("AVR Task Scheduler - Basic Example (Fixed)");
    Serial.println("Initializing tasks...");
    
    if (!vAVRTaskCreate(vTaskBlinkLED1, "LED Blink1", NULL, 1, &xTask1, 0)) {
        Serial.println("Failed to create LED task1!");
    }

    if (!vAVRTaskCreate(vTaskBlinkLED2, "LED Blink2", NULL, 1, &xTask2, 0)) {
        Serial.println("Failed to create LED task2!");
    }

    if (!vAVRTaskCreate(vTaskPrintCPUUsage, "Voltage Output", NULL, 1, &xTask3, 0)) {
        Serial.println("Failed to create CPU Usage task!");
    }
    
    vAVRTaskStartSchedulerNonBlocking();
    
    Serial.println("Scheduler started successfully!");
    Serial.print("Number of tasks: ");
    Serial.println(ucAVRGetNumberOfTasks());
    Serial.print("Scheduler start time: ");
    Serial.println(ulAVRTaskGetTickCount());
}

void loop() {
    vAVRTaskSchedule();
}