#!/bin/bash

set -e  # 遇到错误立即退出

PROJECT_NAME="FTB"  # 请根据你的项目名称修改

# 获取脚本所在目录，并回到项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.." || exit 1

# 清理旧构建目录
echo "[Step 1] 清理旧构建文件..."
rm -rf build

# 创建构建目录
echo "[Step 2] 创建构建目录..."
mkdir -p build && cd build || exit 1

# 生成构建系统文件
echo "[Step 3] 生成构建系统..."
cmake .. || { echo "错误: CMake 配置失败!"; exit 1; }

# 编译项目
echo "[Step 4] 编译项目..."
make -j$(nproc) || { echo "错误: 编译失败！"; exit 1; }

echo "构建成功! 可执行文件位于: build/${PROJECT_NAME}"

# 询问是否安装
read -p "是否要安装程序? (y/n): " install_answer
if [[ "$install_answer" =~ ^[Yy]$ ]]; then
    echo "[Step 5] 安装程序..."
    sudo make install || { echo "错误: 安装失败！"; exit 1; }
    echo "安装成功! 你可以使用 '${PROJECT_NAME}' 命令运行程序。"
fi

# 询问是否运行程序
read -p "是否要运行程序? (y/n): " run_answer
if [[ "$run_answer" =~ ^[Yy]$ ]]; then
    echo "[Step 6] 运行程序..."
    if ! ./${PROJECT_NAME}; then
        echo "错误: 运行失败！是否要调试程序？(y/n)"
        read -p "> " debug_answer
        if [[ "$debug_answer" =~ ^[Yy]$ ]]; then
            gdb ./${PROJECT_NAME}
        fi
    fi
fi

# 询问是否跳转到项目根目录
read -p "是否要跳转到项目根目录? (y/n): " cd_answer
if [[ "$cd_answer" =~ ^[Yy]$ ]]; then
    cd ..  # 返回项目根目录
    echo "已返回项目根目录。"
else
    echo "保持在 build 目录中。"
fi
