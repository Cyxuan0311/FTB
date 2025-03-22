#!/bin/bash
# uninstall.sh
# 位于 bash_tool 目录中，用于调用构建目录中的 cmake_uninstall.cmake 卸载所有安装的文件。
#
# 使用方法：
#   ./uninstall.sh [build_directory]
# 如果不指定 build_directory，则默认使用 ../build

set -e

# 默认构建目录（相对于 bash_tool 目录）
BUILD_DIR=${1:-../build}

echo "使用的构建目录: $BUILD_DIR"

if [ ! -d "$BUILD_DIR" ]; then
    echo "错误: 构建目录 '$BUILD_DIR' 不存在。"
    exit 1
fi

UNINSTALL_SCRIPT="${BUILD_DIR}/cmake_uninstall.cmake"

if [ ! -f "$UNINSTALL_SCRIPT" ]; then
  echo "错误：未找到 ${UNINSTALL_SCRIPT} 文件。"
  echo "请确保在构建目录中已经生成该文件（运行 cmake 配置阶段时生成卸载脚本模板）。"
  exit 1
fi

echo "开始卸载安装的文件..."
cd "$BUILD_DIR"
# 使用 sudo 提升权限运行 cmake 卸载脚本
sudo cmake -P cmake_uninstall.cmake
echo "卸载完成。"
