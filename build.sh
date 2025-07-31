#!/bin/bash

echo "=== 编译串口通讯程序 ==="

# 创建构建目录
mkdir -p build
cd build

# 运行CMake配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

if [ $? -eq 0 ]; then
    ./uart_program
else
    echo "编译失败！"
    exit 1
fi