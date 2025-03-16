#!/bin/bash

PROJECT_NAME="FTB"  # 请根据你的项目名称修改

# 清理旧构建目录
echo "🛠 正在清理旧构建文件..."
rm -rf build

# 创建构建目录
echo "📂 创建构建目录..."
mkdir -p build && cd build || exit 1

# 生成构建系统文件
echo "⚙️ 正在生成构建系统..."
cmake .. || { echo "❌ CMake 配置失败!"; exit 1; }

# 编译项目
echo "🚀 正在编译项目..."
make -j$(nproc) || { echo "❌ 编译失败！"; exit 1; }

echo "✅ 构建成功! 可执行文件位于: build/${PROJECT_NAME}"

# 询问是否安装
read -p "是否要安装程序? (y/n): " install_answer
if [[ "$install_answer" =~ ^[Yy]$ ]]; then
    echo "📦 正在安装..."
    sudo make install || { echo "❌ 安装失败！"; exit 1; }
    echo "✅ 安装成功! 你可以使用 '${PROJECT_NAME}' 命令运行程序。"
fi

# 询问是否运行程序
read -p "是否要运行程序? (y/n): " run_answer
if [[ "$run_answer" =~ ^[Yy]$ ]]; then
    echo "🚀 运行程序..."
    ${PROJECT_NAME} || { echo "❌ 运行失败！"; exit 1; }
fi

# 询问是否跳转到项目根目录
read -p "是否要跳转到项目根目录? (y/n): " cd_answer
if [[ "$cd_answer" =~ ^[Yy]$ ]]; then
    cd ..  # 返回项目根目录
    echo "📂 已返回项目根目录。"
else
    echo "📂 保持在 build 目录中。"
fi
