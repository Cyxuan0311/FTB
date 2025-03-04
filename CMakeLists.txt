cmake_minimum_required(VERSION 3.10)
# 设置项目名称
project(FTB)
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
# 添加头文件搜索路径
include_directories(include)
# 查找 FTXUI 库
find_package(ftxui CONFIG REQUIRED)
# 更改一下文件添加方式
file(GLOB SOURCES "src/*.cpp")
# 添加源文件
add_executable(FTB
    ${SOURCES}
)
# 链接 FTXUI 库
target_link_libraries(FTB
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)