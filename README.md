# 串口通讯程序

基于C++17的异步串口通讯程序，支持电流功率数据接收和串口屏双向通信。

## 功能特性

- ~~🔄 **异步多线程架构**：电流功率接收、串口屏接收、串口屏发送三线程分离~~
- 🔄 **单线程**：更加简洁
- ⚡ **零延迟响应**：使用条件变量实现真正的异步通知
- 🎯 **事件回调系统**：支持串口屏按键事件的灵活处理
- 📊 **实时数据显示**：电流、功率、最大功率实时监控
- 🎮 **串口屏控制**：支持start按键、数字键盘、摄像头控制等

## 快速开始

### 编译
```bash
./build.sh
```

### 运行
```bash
./build/uart_program
```

## 串口配置

- **电流功率串口**：`/dev/ttyUSB0` (9600波特率)
- **串口屏串口**：`/dev/ttyUSB1` (9600波特率)

## 支持的事件

| 事件类型 | 功能 |
|---------|------|
| START_BUTTON | 启动按键 |
| KEYBOARD_0-9 | 数字键盘 |
| DELETE_BUTTON | 删除按键 |
| CAMERA_EXPOSURE_* | 摄像头曝光控制 |
| CAMERA_THRESHOLD_* | 相机阈值控制 |

## 项目结构

```
uart/
├── inc/                    # 头文件
│   ├── protocol.h         # 协议基类
│   ├── uart_reader.h      # 串口读取器
│   ├── current_power_protocol.h    # 电流功率协议
│   └── serial_screen_protocol.h    # 串口屏协议
├── src/                   # 源文件
│   ├── main.cpp          # 主程序
│   ├── uart_reader.cpp   # 串口读取器实现
│   ├── current_power_protocol.cpp  # 电流功率协议实现
│   └── serial_screen_protocol.cpp  # 串口屏协议实现
├── build.sh              # 编译脚本
├── CMakeLists.txt        # CMake配置
└── README.md            # 项目说明
```

## 依赖

- C++17
- CMake 3.10+
- libserialport

## 许可证

MIT License 