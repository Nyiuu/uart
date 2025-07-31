#ifndef CURRENT_POWER_PROTOCOL_H
#define CURRENT_POWER_PROTOCOL_H

#include "protocol.h"
#include <libserialport.h>
#include <functional>

// 电流功率协议类
class CurrentPowerProtocol : public Protocol {
private:
    static const size_t FRAME_SIZE = 20; // 帧头(2) + 数据(16) + 帧尾(2)
    
    // 回调函数类型
    std::function<void(float, float)> currentPowerCallback;

public:
    CurrentPowerProtocol();
    
    bool parseFrame(const std::vector<uint8_t>& frame_data) override;
    bool isValidFrame(const std::vector<uint8_t>& frame_data) override;
    size_t getFrameSize() const override;
    std::string getProtocolName() const override;
    bool findFrameHeader(struct sp_port* port);
    
    // 设置回调函数
    void setCurrentPowerCallback(std::function<void(float, float)> callback);
};

#endif // CURRENT_POWER_PROTOCOL_H 