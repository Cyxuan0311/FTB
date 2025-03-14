# FTB(terminal file browser) - 终端文件浏览器

![C++17](https://img.shields.io/badge/C++-17-blue) ![FTXUI](https://img.shields.io/badge/FTXUI-5.0.0-orange)


![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey) ![Tool](https://img.shields.io/badge/CMake-3.20.0-red)


基于FTXUI库开发的终端交互式文件浏览器，提供直观的目录导航和文件查看体验。

## 效果展示

![效果](https://yt3.ggpht.com/iHL64dUd3WQpbat--V-mzE1PKBu6CLeUyliucuFYF2J8oSZXk3Fn2-aS2v0aQBdrd4CwjP8YWeAh=s1600-rw-nd-v1)


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
git clone https://github.com/Cyxuan0311/FTB.git
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
- Ctrl+f键 新建文件
- Ctrl+k键新建文件夹
- Delete键 删除文件夹或文件

## 项目结构

    FileBrowser/
    ├── include/            # 头文件
    │   |── FileManager.hpp
    |   |—— ThreadGuard.hpp
    |   └—— UIManager.hpp
    ├── src/               # 源代码
    │   ├── FileManager.cpp
    │   |── main.cpp
    |   |—— ThreadGuard.cpp
    |   └—— UIManager.cpp
    ├── CMakeLists.txt     # 构建配置
    |── README.md          # 说明文档
    |—— .gitignore
    |—— build.sh
    |—— .clang-format      # 格式化文件
    |__ License

## 开发环境

- 编译器：g++ 11.0+
- 构建工具：CMake 3.20+
- 依赖库：FTXUI 5.0+

## 许可协议

**MIT License**

