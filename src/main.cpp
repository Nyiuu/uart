#include "uart_reader.h"
#include "current_power_protocol.h"
#include "serial_screen_protocol.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

void listAvailablePorts() {
    struct sp_port **ports;
    enum sp_return result = sp_list_ports(&ports);
    if (result != SP_OK) {
        std::cerr << "无法获取串口列表" << std::endl;
        return;
    }

    std::cout << "可用串口列表:" << std::endl;
    for (int i = 0; ports[i]; i++) {
        std::cout << "  " << sp_get_port_name(ports[i]) << std::endl;
    }
    sp_free_port_list(ports);
}

// 电流功率接收线程函数
void currentPowerReceiveThread(UartReader& reader, std::shared_ptr<SerialScreenProtocol> screenProtocol) {
    std::cout << "电流功率接收线程已启动" << std::endl;
    
    while (true) {
        // 接收电流功率数据
        if (reader.readAndParseFrame()) {
            // 数据接收成功，继续处理
        }
        
        // 短暂休息，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 串口屏读写线程函数（同时处理接收和发送）
void serialScreenThread(std::shared_ptr<SerialScreenProtocol> screenProtocol) {
    std::cout << "串口屏读写线程已启动" << std::endl;
    
    // 启动串口屏发送
    screenProtocol->start();
    
    // 串口屏线程会一直运行，直到程序退出
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::cout << "=== 串口通讯程序（双线程架构）===" << std::endl;
    listAvailablePorts();

    std::string current_power_port = "/dev/ttyUSB0";  // 电流功率数据串口
    std::string serial_screen_port = "/dev/ttyUSB1";  // 串口屏串口
    int baud_rate = 9600;

    std::cout << "串口配置:" << std::endl;
    std::cout << "  - 电流功率串口: " << current_power_port << " (波特率: " << baud_rate << ")" << std::endl;
    std::cout << "  - 串口屏串口: " << serial_screen_port << " (波特率: " << baud_rate << ")" << std::endl;

    // 创建串口屏协议（支持读写）
    auto screenProtocol = std::make_shared<SerialScreenProtocol>(serial_screen_port, baud_rate);
    
    // 注册串口屏事件回调函数
    screenProtocol->registerEventCallback(SerialScreenEvent::START_BUTTON, []() {
        std::cout << "*** 处理start按键事件 ***" << std::endl;
        // 这里可以添加start按键的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::KEYBOARD_0, []() {
        std::cout << "*** 处理键盘0事件 ***" << std::endl;
        // 这里可以添加键盘0的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::KEYBOARD_1, []() {
        std::cout << "*** 处理键盘1事件 ***" << std::endl;
        // 这里可以添加键盘1的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::DELETE_BUTTON, []() {
        std::cout << "*** 处理delete按键事件 ***" << std::endl;
        // 这里可以添加delete按键的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::CAMERA_EXPOSURE_PLUS_1, []() {
        std::cout << "*** 处理摄像头曝光+1事件 ***" << std::endl;
        // 这里可以添加摄像头曝光+1的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::CAMERA_EXPOSURE_MINUS_1, []() {
        std::cout << "*** 处理摄像头曝光-1事件 ***" << std::endl;
        // 这里可以添加摄像头曝光-1的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::CAMERA_THRESHOLD_PLUS_1, []() {
        std::cout << "*** 处理相机阈值+1事件 ***" << std::endl;
        // 这里可以添加相机阈值+1的具体处理逻辑
    });
    
    screenProtocol->registerEventCallback(SerialScreenEvent::CAMERA_THRESHOLD_MINUS_1, []() {
        std::cout << "*** 处理相机阈值-1事件 ***" << std::endl;
        // 这里可以添加相机阈值-1的具体处理逻辑
    });
    
    // 创建电流功率协议
    auto currentPowerProtocol = std::make_unique<CurrentPowerProtocol>();

    // 设置电流功率回调，将数据转发到串口屏
    currentPowerProtocol->setCurrentPowerCallback(
        [screenProtocol](float current, float power) {
            screenProtocol->updateCurrentPower(current, power);
        }
    );

    // 创建电流功率串口读取器
    UartReader currentPowerReader(current_power_port, baud_rate);
    currentPowerReader.addProtocol(std::move(currentPowerProtocol));

    // 打开电流功率串口
    if (!currentPowerReader.open()) {
        std::cerr << "无法打开电流功率串口，程序退出" << std::endl;
        return -1;
    }
    
    // 打开串口屏串口（读写模式）
    if (!screenProtocol->open()) {
        std::cerr << "无法打开串口屏串口，程序退出" << std::endl;
        return -1;
    } else {
        std::cout << "串口屏串口已打开（读写模式）" << std::endl;
    }

    std::cout << "启动两个独立线程..." << std::endl;
    
    // 启动两个独立线程
    std::thread currentPowerThread(currentPowerReceiveThread, std::ref(currentPowerReader), screenProtocol);
    std::thread serialScreenThreadObj(serialScreenThread, screenProtocol);
    
    std::cout << "所有线程已启动，主线程等待..." << std::endl;
    
    // 主线程等待所有子线程
    currentPowerThread.join();
    serialScreenThreadObj.join();

    return 0;
} 