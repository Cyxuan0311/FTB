#!/bin/bash

# 清理旧构建目录
echo "正在清理旧构建文件..."
rm -rf build

# 创建构建目录
echo "创建构建目录..."
mkdir -p build && cd build || exit

# 生成构建系统文件
echo "正在生成构建系统..."
cmake .. || { echo "CMake配置失败!"; exit 1; }

# 编译项目
echo "正在编译项目..."
make -j$(nproc) || { echo "编译失败！"; exit 1; }

echo "构建成功!可执行文件位于:build/${PROJECT_NAME}"