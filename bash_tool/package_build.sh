#!/bin/bash

set -e  # 遇到错误立即退出

# 获取脚本所在目录，并回到项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.." || exit 1  # 进入项目根目录

# 变量定义
BUILD_DIR="build"
OUTPUT_TAR="${1:-project_build.tar.bz2}"  # 允许自定义 tar 包名，默认 "project_build.tar.bz2"

echo "[Step 1] 搜索目标文件..."

# 1. 确保 build 目录存在
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "错误: 目录 '$BUILD_DIR' 不存在！请先构建项目。"
    exit 1
fi

# 2. 搜索所有可执行文件、共享库（.so）、静态库（.a）
TARGET_FILES=()
while IFS= read -r -d '' file; do
    TARGET_FILES+=("$file")
done < <(find "$BUILD_DIR" -type f \( -executable -o -name "*.so*" -o -name "*.a" \) -print0)

# 3. 检查是否找到目标文件
if [[ ${#TARGET_FILES[@]} -eq 0 ]]; then
    echo "错误: 未找到可打包的文件，终止打包。"
    exit 1
fi

# 4. 生成 tar 压缩包
echo "[Step 2] 生成 $OUTPUT_TAR..."
tar -cjf "$OUTPUT_TAR" "${TARGET_FILES[@]}"

# 5. 结束
echo "打包完成，已生成 $OUTPUT_TAR，包含以下文件:"
printf '   - %s\n' "${TARGET_FILES[@]}"
