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

PROJECT_NAME="FTB"  # 假设项目名是FTB，你可以根据实际情况修改
echo "构建成功! 可执行文件位于: build/${PROJECT_NAME}"

# 询问是否跳转到build目录（当前已在build目录中）
read -p "是否要跳转到项目根目录? (y/n): " answer
case $answer in
    [Yy]* )
        cd ..  # 返回项目根目录
        ;;
    [Nn]* )
        echo "保持在build目录中。"
        ;;
    * )
        echo "无效输入，请输入 y 或 n。"
        ;;
esac