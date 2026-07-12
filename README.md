AVR TASK SCHEDULER
===================

A lightweight, polling-based task scheduler for AVR Arduino boards with a FreeRTOS-like API.

FEATURES
--------
- millis() based timing (no hardware timer required)
- Supports multiple independent tasks
- Extremely low memory footprint
- FreeRTOS-style API
- Task statistics and performance monitoring
- Dynamic task management (create/suspend/resume/delete)
- Pure software implementation, no hardware conflicts

INSTALLATION
------------

Method 1: Through Arduino IDE Library Manager
1. Open Arduino IDE
2. Click Tools -> Manage Libraries
3. Search for "AVR Task Scheduler"
4. Click Install

Method 2: Manual Installation
1. Download the ZIP file
2. In Arduino IDE: Sketch -> Include Library -> Add .ZIP Library
3. Select the downloaded ZIP file

QUICK START
-----------

Copy and paste the following code into your Arduino sketch:

#include <AVR_TaskScheduler.h>

xAVRTaskHandle myTask;

void myTaskFunction(void *params) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  vAVRTaskCreate(myTaskFunction, "My Task", NULL, 1, &myTask, 500);
  vAVRTaskStartSchedulerNonBlocking();
}

void loop() {
  vAVRTaskSchedule();
  delay(10);
}

EXAMPLES
--------
See File -> Examples -> AVR Task Scheduler for more examples.

API REFERENCE
-------------

Basic Functions
- vAVRTaskCreate()    : Create a new task
- vAVRTaskDelete()    : Delete a task
- vAVRTaskSuspend()   : Suspend a task
- vAVRTaskResume()    : Resume a task
- vAVRTaskDelay()     : Delay the current task

Scheduler Management
- vAVRTaskStartScheduler()           : Start scheduler (blocking)
- vAVRTaskStartSchedulerNonBlocking(): Start scheduler (non-blocking)
- vAVRTaskSchedule()                 : Execute task scheduling

Utility Functions
- vAVRSetTaskInterval()   : Set task interval
- ulAVRGetTaskInterval()  : Get task interval
- ucAVRGetNumberOfTasks() : Get number of tasks

COMPATIBILITY
-------------
- Arduino Uno, Nano, Mega (AVR)
- ESP32, ESP8266
- Arduino Due, Zero (SAMD)
- Other Arduino-compatible boards

LICENSE
-------
This project is licensed under the MIT License - see the LICENSE file for details.

CONTRIBUTING
------------
Issues and Pull Requests are welcome!

1. Fork this repository
2. Create a feature branch (git checkout -b feature/AmazingFeature)
3. Commit your changes (git commit -m 'Add some AmazingFeature')
4. Push to the branch (git push origin feature/AmazingFeature)
5. Open a Pull Request

SUPPORT
-------
If you encounter problems or have suggestions:
- Email: your.email@example.com
- Issues: https://github.com/yourusername/AVR_TaskScheduler/issues
- Discussions: https://github.com/yourusername/AVR_TaskScheduler/discussions

CHANGELOG
---------
v1.0.0
- Initial release
- Basic task scheduling
- Complete examples and documentation

v1.0.1
- Fixed some problems
