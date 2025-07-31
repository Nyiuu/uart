#include "serial_screen_protocol.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>

SerialScreenProtocol::SerialScreenProtocol(const std::string& port_name, int baud_rate) 
    : port_name(port_name), baud_rate(baud_rate), port(nullptr), 
      distance_D(5.0f), side_length_x(5.0f), current_I(0.0f), power_P(0.0f), max_power(0.0f),
      start_received(false), data_updated(false), running(false) {}

SerialScreenProtocol::~SerialScreenProtocol() {
    stop();
    close();
}

bool SerialScreenProtocol::open() {
    int result = sp_get_port_by_name(port_name.c_str(), &port);
    if (result != SP_OK) {
        std::cerr << "无法打开串口屏串口: " << port_name << std::endl;
        return false;
    }

    result = sp_open(port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        std::cerr << "无法打开串口屏串口进行读写" << std::endl;
        return false;
    }

    // 设置完整的串口参数，与系统配置保持一致
    result = sp_set_baudrate(port, baud_rate);
    if (result != SP_OK) {
        std::cerr << "无法设置串口屏波特率: " << baud_rate << std::endl;
        return false;
    }

    // 设置数据位为8位
    result = sp_set_bits(port, 8);
    if (result != SP_OK) {
        std::cerr << "无法设置串口屏数据位" << std::endl;
        return false;
    }

    // 设置停止位为1位
    result = sp_set_stopbits(port, 1);
    if (result != SP_OK) {
        std::cerr << "无法设置串口屏停止位" << std::endl;
        return false;
    }

    // 设置无校验位
    result = sp_set_parity(port, SP_PARITY_NONE);
    if (result != SP_OK) {
        std::cerr << "无法设置串口屏校验位" << std::endl;
        return false;
    }

    // 设置无流控制
    result = sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
    if (result != SP_OK) {
        std::cerr << "无法设置串口屏流控制" << std::endl;
        return false;
    }

    std::cout << "成功打开串口屏串口: " << port_name << " 波特率: " << baud_rate 
              << " 数据位: 8 停止位: 1 校验位: 无 流控制: 无 (读写模式)" << std::endl;
    return true;
}

void SerialScreenProtocol::close() {
    if (port) {
        sp_close(port);
        sp_free_port(port);
        port = nullptr;
    }
}

void SerialScreenProtocol::sendFloat(const std::string& name, float value) {
    char cmd[50];
    char floatStr[10];
    
    // 使用snprintf确保格式与C代码完全一致
    snprintf(floatStr, sizeof(floatStr), "%.3f", value);    // 三位小数
    snprintf(cmd, sizeof(cmd), "%s=\"%s\"", name.c_str(), floatStr);  // 格式如t5.txt="2.999"，没有空格
    
    sendCmd(cmd);
}

void SerialScreenProtocol::sendCmd(const std::string& cmd) {
    if (!port) {
        return; // 串口未打开就直接返回，不报错
    }

    // 使用非阻塞方式发送命令字符串
    sp_nonblocking_write(port, 
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());

    // 使用非阻塞方式发送结束符 0xFF 0xFF 0xFF
    uint8_t endCmd[3] = {0xFF, 0xFF, 0xFF};
    sp_nonblocking_write(port, endCmd, 3);

    // 减少调试输出，只在重要数据更新时显示
    static std::string last_cmd = "";
    if (cmd != last_cmd) {
        std::cout << "发送串口屏命令: " << cmd << std::endl;
        last_cmd = cmd;
    }
}

void SerialScreenProtocol::updateCurrentPower(float current, float power) {
    std::lock_guard<std::mutex> lock(data_mutex);
    current_I = current;
    power_P = power;
    
    // 更新最大功率（修复比较逻辑）
    if (power > max_power) {
        max_power = power;
        std::cout << "*** 更新最大功率: " << std::fixed << std::setprecision(3) << max_power << " W ***" << std::endl;
    }
    
    data_updated = true;
}

void SerialScreenProtocol::updateMaxPower(float max_power) {
    std::lock_guard<std::mutex> lock(data_mutex);
        this->max_power = max_power;
    data_updated = true;
}

void SerialScreenProtocol::setStartButtonCallback(std::function<void()> callback) {
    startButtonCallback = callback;
}

void SerialScreenProtocol::notifyStartButtonPressed() {
    std::lock_guard<std::mutex> lock(data_mutex);
    start_received = true;
    std::cout << "*** 收到start按键通知，将发送距离和边长数据 ***" << std::endl;
}

void SerialScreenProtocol::registerEventCallback(SerialScreenEvent event, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(data_mutex);
    eventCallbacks[event] = callback;
    std::cout << "注册事件回调: " << static_cast<int>(event) << std::endl;
}

void SerialScreenProtocol::unregisterEventCallback(SerialScreenEvent event) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = eventCallbacks.find(event);
    if (it != eventCallbacks.end()) {
        eventCallbacks.erase(it);
        std::cout << "注销事件回调: " << static_cast<int>(event) << std::endl;
    }
}

void SerialScreenProtocol::clearAllEventCallbacks() {
    std::lock_guard<std::mutex> lock(data_mutex);
    eventCallbacks.clear();
    std::cout << "清除所有事件回调" << std::endl;
}

void SerialScreenProtocol::triggerEventCallback(SerialScreenEvent event) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = eventCallbacks.find(event);
    if (it != eventCallbacks.end() && it->second) {
        std::cout << "触发事件回调: " << static_cast<int>(event) << std::endl;
        it->second(); // 调用回调函数
    }
}

SerialScreenEvent SerialScreenProtocol::parseEvent(uint8_t page, uint8_t control, uint8_t event) {
    if (page == 0x01 && control == 0x02) {
        return SerialScreenEvent::START_BUTTON;
    } else if (page == 0x02) {
        switch (control) {
            case 0x02: return SerialScreenEvent::KEYBOARD_0;
            case 0x05: return SerialScreenEvent::KEYBOARD_1;
            case 0x06: return SerialScreenEvent::KEYBOARD_2;
            case 0x07: return SerialScreenEvent::KEYBOARD_3;
            case 0x08: return SerialScreenEvent::KEYBOARD_4;
            case 0x09: return SerialScreenEvent::KEYBOARD_5;
            case 0x0A: return SerialScreenEvent::KEYBOARD_6;
            case 0x0B: return SerialScreenEvent::KEYBOARD_7;
            case 0x0C: return SerialScreenEvent::KEYBOARD_8;
            case 0x0E: return SerialScreenEvent::KEYBOARD_9;
            case 0x0D: return SerialScreenEvent::DELETE_BUTTON;
        }
    } else if (page == 0x04) {
        switch (control) {
            case 0x02: return SerialScreenEvent::CAMERA_EXPOSURE_PLUS_1;
            case 0x04: return SerialScreenEvent::CAMERA_EXPOSURE_PLUS_10;
            case 0x05: return SerialScreenEvent::CAMERA_EXPOSURE_PLUS_100;
            case 0x06: return SerialScreenEvent::CAMERA_EXPOSURE_PLUS_1000;
            case 0x07: return SerialScreenEvent::CAMERA_EXPOSURE_MINUS_1;
            case 0x08: return SerialScreenEvent::CAMERA_EXPOSURE_MINUS_10;
            case 0x09: return SerialScreenEvent::CAMERA_EXPOSURE_MINUS_100;
            case 0x0A: return SerialScreenEvent::CAMERA_EXPOSURE_MINUS_1000;
        }
    } else if (page == 0x05) {
        switch (control) {
            case 0x02: return SerialScreenEvent::CAMERA_THRESHOLD_PLUS_1;
            case 0x04: return SerialScreenEvent::CAMERA_THRESHOLD_PLUS_10;
            case 0x05: return SerialScreenEvent::CAMERA_THRESHOLD_PLUS_100;
            case 0x06: return SerialScreenEvent::CAMERA_THRESHOLD_PLUS_1000;
            case 0x07: return SerialScreenEvent::CAMERA_THRESHOLD_MINUS_1;
            case 0x08: return SerialScreenEvent::CAMERA_THRESHOLD_MINUS_10;
            case 0x09: return SerialScreenEvent::CAMERA_THRESHOLD_MINUS_100;
            case 0x0A: return SerialScreenEvent::CAMERA_THRESHOLD_MINUS_1000;
        }
    }
    
    return SerialScreenEvent::UNKNOWN_EVENT;
}

void SerialScreenProtocol::start() {
    if (running) return;
    
    running = true;
    send_thread = std::thread(&SerialScreenProtocol::sendThreadFunc, this);
    std::cout << "串口屏发送线程已启动" << std::endl;
}

void SerialScreenProtocol::stop() {
    if (!running) return;
    
    running = false;
    if (send_thread.joinable()) {
        send_thread.join();
    }
    std::cout << "串口屏发送线程已停止" << std::endl;
}

void SerialScreenProtocol::sendThreadFunc() {
    while (running) {
        bool should_send_current_power = false;
        bool should_send_distance_side = false;
        
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (data_updated) {
                should_send_current_power = true;
                data_updated = false;
            }
            if (start_received) {
                should_send_distance_side = true;
                start_received = false;
            }
        }
        
        // 发送电流和功率（只要收到就发送）
        if (should_send_current_power) {
            sendCurrentAndPower();
        }
        
        // 发送距离和边长（只有收到start才发送）
        if (should_send_distance_side) {
            sendDistanceAndSideLength();
        }
        
        // 持续发送最大功率
        sendMaxPower();
        
        // 尝试接收串口屏按键事件
        if (port) {
            // 读取一个字节来判断是否有数据
            uint8_t first_byte;
            size_t bytes_read = sp_nonblocking_read(port, &first_byte, 1);
            
            if (bytes_read > 0 && first_byte == 0x65) {
                // 检测到串口屏协议帧头，读取完整帧
                std::vector<uint8_t> frame_data;
                frame_data.push_back(first_byte);
                
                // 读取剩余6字节
                uint8_t buffer[6];
                bytes_read = sp_blocking_read(port, buffer, 6, 100);
                
                if (bytes_read == 6) {
                    // 添加剩余数据到frame_data
                    frame_data.insert(frame_data.end(), buffer, buffer + 6);
                    
                    // 解析数据帧
                    parseFrame(frame_data);
                }
            }
        }
        
        // 短暂延时，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void SerialScreenProtocol::sendDistanceAndSideLength() {
    // 发送距离D (只有收到start才发送)
    sendFloat("t0.txt", distance_D);
    
    // 发送边长x (只有收到start才发送)
    sendFloat("t1.txt", side_length_x);
}

void SerialScreenProtocol::sendCurrentAndPower() {
    // 发送电流I (只要收到就发送)
    sendFloat("t2.txt", current_I);
    
    // 发送功率P (只要收到就发送)
    sendFloat("t3.txt", power_P);
}

void SerialScreenProtocol::sendMaxPower() {
    // 发送最大功率 (持续发送)
    sendFloat("t4.txt", max_power);
}

void SerialScreenProtocol::sendAllData() {
    // 这个方法保留用于兼容性，但主要使用上面的分离方法
    sendDistanceAndSideLength();
    sendCurrentAndPower();
    sendMaxPower();
}

// 以下是接收解析相关的方法（根据通信.csv协议格式）
bool SerialScreenProtocol::parseFrame(const std::vector<uint8_t>& frame_data) {
    if (!isValidFrame(frame_data)) {
        return false;
    }

    // 根据通信.csv协议格式解析
    uint8_t page = frame_data[1];      // 页面
    uint8_t control = frame_data[2];   // 控件
    uint8_t event = frame_data[3];     // 事件

    // 打印结果
    std::cout << "\n=== 串口屏协议数据帧解析结果 ===" << std::endl;
    std::cout << "页面: 0x" << std::hex << std::setfill('0') << std::setw(2) 
              << static_cast<int>(page) << std::dec << std::endl;
    std::cout << "控件: 0x" << std::hex << std::setfill('0') << std::setw(2) 
              << static_cast<int>(control) << std::dec << std::endl;
    std::cout << "事件: 0x" << std::hex << std::setfill('0') << std::setw(2) 
              << static_cast<int>(event) << std::dec << std::endl;
    
    // 使用新的解析方法获取事件类型
    SerialScreenEvent screenEvent = parseEvent(page, control, event);
    
    // 根据事件类型获取功能名称（用于显示）
    std::string function_name = "未知功能";
    bool is_start_button = false;
    
    switch (screenEvent) {
        case SerialScreenEvent::START_BUTTON:
            function_name = "start按键";
            is_start_button = true;
            break;
        case SerialScreenEvent::KEYBOARD_0: function_name = "键盘0"; break;
        case SerialScreenEvent::KEYBOARD_1: function_name = "键盘1"; break;
        case SerialScreenEvent::KEYBOARD_2: function_name = "键盘2"; break;
        case SerialScreenEvent::KEYBOARD_3: function_name = "键盘3"; break;
        case SerialScreenEvent::KEYBOARD_4: function_name = "键盘4"; break;
        case SerialScreenEvent::KEYBOARD_5: function_name = "键盘5"; break;
        case SerialScreenEvent::KEYBOARD_6: function_name = "键盘6"; break;
        case SerialScreenEvent::KEYBOARD_7: function_name = "键盘7"; break;
        case SerialScreenEvent::KEYBOARD_8: function_name = "键盘8"; break;
        case SerialScreenEvent::KEYBOARD_9: function_name = "键盘9"; break;
        case SerialScreenEvent::DELETE_BUTTON: function_name = "delete按键"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_PLUS_1: function_name = "摄像头曝光+1"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_PLUS_10: function_name = "摄像头曝光+10"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_PLUS_100: function_name = "摄像头曝光+100"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_PLUS_1000: function_name = "摄像头曝光+1000"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_MINUS_1: function_name = "摄像头曝光-1"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_MINUS_10: function_name = "摄像头曝光-10"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_MINUS_100: function_name = "摄像头曝光-100"; break;
        case SerialScreenEvent::CAMERA_EXPOSURE_MINUS_1000: function_name = "摄像头曝光-1000"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_PLUS_1: function_name = "相机阈值+1"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_PLUS_10: function_name = "相机阈值+10"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_PLUS_100: function_name = "相机阈值+100"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_PLUS_1000: function_name = "相机阈值+1000"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_MINUS_1: function_name = "相机阈值-1"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_MINUS_10: function_name = "相机阈值-10"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_MINUS_100: function_name = "相机阈值-100"; break;
        case SerialScreenEvent::CAMERA_THRESHOLD_MINUS_1000: function_name = "相机阈值-1000"; break;
        case SerialScreenEvent::UNKNOWN_EVENT: function_name = "未知功能"; break;
    }
    
    std::cout << "功能: " << function_name << std::endl;
    
    // 触发通用事件回调
    triggerEventCallback(screenEvent);
    
    // 如果是start按键，设置标志并调用旧的回调（保持向后兼容）
    if (is_start_button) {
        std::lock_guard<std::mutex> lock(data_mutex);
        start_received = true;
        std::cout << "*** 检测到start按键，将发送距离和边长数据 ***" << std::endl;
        
        // 调用旧的回调函数通知其他实例（保持向后兼容）
        if (startButtonCallback) {
            startButtonCallback();
        }
    }
    
    // 打印原始数据
    std::cout << "原始数据: ";
    for (size_t i = 0; i < frame_data.size(); ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(frame_data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
    std::cout << "================================\n" << std::endl;
    
    return true;
}

bool SerialScreenProtocol::isValidFrame(const std::vector<uint8_t>& frame_data) {
    // 根据通信.csv协议格式验证
    if (frame_data.size() != 7) {  // 帧头(1) + 页面(1) + 控件(1) + 事件(1) + 帧尾(3)
        return false;
    }

    // 验证帧头
    if (frame_data[0] != 0x65) {
        return false;
    }

    // 验证帧尾 (三个0xFF)
    if (frame_data[4] != 0xFF || frame_data[5] != 0xFF || frame_data[6] != 0xFF) {
        return false;
    }

    return true;
}

size_t SerialScreenProtocol::getFrameSize() const {
    return 7; // 串口屏协议：0x65 + 页面 + 控件 + 事件 + 0xFF + 0xFF + 0xFF
}

std::string SerialScreenProtocol::getProtocolName() const {
    return "串口屏协议";
}

bool SerialScreenProtocol::findFrameHeader(struct sp_port* port) {
    uint8_t buffer[1];
    size_t bytes_read;
    
    do {
        bytes_read = sp_blocking_read(port, buffer, 1, 10); // 减少超时时间到10ms
        if (bytes_read == 0) {
            return false;
        }
    } while (buffer[0] != 0x65);

    return true;
} 