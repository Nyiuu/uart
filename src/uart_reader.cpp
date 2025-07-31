#include "uart_reader.h"
#include "current_power_protocol.h"
#include "serial_screen_protocol.h"
#include <iostream>
#include <iomanip>

UartReader::UartReader(const std::string& port_name, int baud_rate) 
    : port_name(port_name), baud_rate(baud_rate), port(nullptr) {}

UartReader::~UartReader() {
    if (port) {
        sp_close(port);
        sp_free_port(port);
    }
}

void UartReader::addProtocol(std::unique_ptr<Protocol> protocol) {
    protocols.push_back(std::move(protocol));
}

bool UartReader::open() {
    int result = sp_get_port_by_name(port_name.c_str(), &port);
    if (result != SP_OK) {
        std::cerr << "无法打开串口: " << port_name << std::endl;
        return false;
    }

    result = sp_open(port, SP_MODE_READ);
    if (result != SP_OK) {
        std::cerr << "无法打开串口进行读取" << std::endl;
        return false;
    }

    // 设置完整的串口参数，与系统配置保持一致
    result = sp_set_baudrate(port, baud_rate);
    if (result != SP_OK) {
        std::cerr << "无法设置波特率: " << baud_rate << std::endl;
        return false;
    }

    // 设置数据位为8位
    result = sp_set_bits(port, 8);
    if (result != SP_OK) {
        std::cerr << "无法设置数据位" << std::endl;
        return false;
    }

    // 设置停止位为1位
    result = sp_set_stopbits(port, 1);
    if (result != SP_OK) {
        std::cerr << "无法设置停止位" << std::endl;
        return false;
    }

    // 设置无校验位
    result = sp_set_parity(port, SP_PARITY_NONE);
    if (result != SP_OK) {
        std::cerr << "无法设置校验位" << std::endl;
        return false;
    }

    // 设置无流控制
    result = sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
    if (result != SP_OK) {
        std::cerr << "无法设置流控制" << std::endl;
        return false;
    }

    std::cout << "成功打开串口: " << port_name << " 波特率: " << baud_rate 
              << " 数据位: 8 停止位: 1 校验位: 无 流控制: 无" << std::endl;
    return true;
}

bool UartReader::readAndParseFrame() {
    // 读取一个字节来判断协议类型
    uint8_t first_byte;
    size_t bytes_read = sp_blocking_read(port, &first_byte, 1, 100); // 优化超时时间到100ms
    
    if (bytes_read == 0) {
        return false; // 没有数据
    }
    
    // 根据第一个字节判断协议类型
    if (first_byte == 0xAA) {
        // 可能是电流功率协议，需要读取第二个字节确认
        uint8_t second_byte;
        bytes_read = sp_blocking_read(port, &second_byte, 1, 100); // 优化超时时间
        if (bytes_read == 0 || second_byte != 0xAA) {
            return false;
        }
        
        // 确认是电流功率协议，读取剩余数据
        return tryReadCurrentPowerFrame(first_byte, second_byte);
        
    } else if (first_byte == 0x65) {
        // 串口屏协议，读取剩余数据
        return tryReadSerialScreenFrame(first_byte);
    }
    
    return false; // 未知协议
}

bool UartReader::tryReadCurrentPowerFrame(uint8_t first_byte, uint8_t second_byte) {
    // 查找电流功率协议处理器
    for (auto& protocol : protocols) {
        if (auto currentPowerProtocol = dynamic_cast<CurrentPowerProtocol*>(protocol.get())) {
            std::vector<uint8_t> frame_data;
            frame_data.push_back(first_byte);  // 0xAA
            frame_data.push_back(second_byte); // 0xAA
            
            // 读取剩余的数据
            size_t remaining_size = currentPowerProtocol->getFrameSize() - 2;
            uint8_t buffer[remaining_size];
            size_t bytes_read = sp_blocking_read(port, buffer, remaining_size, 100);
            
            if (bytes_read != remaining_size) {
                return false;
            }
            
            // 添加剩余数据到frame_data
            frame_data.insert(frame_data.end(), buffer, buffer + bytes_read);
            
            // 解析数据帧
            if (currentPowerProtocol->parseFrame(frame_data)) {
                return true;
            }
            break;
        }
    }
    return false;
}

bool UartReader::tryReadSerialScreenFrame(uint8_t first_byte) {
    // 查找串口屏协议处理器
    for (auto& protocol : protocols) {
        if (auto serialScreenProtocol = dynamic_cast<SerialScreenProtocol*>(protocol.get())) {
            std::vector<uint8_t> frame_data;
            frame_data.push_back(first_byte);  // 0x65
            
            // 读取剩余的数据（使用动态帧大小）
            size_t remaining_size = serialScreenProtocol->getFrameSize() - 1; // 减去已读取的第一个字节
            uint8_t buffer[remaining_size];
            size_t bytes_read = sp_blocking_read(port, buffer, remaining_size, 200); // 优化超时时间到200ms
            
            if (bytes_read != remaining_size) {
                return false;
            }
            
            // 添加剩余数据到frame_data
            frame_data.insert(frame_data.end(), buffer, buffer + bytes_read);
            
            // 解析数据帧
            if (serialScreenProtocol->parseFrame(frame_data)) {
                return true;
            }
            break;
        }
    }
    return false;
} 