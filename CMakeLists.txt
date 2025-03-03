cmake_minimum_required(VERSION 3.10)
# 设置项目名称
project(FTB)
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
# 添加头文件搜索路径
include_directories(include)
# 查找 FTXUI 库
find_package(ftxui CONFIG REQUIRED)
# 添加源文件
add_executable(FTB
    src/main.cpp
    src/file_browser.cpp
    src/thread_guard.cpp
    src/file_operations.cpp
    src/new_file_dialog.cpp
)
# 链接 FTXUI 库
target_link_libraries(FTB
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)