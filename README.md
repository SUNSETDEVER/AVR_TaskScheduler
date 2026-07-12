# AVR Task Scheduler

A lightweight, polling-based task scheduler for AVR Arduino boards with a FreeRTOS-like API.

## Features

- 🕐 `millis()`-based timing (no hardware timers required)
- 🔄 Support for multiple independent tasks
- ⚡ Extremely low memory footprint
- 🎯 FreeRTOS-like API style
- 📊 Task statistics and performance monitoring
- 🔧 Dynamic task management (create / suspend / resume / delete)
- 💾 Pure software implementation, no hardware conflicts

## Installation

### Method 1: Through the Arduino IDE Library Manager
1. Open the Arduino IDE.
2. Click **Tools -> Manage Libraries**.
3. Search for "AVR Task Scheduler".
4. Click **Install**.

### Method 2: Manual Installation
1. Download the ZIP file.
2. In the Arduino IDE: **Sketch -> Include Library -> Add .ZIP Library**.
3. Select the downloaded ZIP file.

## Quick Start

Here is a simple example to get you started. The following code creates a task that toggles the built-in LED every 500 milliseconds.

```cpp
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
Examples
Check out File -> Examples -> AVR Task Scheduler for more sample sketches.

API Reference
Basic Functions
Function	Description
vAVRTaskCreate()	Create a new task
vAVRTaskDelete()	Delete a task
vAVRTaskSuspend()	Suspend a task
vAVRTaskResume()	Resume a task
vAVRTaskDelay()	Delay the current task
Scheduler Management
Function	Description
vAVRTaskStartScheduler()	Start the scheduler (blocking)
vAVRTaskStartSchedulerNonBlocking()	Start the scheduler (non-blocking)
vAVRTaskSchedule()	Execute task scheduling (call in loop())
Utility Functions
Function	Description
vAVRSetTaskInterval()	Set the interval for a task
ulAVRGetTaskInterval()	Get the interval of a task
ucAVRGetNumberOfTasks()	Get the current number of tasks
Compatibility
✅ Arduino Uno, Nano, Mega (AVR)

✅ ESP32, ESP8266

✅ Arduino Due, Zero (SAMD)

✅ Other Arduino-compatible boards

License
This project is licensed under the MIT License - see the LICENSE file for details.

Contributing
Issues and Pull Requests are welcome!

Fork this repository.

Create a feature branch (git checkout -b feature/AmazingFeature).

Commit your changes (git commit -m 'Add some AmazingFeature').

Push to the branch (git push origin feature/AmazingFeature).

Open a Pull Request.

Support
If you encounter any issues or have suggestions:

📧 Email: your.email@example.com

🐛 Issues: GitHub Issues

💬 Discussions: GitHub Discussions

Changelog
v1.0.0
Initial release

Basic task scheduling functionality

Complete examples and documentation
