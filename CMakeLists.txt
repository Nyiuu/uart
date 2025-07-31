cmake_minimum_required(VERSION 3.10)
project(uart_program)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找libserialport
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSERIALPORT REQUIRED libserialport)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${LIBSERIALPORT_INCLUDE_DIRS})

# 添加可执行文件
add_executable(uart_program
    src/main.cpp
    src/uart_reader.cpp
    src/current_power_protocol.cpp
    src/serial_screen_protocol.cpp
)

# 链接库
target_link_libraries(uart_program ${LIBSERIALPORT_LIBRARIES})

# 设置编译选项
target_compile_options(uart_program PRIVATE ${LIBSERIALPORT_CFLAGS_OTHER} -Wall -Wextra) 