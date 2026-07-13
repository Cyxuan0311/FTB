cmake_minimum_required(VERSION 3.10)
project(FTB VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# 检测编译器特性
include(CheckCXXSourceCompiles)

# Apple ld 不支持 --no-as-needed，检测并跳过
set(CMAKE_REQUIRED_FLAGS "-Wl,--no-as-needed")
check_cxx_source_compiles("int main(){return 0;}" LINKER_SUPPORTS_NO_AS_NEEDED)
set(CMAKE_REQUIRED_FLAGS "")

# 添加编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")

# ---- 可选功能开关 ----
option(FTB_ENABLE_SSH "Enable SSH connection support" OFF)
option(FTB_ENABLE_ICONS "Enable Nerd Font icons (requires patched font)" OFF)
option(FTB_ENABLE_LIBCHAFA "Enable libchafa for advanced image preview" OFF)
option(FTB_ENABLE_LIBSIXEL "Enable libsixel for native sixel image preview" OFF)
option(FTB_ENABLE_TREE_SITTER "Enable tree-sitter for advanced syntax highlighting" OFF)
option(FTB_ENABLE_PLUGINS "Enable TypeScript/JavaScript plugin system" OFF)
option(FTB_ENABLE_AI "Enable AI assistant panel (requires httplib)" OFF)

# 设置包含目录
include_directories(include)
include_directories(third_party)

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

if(FTB_ENABLE_LIBSIXEL)
    add_compile_definitions(FTB_ENABLE_LIBSIXEL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SIXEL REQUIRED libsixel)
    # libsixel may install header as <sixel.h> (flat) or <sixel/sixel.h> (subdir)
    if(EXISTS "${SIXEL_INCLUDE_DIRS}/sixel/sixel.h")
        add_compile_definitions(LIBSIXEL_HAS_SUBDIR)
    endif()
    message(STATUS "libsixel support: ENABLED (native sixel image preview)")
else()
    message(STATUS "libsixel support: DISABLED")
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

if(FTB_ENABLE_AI)
    add_compile_definitions(FTB_ENABLE_AI)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)
        message(STATUS "AI assistant: ENABLED (Ollama + OpenAI + HTTPS/SSL)")
    else()
        message(STATUS "AI assistant: ENABLED (Ollama only, install libssl-dev for HTTPS)")
    endif()
else()
    message(STATUS "AI assistant: DISABLED")
endif()

# ---- 构建核心库 ----

# 核心源文件
set(FTB_CORE_SOURCES
    # core
    src/core/AnsiParser.cpp
    src/core/FuzzyFinder.cpp
    src/core/JumpFileContext.cpp
    src/core/MainUI.cpp
    src/core/Navigation.cpp
    src/core/PanelCommands.cpp
    src/core/TabManager.cpp
    # browser
    src/browser/AsyncFileManager.cpp
    src/browser/BinaryFileHandler.cpp
    src/browser/ClipboardManager.cpp
    src/browser/DirectoryHistory.cpp
    src/browser/FileManager.cpp
    src/browser/FileSizeCalculator.cpp
    src/browser/SortMode.cpp
    src/browser/TaskSystem.cpp
    # preview
    src/preview/ArchivePreview.cpp
    src/preview/DocPreview.cpp
    src/preview/HexPreview.cpp
    src/preview/ImagePreview.cpp
    src/preview/MarkdownPreview.cpp
    src/preview/MediaPreview.cpp
    src/preview/AudioPreview.cpp
    src/preview/PdfPreview.cpp
    src/preview/PreviewCache.cpp
    src/preview/SpreadsheetPreview.cpp
    # protocols
    src/protocols/SixelProtocol.cpp
    src/protocols/KittyProtocol.cpp
    src/protocols/ITerm2Protocol.cpp
    src/protocols/ImageOutputManager.cpp
    src/protocols/stb_image_write_impl.cpp
    src/protocols/stb_image_resize_impl.cpp
    # renderer
    src/renderer/FileListRenderer.cpp
    src/renderer/Powerline.cpp
    src/renderer/StatusBarRenderer.cpp
    src/renderer/TextSelection.cpp
    src/renderer/detail_element.cpp
    # utils
    src/utils/UnicodeUtil.cpp
    src/utils/TerminalProbe.cpp
    src/utils/TmuxContext.cpp
    # config
    src/config/CLIArgs.cpp
    src/config/ConfigManager.cpp
    src/config/DefaultExtensionColors.cpp
    src/config/KeyBindings.cpp
    src/config/ObjectPool.cpp
    src/config/ThemeManager.cpp
    src/config/ThreadGuard.cpp
    # ops
    src/ops/ArchiveOps.cpp
    src/ops/EventHandler.cpp
    src/ops/OpenerManager.cpp
    src/ops/PluginManager.cpp
    # editor
    src/editor/MD_transformer.cpp
    src/editor/NanoEditor.cpp
    src/editor/SyntaxHighlighter.cpp
    src/editor/SyntaxHighlighterTreeSitter.cpp
)

# 对话框
list(APPEND FTB_CORE_SOURCES
    src/dialog/CalendarPanel.cpp
    src/dialog/ClipboardDialog.cpp
    src/dialog/DeleteDialog.cpp
    src/dialog/EditorPanel.cpp
    src/dialog/ImagePreviewPanel.cpp
    src/dialog/HexEditorPanel.cpp
    src/dialog/FilePreviewDialog.cpp
    src/dialog/FolderDetailsDialog.cpp
    src/dialog/FuzzyFinderDialog.cpp
    src/dialog/HelpDialog.cpp
    src/dialog/JumpDirectoryDialog.cpp
    src/dialog/LayoutPanel.cpp
    src/dialog/NewFileDialog.cpp
    src/dialog/NewFolderDialog.cpp
    src/dialog/NewTabDialog.cpp
    src/dialog/OpenerConfigDialog.cpp
    src/dialog/OpenerInputDialog.cpp
    src/dialog/OpenerPickerDialog.cpp
    src/dialog/PluginDialog.cpp
    src/dialog/RenameDialog.cpp
    src/dialog/BatchRenameDialog.cpp
    src/dialog/SortDialog.cpp
    src/dialog/TabBarRenderer.cpp
    src/dialog/TaskPanelDialog.cpp
    src/dialog/ThemeDialog.cpp
    src/dialog/UIStylePanel.cpp
)

# AI 模块源文件
if(FTB_ENABLE_AI)
    list(APPEND FTB_CORE_SOURCES
        src/ai/AIClient.cpp
        src/ai/ContextBuilder.cpp
        src/ai/ToolRegistry.cpp
        src/ai/ResponseRouter.cpp
        src/ai/ActionExecutor.cpp
        src/ai/ActionExecutor_FileIO.cpp
        src/ai/ActionExecutor_Browse.cpp
        src/ai/ActionExecutor_Search.cpp
        src/ai/ActionExecutor_Clipboard.cpp
        src/ai/ActionExecutor_Batch.cpp
        src/ai/ActionExecutor_Advanced.cpp
        src/ai/MemoryManager.cpp
        src/ai/AIAgent.cpp
        src/ai/SessionManager.cpp
        src/dialog/AIDialog.cpp
        src/dialog/AIConfigDialog.cpp
    )
endif()

# 可选模块源文件
if(FTB_ENABLE_SSH)
    list(APPEND FTB_CORE_SOURCES
        src/remote/SSHConnection.cpp
        src/dialog/SSHDialog.cpp
    )
endif()

if(FTB_ENABLE_ICONS)
    list(APPEND FTB_CORE_SOURCES
        src/renderer/IconMapper.cpp
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
# AI 模块链接 (SSL for OpenRouter / OpenAI HTTPS endpoints)
if(FTB_ENABLE_AI AND OpenSSL_FOUND)
    target_link_libraries(FTB_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(FTB_ENABLE_SSH)
    target_include_directories(FTB_core PUBLIC ${LIBSSH2_INCLUDE_DIRS})
    if(LINKER_SUPPORTS_NO_AS_NEEDED)
        # --no-as-needed 确保 SSHConnection 尚未被实际引用时 libssh2 仍被链接
        target_link_libraries(FTB_core PUBLIC
            "-Wl,--no-as-needed"
            ${LIBSSH2_LIBRARIES}
            "-Wl,--as-needed"
        )
    else()
        target_link_libraries(FTB_core PUBLIC ${LIBSSH2_LIBRARIES})
    endif()
endif()

if(FTB_ENABLE_LIBCHAFA)
    target_link_libraries(FTB_core PUBLIC ${CHAFA_LIBRARIES})
    target_include_directories(FTB_core PUBLIC ${CHAFA_INCLUDE_DIRS})
endif()

if(FTB_ENABLE_LIBSIXEL)
    target_link_libraries(FTB_core PUBLIC ${SIXEL_LIBRARIES})
    target_include_directories(FTB_core PUBLIC ${SIXEL_INCLUDE_DIRS})
endif()

if(FTB_ENABLE_TREE_SITTER)
    target_include_directories(FTB_core PUBLIC ${TREE_SITTER_INCLUDE_DIRS})

    # ---- 检测各语言 grammar 库 ----
    set(TREE_SITTER_GRAMMARS
        bash         BUILD_TREE_SITTER_BASH
        c            BUILD_TREE_SITTER_C
        clojure      BUILD_TREE_SITTER_CLOJURE
        cpp          BUILD_TREE_SITTER_CPP
        dart         BUILD_TREE_SITTER_DART
        erlang       BUILD_TREE_SITTER_ERLANG
        fsharp       BUILD_TREE_SITTER_FSHARP
        go           BUILD_TREE_SITTER_GO
        java         BUILD_TREE_SITTER_JAVA
        javascript   BUILD_TREE_SITTER_JAVASCRIPT
        json         BUILD_TREE_SITTER_JSON
        julia        BUILD_TREE_SITTER_JULIA
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

    if(LINKER_SUPPORTS_NO_AS_NEEDED)
        # --as-needed 会丢弃静态库引用的共享库, 用 --no-as-needed 强制保留
        target_link_libraries(FTB_core PUBLIC
            "-Wl,--no-as-needed"
            ${_ts_libs}
            "-Wl,--as-needed"
        )
    else()
        target_link_libraries(FTB_core PUBLIC ${_ts_libs})
    endif()
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
install(DIRECTORY
    include/core include/browser include/preview
    include/renderer include/editor include/config
    include/ops include/utils include/dialog include/remote
    DESTINATION include/FTB
)

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
