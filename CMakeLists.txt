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
include_directories(${LIBSSH2_INCLUDE_DIRS})
include_directories(${YAML_CPP_INCLUDE_DIRS})

# 查找依赖库
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)

# 查找yaml-cpp库
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

# 查找TBB库
find_package(TBB REQUIRED)

# 查找libssh2
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSSH2 REQUIRED libssh2)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

# 查找X11库
find_package(X11 REQUIRED)

# 构建图片预览库
add_library(image_viewer
    src/Video_and_Photo/ImageViewer.cpp
    src/Video_and_Photo/VideoPlayer.cpp
    src/Video_and_Photo/CommonMedia.cpp
    src/Video_and_Photo/FFmpegUtils.cpp
)

target_link_libraries(image_viewer
    PRIVATE
    PkgConfig::FFMPEG
    ftxui::screen
    ftxui::dom
    ftxui::component
    ${X11_LIBRARIES}  # 添加X11库链接
)

# 使用通配符匹配所有 .cpp 文件
file(GLOB FTB_CORE_SOURCES
    src/*.cpp
    src/UI/*.cpp
    src/FTB/*.cpp
    src/FTB/HandleManager/*.cpp
    src/Connection/*.cpp
)

# 构建核心库
add_library(FTB_core ${FTB_CORE_SOURCES})

# 链接 FTXUI 库
target_link_libraries(FTB_core 
    PRIVATE 
    ftxui::screen 
    ftxui::dom 
    ftxui::component
    image_viewer
    nlohmann_json::nlohmann_json
    ${YAML_CPP_LIBRARIES}
    ${LIBSSH2_LIBRARIES}
    TBB::tbb
)

# 添加编译标志
target_compile_options(FTB_core PRIVATE ${YAML_CPP_CFLAGS_OTHER})

# 构建主程序
add_executable(FTB src/FTB/main.cpp)
target_link_libraries(FTB 
    PRIVATE 
    FTB_core
    image_viewer
)

# 添加子目录 tests，但不将其包含在安装中
add_subdirectory(tests EXCLUDE_FROM_ALL)

# 安装规则
install(TARGETS FTB FTB_core image_viewer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/FTB/ DESTINATION include/FTB)
install(DIRECTORY include/Video_and_Photo/ DESTINATION include/Video_and_Photo)

# --- 添加卸载目标 ---
set(INSTALL_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    COMMENT "卸载所有由 CMake 安装的文件"
)
