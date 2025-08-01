#ifndef SERIAL_SCREEN_PROTOCOL_H
#define SERIAL_SCREEN_PROTOCOL_H

#include "protocol.h"
#include <libserialport.h>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

// 串口屏按键事件枚举
enum class SerialScreenEvent {
    START_BUTTON,           // start按键
    KEYBOARD_0,            // 键盘0
    KEYBOARD_1,            // 键盘1
    KEYBOARD_2,            // 键盘2
    KEYBOARD_3,            // 键盘3
    KEYBOARD_4,            // 键盘4
    KEYBOARD_5,            // 键盘5
    KEYBOARD_6,            // 键盘6
    KEYBOARD_7,            // 键盘7
    KEYBOARD_8,            // 键盘8
    KEYBOARD_9,            // 键盘9
    DELETE_BUTTON,         // delete按键
    CAMERA_EXPOSURE_PLUS_1,    // 摄像头曝光+1
    CAMERA_EXPOSURE_PLUS_10,   // 摄像头曝光+10
    CAMERA_EXPOSURE_PLUS_100,  // 摄像头曝光+100
    CAMERA_EXPOSURE_PLUS_1000, // 摄像头曝光+1000
    CAMERA_EXPOSURE_MINUS_1,   // 摄像头曝光-1
    CAMERA_EXPOSURE_MINUS_10,  // 摄像头曝光-10
    CAMERA_EXPOSURE_MINUS_100, // 摄像头曝光-100
    CAMERA_EXPOSURE_MINUS_1000,// 摄像头曝光-1000
    CAMERA_THRESHOLD_PLUS_1,   // 相机阈值+1
    CAMERA_THRESHOLD_PLUS_10,  // 相机阈值+10
    CAMERA_THRESHOLD_PLUS_100, // 相机阈值+100
    CAMERA_THRESHOLD_PLUS_1000,// 相机阈值+1000
    CAMERA_THRESHOLD_MINUS_1,  // 相机阈值-1
    CAMERA_THRESHOLD_MINUS_10, // 相机阈值-10
    CAMERA_THRESHOLD_MINUS_100,// 相机阈值-100
    CAMERA_THRESHOLD_MINUS_1000,// 相机阈值-1000
    UNKNOWN_EVENT            // 未知事件
};

// 串口屏协议类
class SerialScreenProtocol : public Protocol {
private:
    std::string port_name;
    int baud_rate;
    struct sp_port* port;
    std::mutex data_mutex;
    
    // 数据变量
    float distance_D;
    float side_length_x;
    float current_I;
    float power_P;
    float max_power;
    
    // 控制标志
    bool start_received;
    bool data_updated;
    
    // 回调函数
    std::function<void()> startButtonCallback;
    
    // 通用事件回调注册表
    std::unordered_map<SerialScreenEvent, std::function<void()>> eventCallbacks;

public:
    SerialScreenProtocol(const std::string& port_name, int baud_rate = 9600);
    ~SerialScreenProtocol();
    
    bool parseFrame(const std::vector<uint8_t>& frame_data) override;
    bool isValidFrame(const std::vector<uint8_t>& frame_data) override;
    size_t getFrameSize() const override;
    std::string getProtocolName() const override;
    bool findFrameHeader(struct sp_port* port);
    
    // 串口屏发送功能
    bool open();
    void close();
    void sendFloat(const std::string& name, float value);
    void sendCmd(const std::string& cmd);
    
    // 数据更新接口
    void updateCurrentPower(float current, float power);
    void updateMaxPower(float max_power);
    
    // 立即发送接口
    void sendDistanceAndSideLengthImmediately();
    
    // 单线程支持接口
    void checkForSerialScreenData();
    void sendPeriodicData();
    
    // 回调设置接口
    void setStartButtonCallback(std::function<void()> callback);
    void notifyStartButtonPressed();
    
    // 通用事件回调注册接口
    void registerEventCallback(SerialScreenEvent event, std::function<void()> callback);
    void unregisterEventCallback(SerialScreenEvent event);
    void clearAllEventCallbacks();
    
private:
    void sendAllData();
    void sendDistanceAndSideLength();
    void sendCurrentAndPower();
    void sendMaxPower();
    
    // 内部辅助方法
    SerialScreenEvent parseEvent(uint8_t page, uint8_t control, uint8_t event);
    void triggerEventCallback(SerialScreenEvent event);
};

#endif // SERIAL_SCREEN_PROTOCOL_H 