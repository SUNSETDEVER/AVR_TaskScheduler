// BasicExample.ino
#include <AVR_TaskScheduler.h>

// 任务句柄
AVRTaskHandle_t xTask1, xTask2;

// 任务1：闪烁LED
void vTaskBlinkLED(void *pvParameters) {
    static bool ledState = false;
    digitalWrite(LED_BUILTIN, ledState);
    ledState = !ledState;
    
    #ifdef AVR_TASKSCHEDULER_DEBUG
    static uint32_t count = 0;
    Serial.print("LED Task executed: ");
    Serial.println(count++);
    #endif
    vAVRTaskDelay(500);
}

// 任务2：电压输出
void vTaskVoltageOutput(void *pvParameters) {
    float voltage = (analogRead(0) / 1023.0) * 5.0;
    Serial.print("ADC Task read: ");
    Serial.println(voltage);
    vAVRTaskDelay(1000);
}

void setup() {
    // 先初始化硬件
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    
    // 短暂的硬件稳定延迟
    delay(100);
    
    Serial.println("AVR Task Scheduler - Basic Example (Fixed)");
    Serial.println("Initializing tasks...");
    
    if (!vAVRTaskCreate(vTaskBlinkLED, "LED Blink", NULL, 1, &xTask1, 0)) {
        Serial.println("Failed to create LED task!");
    }

    if (!vAVRTaskCreate(vTaskVoltageOutput, "Voltage Output", NULL, 1, &xTask2, 0)) {
        Serial.println("Failed to create Voltage task!");
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