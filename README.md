# AVR_TaskScheduler

一个轻量级的、基于轮询的任务调度器，适用于AVR Arduino开发板，具有类似FreeRTOS的API。

A lightweight, polling-based task scheduler for AVR Arduino boards with FreeRTOS-like API.

## 特性 Features

- 🕐 基于 `millis()` 的定时（不需要硬件定时器）
- 🔄 支持多个独立任务
- ⚡ 极低的内存占用
- 🎯 类似FreeRTOS的API风格
- 📊 任务统计和性能监控
- 🔧 动态任务管理（创建/挂起/恢复/删除）
- 💾 纯软件实现，无硬件冲突

## 安装 Installation

### 方法1：通过Arduino IDE库管理器
1. 打开Arduino IDE
2. 点击 工具 -> 管理库
3. 搜索 "AVR Task Scheduler"
4. 点击安装

### 方法2：手动安装
1. 下载ZIP文件
2. 在Arduino IDE中：项目 -> 加载库 -> 添加.ZIP库
3. 选择下载的ZIP文件

## 快速开始 Quick Start
cpp
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
## 示例 Examples

查看 `文件 -> 示例 -> AVR Task Scheduler` 获取更多示例：

## API参考 API Reference

### 基本函数 Basic Functions

| 函数 Function | 描述 Description |
|-------------|-----------------|
| `vAVRTaskCreate()` | 创建新任务 Create new task |
| `vAVRTaskDelete()` | 删除任务 Delete task |
| `vAVRTaskSuspend()` | 挂起任务 Suspend task |
| `vAVRTaskResume()` | 恢复任务 Resume task |
| `vAVRTaskDelay()` | 任务延时 Task delay |

### 调度器管理 Scheduler Management

| 函数 Function | 描述 Description |
|-------------|-----------------|
| `vAVRTaskStartScheduler()` | 启动调度器（阻塞式）Start scheduler (blocking) |
| `vAVRTaskStartSchedulerNonBlocking()` | 启动调度器（非阻塞式）Start scheduler (non-blocking) |
| `vAVRTaskSchedule()` | 执行任务调度 Execute task scheduling |

### 工具函数 Utility Functions

| 函数 Function | 描述 Description |
|-------------|-----------------|
| `vAVRSetTaskInterval()` | 设置任务间隔 Set task interval |
| `ulAVRGetTaskInterval()` | 获取任务间隔 Get task interval |
| `ucAVRGetNumberOfTasks()` | 获取任务数量 Get number of tasks |

## 兼容性 Compatibility

- ✅ Arduino Uno, Nano, Mega (AVR)
- ✅ ESP32, ESP8266
- ✅ Arduino Due, Zero (SAMD)
- ✅ 其他Arduino兼容板 Other Arduino-compatible boards

## 许可证 License

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 贡献 Contributing

欢迎提交Issue和Pull Request！

1. Fork本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开Pull Request

## 支持 Support

如果你遇到问题或有建议：

- 📧 发送邮件到: your.email@example.com
- 🐛 提交Issue: [GitHub Issues](https://github.com/yourusername/AVR_TaskScheduler/issues)
- 💬 讨论区: [GitHub Discussions](https://github.com/yourusername/AVR_TaskScheduler/discussions)

## 更新日志 Changelog

### v1.0.0
- 初始发布 Initial release
- 基础任务调度功能 Basic task scheduling
- 完整的示例和文档 Complete examples and documentation
