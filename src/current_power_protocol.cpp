#include "current_power_protocol.h"
#include <iostream>
#include <iomanip>
#include <cstring>

CurrentPowerProtocol::CurrentPowerProtocol() : currentPowerCallback(nullptr) {}

void CurrentPowerProtocol::setCurrentPowerCallback(std::function<void(float, float)> callback) {
    currentPowerCallback = callback;
}

bool CurrentPowerProtocol::parseFrame(const std::vector<uint8_t>& frame_data) {
    if (!isValidFrame(frame_data)) {
        return false;
    }

    // 提取电流值 (前4字节)
    float current;
    std::memcpy(&current, &frame_data[2], sizeof(float));
    
    // 提取功率值 (接下来4字节)
    float power;
    std::memcpy(&power, &frame_data[6], sizeof(float));

    // 验证剩余8字节是否为0
    bool remaining_zeros = true;
    for (size_t i = 10; i < 18; ++i) {
        if (frame_data[i] != 0) {
            remaining_zeros = false;
            break;
        }
    }

    // 打印结果
    std::cout << "\n=== 电流功率协议数据帧解析结果 ===" << std::endl;
    std::cout << "电流 I: " << std::fixed << std::setprecision(3) << current << " A" << std::endl;
    std::cout << "功率 W: " << std::fixed << std::setprecision(3) << power << " W" << std::endl;
    std::cout << "剩余字节验证: " << (remaining_zeros ? "通过" : "失败") << std::endl;
    
    // 打印原始数据
    std::cout << "原始数据: ";
    for (size_t i = 0; i < frame_data.size(); ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(frame_data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
    std::cout << "=====================================\n" << std::endl;
    
    // 调用回调函数通知串口屏协议
    if (currentPowerCallback) {
        currentPowerCallback(current, power);
    }
    
    return true;
}

bool CurrentPowerProtocol::isValidFrame(const std::vector<uint8_t>& frame_data) {
    if (frame_data.size() != FRAME_SIZE) {
        return false;
    }

    // 验证帧头
    if (frame_data[0] != 0xAA || frame_data[1] != 0xAA) {
        return false;
    }

    // 验证帧尾
    if (frame_data[FRAME_SIZE-2] != 0xFF || frame_data[FRAME_SIZE-1] != 0xFF) {
        return false;
    }

    return true;
}

size_t CurrentPowerProtocol::getFrameSize() const {
    return FRAME_SIZE;
}

std::string CurrentPowerProtocol::getProtocolName() const {
    return "电流功率协议";
}

bool CurrentPowerProtocol::findFrameHeader(struct sp_port* port) {
    uint8_t buffer[2];
    size_t bytes_read;
    
    do {
        bytes_read = sp_blocking_read(port, buffer, 1, 10); // 减少超时时间到10ms
        if (bytes_read == 0) {
            return false; // 超时
        }
    } while (buffer[0] != 0xAA);

    bytes_read = sp_blocking_read(port, buffer + 1, 1, 10); // 减少超时时间到10ms
    if (bytes_read == 0 || buffer[1] != 0xAA) {
        return false;
    }

    return true;
} 