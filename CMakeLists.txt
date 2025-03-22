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

# 构建核心库
add_library(FTB_core
    src/FileManager.cpp
    src/ThreadGuard.cpp
    src/UIManager.cpp
    src/FileSizeCalculator.cpp
    src/DirectoryHistory.cpp
    src/Vim_Like.cpp
    src/detail_element.cpp
)

# 链接 FTXUI 库
target_link_libraries(FTB_core PRIVATE ftxui::screen ftxui::dom ftxui::component)

# 构建主程序
add_executable(FTB src/main.cpp)
target_link_libraries(FTB PRIVATE FTB_core)

# 添加子目录 tests，但不将其包含在安装中
add_subdirectory(tests EXCLUDE_FROM_ALL)

# 安装规则（仅安装 FTB 和 FTB_core）
install(TARGETS FTB FTB_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件（假设你的头文件实际在 include/FTB/ 中）
install(DIRECTORY include/FTB/ DESTINATION include/FTB)

# --- 添加卸载目标 ---

# 设置一个变量 INSTALL_MANIFEST 指定安装清单文件路径
set(INSTALL_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

# 配置卸载脚本模板，生成 cmake_uninstall.cmake 文件
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  @ONLY
)

# 添加自定义卸载目标
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  COMMENT "卸载所有由 CMake 安装的文件"
)
