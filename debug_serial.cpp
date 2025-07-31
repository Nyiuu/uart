#include <libserialport.h>
#include <iostream>
#include <iomanip>
#include <vector>

int main() {
    std::cout << "=== 串口调试程序 ===" << std::endl;
    
    struct sp_port* port;
    int result = sp_get_port_by_name("/dev/ttyUSB1", &port);
    if (result != SP_OK) {
        std::cerr << "无法获取串口" << std::endl;
        return -1;
    }
    
    result = sp_open(port, SP_MODE_READ);
    if (result != SP_OK) {
        std::cerr << "无法打开串口" << std::endl;
        sp_free_port(port);
        return -1;
    }
    
    // 设置串口参数
    sp_set_baudrate(port, 9600);
    sp_set_bits(port, 8);
    sp_set_stopbits(port, 1);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
    
    std::cout << "开始读取串口数据，按Ctrl+C停止..." << std::endl;
    std::cout << "等待串口屏按键事件..." << std::endl;
    
    while (true) {
        // 读取第一个字节
        uint8_t first_byte;
        size_t bytes_read = sp_blocking_read(port, &first_byte, 1, 1000);
        
        if (bytes_read == 0) {
            continue; // 没有数据
        }
        
        std::cout << "收到第一个字节: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(first_byte) << std::dec << std::endl;
        
        // 如果是0x65，尝试读取完整的串口屏协议
        if (first_byte == 0x65) {
            std::cout << "检测到串口屏协议帧头 0x65" << std::endl;
            
            // 读取剩余5个字节
            uint8_t buffer[5];
            bytes_read = sp_blocking_read(port, buffer, 5, 1000);
            
            if (bytes_read == 5) {
                std::cout << "成功读取完整帧:" << std::endl;
                std::cout << "  帧头: 0x65" << std::endl;
                std::cout << "  页面: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                          << static_cast<int>(buffer[0]) << std::dec << std::endl;
                std::cout << "  控件: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                          << static_cast<int>(buffer[1]) << std::dec << std::endl;
                std::cout << "  事件: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                          << static_cast<int>(buffer[2]) << std::dec << std::endl;
                std::cout << "  结束符: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                          << static_cast<int>(buffer[3]) << " 0x" << std::setw(2) 
                          << static_cast<int>(buffer[4]) << std::dec << std::endl;
                
                // 验证结束符
                if (buffer[3] == 0xFF && buffer[4] == 0xFF) {
                    std::cout << "  ✓ 结束符验证通过" << std::endl;
                    
                    // 识别功能
                    if (buffer[0] == 0x01 && buffer[1] == 0x02) {
                        std::cout << "  ✓ 检测到start按键事件" << std::endl;
                    }
                } else {
                    std::cout << "  ✗ 结束符验证失败" << std::endl;
                }
                
                std::cout << "----------------------------------------" << std::endl;
            } else {
                std::cout << "读取剩余字节失败，只读到 " << bytes_read << " 字节" << std::endl;
            }
        } else {
            std::cout << "未知协议帧头: 0x" << std::hex << std::setfill('0') << std::setw(2) 
                      << static_cast<int>(first_byte) << std::dec << std::endl;
        }
    }
    
    sp_close(port);
    sp_free_port(port);
    return 0;
} 