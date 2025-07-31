#include "uart_reader.h"
#include "current_power_protocol.h"
#include "serial_screen_protocol.h"
#include <iostream>
#include <thread>
#include <chrono>

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

// 融合线程函数（接收+发送）
void fusedThread(UartReader& reader, std::shared_ptr<SerialScreenProtocol> screenProtocol) {
    std::cout << "融合线程已启动（接收+发送）" << std::endl;
    
    // 启动串口屏发送
    screenProtocol->start();
    
    while (true) {
        // 接收数据
        if (reader.readAndParseFrame()) {
            // 数据接收成功，继续处理
        }
        
        // 发送数据到串口屏（非阻塞）
        // 这里我们让串口屏协议自己处理发送
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 短暂休息
    }
}

int main() {
    std::cout << "=== 串口通讯程序 ===" << std::endl;
    listAvailablePorts();

    std::string receive_port_name = "/dev/ttyUSB0";
    std::string send_port_name = "/dev/ttyUSB1";
    int receive_baud_rate = 9600;
    int send_baud_rate = 9600;

    std::cout << "串口配置:" << std::endl;
    std::cout << "  - 接收串口: " << receive_port_name << " (波特率: " << receive_baud_rate << ")" << std::endl;
    std::cout << "  - 发送串口: " << send_port_name << " (波特率: " << send_baud_rate << ")" << std::endl;

    // 创建串口屏发送协议
    auto screenProtocol = std::make_shared<SerialScreenProtocol>(send_port_name, send_baud_rate);
    
    // 创建电流功率协议
    auto currentPowerProtocol = std::make_unique<CurrentPowerProtocol>();

    // 设置电流功率回调，将数据转发到串口屏
    currentPowerProtocol->setCurrentPowerCallback(
        [screenProtocol](float current, float power) {
            screenProtocol->updateCurrentPower(current, power);
        }
    );

    // 创建串口读取器
    UartReader dataReader(receive_port_name, receive_baud_rate);
    
    // 添加电流功率协议到读取器
    dataReader.addProtocol(std::move(currentPowerProtocol));
    
    // 添加串口屏接收协议到读取器（用于接收按键事件）
    // 注意：这里使用接收串口，因为串口屏的按键事件是从接收串口发送过来的
    auto screenReceiveProtocol = std::make_unique<SerialScreenProtocol>(receive_port_name, receive_baud_rate);
    
    // 设置串口屏接收协议的回调，当收到start按键时通知发送协议
    screenReceiveProtocol->setStartButtonCallback(
        [screenProtocol]() {
            screenProtocol->notifyStartButtonPressed();
        }
    );
    
    dataReader.addProtocol(std::move(screenReceiveProtocol));

    // 打开接收串口
    if (!dataReader.open()) {
        std::cerr << "无法打开接收串口，程序退出" << std::endl;
        return -1;
    }
    
    // 打开发送串口
    if (!screenProtocol->open()) {
        std::cerr << "警告: 无法打开发送串口，将只进行数据接收" << std::endl;
    } else {
        std::cout << "发送串口已打开" << std::endl;
    }

    std::cout << "开始融合线程运行..." << std::endl;
    std::thread fusedThreadObj(fusedThread, std::ref(dataReader), screenProtocol);
    fusedThreadObj.join();

    return 0;
} 