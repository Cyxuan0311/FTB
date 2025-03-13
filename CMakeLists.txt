cmake_minimum_required(VERSION 3.10)
project(FTB)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# 添加编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")

# 设置包含目录
include_directories(include)

# 查找依赖库
find_package(ftxui CONFIG REQUIRED)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 构建核心库
add_library(FTB_core
    src/FileManager.cpp
    src/ThreadGuard.cpp
    src/UIManager.cpp
)

# 链接UI库
target_link_libraries(FTB_core PRIVATE ftxui::screen ftxui::dom ftxui::component)

# 构建主程序
add_executable(FTB src/main.cpp)
target_link_libraries(FTB PRIVATE FTB_core)

# 假设的测试源文件列表，你需要根据实际情况修改
set(TEST_SOURCES
    tests/UIManagerTest.cpp
)

# 构建测试程序
add_executable(FTB_tests ${TEST_SOURCES})

# 链接测试依赖
target_link_libraries(FTB_tests
    PRIVATE gtest
    PRIVATE gtest_main
    PRIVATE ${GTEST_BOTH_LIBRARIES}
    PRIVATE pthread
    PRIVATE FTB_core
)

# 启用测试
enable_testing()
add_test(NAME FTB_tests COMMAND FTB_tests)