#include <AVR_TaskScheduler.h>

AVRTaskHandle_t xloopTask;

//SCHEDULER TASKS:
void setupTask() {
//Put your code here and run once:

}

void LoopTask(void* pvParameters){
//Put your main code here to run repeatedly:

}
//END SCHEDULER TASKS
//SYSTEM VOID TASKS:
void setup() {
 setupTask();
 vAVRTaskCreate(LoopTask, "LoopTask", NULL, 1, &xloopTask, 0);
 AVRSchedulerInit();
}

void loop() {

}
//END SYSTEM TASKS
