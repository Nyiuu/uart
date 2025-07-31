#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <string>

// 协议基类
class Protocol {
public:
    virtual ~Protocol() = default;
    virtual bool parseFrame(const std::vector<uint8_t>& frame_data) = 0;
    virtual bool isValidFrame(const std::vector<uint8_t>& frame_data) = 0;
    virtual size_t getFrameSize() const = 0;
    virtual std::string getProtocolName() const = 0;
};

#endif // PROTOCOL_H 