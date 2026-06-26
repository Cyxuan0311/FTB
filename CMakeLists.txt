cmake_minimum_required(VERSION 3.10)
project(FTB VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# 添加编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")

# ---- 可选功能开关 ----
option(FTB_ENABLE_SSH "Enable SSH connection support" OFF)
option(FTB_ENABLE_ICONS "Enable Nerd Font icons (requires patched font)" ON)
option(FTB_ENABLE_LIBCHAFA "Enable libchafa for advanced image preview" OFF)
option(FTB_ENABLE_TREE_SITTER "Enable tree-sitter for advanced syntax highlighting" OFF)
option(FTB_ENABLE_PLUGINS "Enable TypeScript/JavaScript plugin system" ON)

# 设置包含目录
include_directories(include)

# ---- 核心依赖 (必须) ----
find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# ---- 可选依赖 ----

# libssh2
if(FTB_ENABLE_SSH)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED libssh2)
    include_directories(${LIBSSH2_INCLUDE_DIRS})
    add_compile_definitions(FTB_ENABLE_SSH)
    message(STATUS "SSH support: ENABLED")
else()
    message(STATUS "SSH support: DISABLED")
endif()

if(FTB_ENABLE_ICONS)
    add_compile_definitions(FTB_ENABLE_ICONS)
    message(STATUS "Icons support: ENABLED (Nerd Font)")
else()
    message(STATUS "Icons support: DISABLED (ASCII fallback)")
endif()

if(FTB_ENABLE_LIBCHAFA)
    add_compile_definitions(FTB_ENABLE_LIBCHAFA)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CHAFA REQUIRED chafa)
    message(STATUS "libchafa support: ENABLED (advanced image preview)")
else()
    message(STATUS "libchafa support: DISABLED (using stb_image + Unicode blocks)")
endif()

# tree-sitter
if(FTB_ENABLE_TREE_SITTER)
    find_package(PkgConfig REQUIRED)
    set(ENV{PKG_CONFIG_PATH} "/usr/local/share/pkgconfig:/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    pkg_check_modules(TREE_SITTER REQUIRED tree-sitter)
    include_directories(${TREE_SITTER_INCLUDE_DIRS})
    add_compile_definitions(FTB_ENABLE_TREE_SITTER)
    message(STATUS "tree-sitter support: ENABLED (advanced syntax highlighting)")

else()
    message(STATUS "tree-sitter support: DISABLED (using built-in keyword matching)")
endif()

if(FTB_ENABLE_PLUGINS)
    add_compile_definitions(FTB_ENABLE_PLUGINS)
    message(STATUS "Plugin system: ENABLED (TypeScript/JavaScript)")
else()
    message(STATUS "Plugin system: DISABLED")
endif()

# ---- 构建核心库 ----

# 核心源文件
set(FTB_CORE_SOURCES
    src/FTB/AnsiParser.cpp
    src/FTB/ArchivePreview.cpp
    src/FTB/AsyncFileManager.cpp
    src/FTB/FuzzyFinder.cpp
    src/FTB/BinaryFileHandler.cpp
    src/FTB/CLIArgs.cpp
    src/FTB/ClipboardManager.cpp
    src/FTB/ConfigManager.cpp
    src/FTB/DirectoryHistory.cpp
    src/FTB/FileListRenderer.cpp
    src/FTB/FileManager.cpp
    src/FTB/FileSizeCalculator.cpp
    src/FTB/ImagePreview.cpp
    src/FTB/MarkdownPreview.cpp
    src/FTB/SpreadsheetPreview.cpp
    src/FTB/MediaPreview.cpp
    src/FTB/PdfPreview.cpp
    src/FTB/DocPreview.cpp
    src/FTB/HexPreview.cpp
    src/FTB/MainUI.cpp
    src/FTB/JumpFileContext.cpp
    src/FTB/KeyBindings.cpp
    src/FTB/ObjectPool.cpp
    src/FTB/Powerline.cpp
    src/FTB/Navigation.cpp
    src/FTB/PanelCommands.cpp
    src/FTB/PreviewCache.cpp
    src/FTB/SortMode.cpp
    src/FTB/StatusBarRenderer.cpp
    src/FTB/TextSelection.cpp
    src/FTB/ThreadGuard.cpp
    src/FTB/ThemeManager.cpp
    src/FTB/PluginManager.cpp
    src/FTB/detail_element.cpp
    src/FTB/HandleManager/EventHandler.cpp
    src/FTB/Editor/MD_transformer.cpp
    src/FTB/Editor/SyntaxHighlighter.cpp
    src/FTB/Editor/SyntaxHighlighterTreeSitter.cpp
    src/FTB/Editor/NanoEditor.cpp
    src/FTB/TabManager.cpp
    src/FTB/OpenerManager.cpp
)

# 核心UI对话框
list(APPEND FTB_CORE_SOURCES
    src/UI/HelpDialog.cpp
    src/UI/ThemeDialog.cpp
    src/UI/LayoutPanel.cpp
    src/UI/RenameDialog.cpp
    src/UI/NewFileDialog.cpp
    src/UI/NewFolderDialog.cpp
    src/UI/DeleteDialog.cpp
    src/UI/FilePreviewDialog.cpp
    src/UI/FolderDetailsDialog.cpp
    src/UI/JumpDirectoryDialog.cpp
    src/UI/CalendarPanel.cpp
    src/UI/FuzzyFinderDialog.cpp
    src/UI/PluginDialog.cpp
    src/UI/UIStylePanel.cpp
    src/UI/SortDialog.cpp
    src/UI/NewTabDialog.cpp
    src/UI/TabBarRenderer.cpp
    src/UI/ClipboardDialog.cpp
    src/UI/OpenerPickerDialog.cpp
    src/UI/OpenerInputDialog.cpp
    src/UI/OpenerConfigDialog.cpp
)

# 可选模块源文件
if(FTB_ENABLE_SSH)
    list(APPEND FTB_CORE_SOURCES
        src/Connection/SSHConnection.cpp
        src/UI/SSHDialog.cpp
    )
endif()

if(FTB_ENABLE_ICONS)
    list(APPEND FTB_CORE_SOURCES
        src/FTB/IconMapper.cpp
    )
endif()

# 构建核心库
add_library(FTB_core ${FTB_CORE_SOURCES})

# 核心链接库 (PUBLIC 以传播到最终可执行文件)
target_link_libraries(FTB_core
    PUBLIC
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
)

# 可选链接库 (PUBLIC 以传播到最终可执行文件)
if(FTB_ENABLE_SSH)
    # --no-as-needed 确保 SSHConnection 尚未被实际引用时 libssh2 仍被链接
    target_link_libraries(FTB_core PUBLIC
        "-Wl,--no-as-needed"
        ${LIBSSH2_LIBRARIES}
        "-Wl,--as-needed"
    )
    target_include_directories(FTB_core PUBLIC ${LIBSSH2_INCLUDE_DIRS})
endif()

if(FTB_ENABLE_LIBCHAFA)
    target_link_libraries(FTB_core PUBLIC ${CHAFA_LIBRARIES})
    target_include_directories(FTB_core PUBLIC ${CHAFA_INCLUDE_DIRS})
endif()

if(FTB_ENABLE_TREE_SITTER)
    target_include_directories(FTB_core PUBLIC ${TREE_SITTER_INCLUDE_DIRS})

    # ---- 检测各语言 grammar 库 ----
    set(TREE_SITTER_GRAMMARS
        bash         BUILD_TREE_SITTER_BASH
        c            BUILD_TREE_SITTER_C
        cpp          BUILD_TREE_SITTER_CPP
        go           BUILD_TREE_SITTER_GO
        java         BUILD_TREE_SITTER_JAVA
        javascript   BUILD_TREE_SITTER_JAVASCRIPT
        json         BUILD_TREE_SITTER_JSON
        python       BUILD_TREE_SITTER_PYTHON
        rust         BUILD_TREE_SITTER_RUST
        typescript   BUILD_TREE_SITTER_TYPESCRIPT
    )

    list(LENGTH TREE_SITTER_GRAMMARS _grammar_total)
    math(EXPR _grammar_count "${_grammar_total} / 2")
    math(EXPR _grammar_last "${_grammar_total} - 1")
    set(_found_count 0)

    # 收集所有 tree-sitter 库，用 --no-as-needed 强制链接
    set(_ts_libs "${TREE_SITTER_LIBRARIES}")

    foreach(_i RANGE 1 ${_grammar_last} 2)
        math(EXPR _pkg_idx "${_i} - 1")
        math(EXPR _def_idx "${_i}")
        list(GET TREE_SITTER_GRAMMARS ${_pkg_idx} _pkg_name)
        list(GET TREE_SITTER_GRAMMARS ${_def_idx} _def_name)
        pkg_check_modules(TS_${_pkg_name} tree-sitter-${_pkg_name} QUIET)
        if(TS_${_pkg_name}_FOUND)
            add_compile_definitions(${_def_name})
            list(APPEND _ts_libs ${TS_${_pkg_name}_LIBRARIES})
            target_include_directories(FTB_core PUBLIC ${TS_${_pkg_name}_INCLUDE_DIRS})
            math(EXPR _found_count "${_found_count} + 1")
            message(STATUS "  tree-sitter-${_pkg_name}: FOUND")
        else()
            message(STATUS "  tree-sitter-${_pkg_name}: NOT FOUND")
        endif()
    endforeach()
    message(STATUS "tree-sitter grammars: ${_found_count}/${_grammar_count} found")

    # --as-needed 会丢弃静态库引用的共享库, 用 --no-as-needed 强制保留
    target_link_libraries(FTB_core PUBLIC
        "-Wl,--no-as-needed"
        ${_ts_libs}
        "-Wl,--as-needed"
    )
endif()

# 构建主程序
add_executable(FTB src/FTB/main.cpp)
target_link_libraries(FTB
    PRIVATE
    FTB_core
)

# 添加子目录 tests，但不将其包含在安装中
add_subdirectory(tests EXCLUDE_FROM_ALL)

# 安装规则
install(TARGETS FTB FTB_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/FTB/ DESTINATION include/FTB)

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
