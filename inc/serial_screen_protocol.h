#ifndef SERIAL_SCREEN_PROTOCOL_H
#define SERIAL_SCREEN_PROTOCOL_H

#include "protocol.h"
#include <libserialport.h>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <functional>

// 串口屏协议类
class SerialScreenProtocol : public Protocol {
private:
    std::string port_name;
    int baud_rate;
    struct sp_port* port;
    std::thread send_thread;
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
    
    // 线程控制
    bool running;
    
    // 回调函数
    std::function<void()> startButtonCallback;

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
    
    // 回调设置接口
    void setStartButtonCallback(std::function<void()> callback);
    void notifyStartButtonPressed();
    
    // 线程控制
    void start();
    void stop();
    
private:
    void sendThreadFunc();
    void sendAllData();
    void sendDistanceAndSideLength();
    void sendCurrentAndPower();
    void sendMaxPower();
};

#endif // SERIAL_SCREEN_PROTOCOL_H 