# FTB - 终端文件浏览器

![C++17](https://img.shields.io/badge/C++-17-blue) ![FTXUI](https://img.shields.io/badge/FTXUI-30-orange)


![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey)


基于FTXUI库开发的终端交互式文件浏览器，提供直观的目录导航和文件查看体验。

## 功能特性

- 🗂️ 实时目录内容展示A
- 🎨 彩色显示（文件夹蓝色/文件红色）
- ⏰ 顶部状态栏显示当前时间
- 📁 路径历史栈（支持返回上级目录）
- ↕️ 键盘导航（↑↓键选择，Enter进入目录）
- 🔄 自动刷新（每100ms更新界面）
- 🔎 文件夹搜索
- 🧑‍🎓 计算当前选中文件或文件夹的内存占比大小
- 🛠️ 新建文件或文件夹，删除功能。


## 安装依赖

```bash
# 安装FTXUI库
sudo apt-get install libftxui-dev
```

## 编译运行
```bash
# 克隆仓库
git clone https://github.com/yourusername/file-browser.git
cd file-browser

# 编译项目
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/ftxui
make

# 运行程序
./bin/FTB
```

## 使用说明 
- ↑/↓:导航文件列表
- Enter:进入选中目录
- Backspace:返回上级目录
- ESC:退出程序
- 搜索框:输入关键字，查询指定文件
- K键新建文件
- F键新建文件夹
- D键删除文件或文件夹

## 项目结构

    FileBrowser/
    ├── include/            # 头文件
    │   |── file_browser.hpp
    |   |—— file_operations.hpp
    |   |—— new_file_dialog.hpp
    |   └—— thread_guard.hpp
    ├── src/               # 源代码
    │   ├── file_browser.cpp
    │   |── main.cpp
    |   |—— file_operations.cpp
    |   |—— new_file_dialog.cpp
    |   └—— thread_guard.cpp
    ├── CMakeLists.txt     # 构建配置
    |── README.md          # 说明文档
    |—— .gitignore
    |—— build.sh
    |__ License

## 开发环境

- 编译器：g++ 9.4+
- 构建工具：CMake 3.16+
- 依赖库：FTXUI 3.0+

## 许可协议

**MIT License**

