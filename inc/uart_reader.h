#ifndef UART_READER_H
#define UART_READER_H

#include "protocol.h"
#include <libserialport.h>
#include <string>
#include <vector>
#include <memory>

// 串口读取器类
class UartReader {
private:
    std::string port_name;
    int baud_rate;
    struct sp_port* port;
    std::vector<std::unique_ptr<Protocol>> protocols;

    bool tryReadCurrentPowerFrame(uint8_t first_byte, uint8_t second_byte);
    bool tryReadSerialScreenFrame(uint8_t first_byte);

public:
    UartReader(const std::string& port_name, int baud_rate = 9600);
    ~UartReader();

    void addProtocol(std::unique_ptr<Protocol> protocol);
    bool open();
    bool readAndParseFrame();
    std::string getPortName() const { return port_name; }
};

#endif // UART_READER_H 