#include "FTB/HandleManager/UIManagerInternal.hpp"

#include <filesystem>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <algorithm>
#include <optional>
#include <future>

#include "FTB/ClipboardManager.hpp"
#include "FTB/DirectoryHistory.hpp"
#include "FTB/FileManager.hpp"
#include "FTB/Vim/Vim_Like.hpp"
#include "FTB/ObjectPool.hpp"
#include "FTB/AsyncFileManager.hpp"
#include "Video_and_Photo/ImageViewer.hpp"
#include "FTB/BinaryFileHandler.hpp"
#include "UI/MySQLDialog.hpp"
#include "UI/SystemInfoDialog.hpp"
#include "UI/NetworkServiceDialog.hpp"
#include "UI/JumpFileContextDialog.hpp"
#include "FTB/ConfigManager.hpp"
#include "FTB/ThemeManager.hpp"

namespace fs = std::filesystem;
using namespace ftxui;

namespace UIManagerInternal {

// --------------------------------------------------
// 辅助函数：获取选中项的完整路径
// 功能：根据当前路径和过滤后的列表与选中索引，验证索引有效性
//      并返回对应的完整路径（fs::path 对象）
// 返回：若索引无效则返回 std::nullopt，否则返回 std::optional<fs::path>
// --------------------------------------------------
std::optional<fs::path> getSelectedFullPath(
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected) 
{
    // 如果 selected 不在 [0, filteredContents.size()) 范围内，则无效
    if (selected < 0 || selected >= static_cast<int>(filteredContents.size()))
        return std::nullopt;
    // 否则将 currentPath 与 filteredContents[selected] 拼接为完整路径
    return fs::path(currentPath) / filteredContents[selected];
}

// --------------------------------------------------
// 处理文件/文件夹重命名操作（Alt+N）
// 功能：监听 Alt+N 键盘事件，弹出重命名对话框，执行重命名并刷新目录缓存
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   allContents      - 当前目录下所有文件/文件夹名称列表（引用）
//   filteredContents - 过滤后的列表（引用）
//   selected         - 当前选中项索引
//   screen           - FTXUI 交互式屏幕引用，用于弹出对话框
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleRename(
    Event event,
    const std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents,
    int selected,
    ScreenInteractive& screen) 
{
    // 仅在 Alt+N 键时触发
    if (event == Event::AltN) {
        // 获取选中项的完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path oldPath = *fullPathOpt;
        // 弹出重命名对话框，传入当前名称，获得新名称
        std::string newName = RenameDialog::show(screen, filteredContents[selected]);
        // 如果用户输入不为空，执行重命名
        if (!newName.empty()) {
            if (FileManager::renameFileOrDirectory(oldPath.string(), newName)) {
                // 重命名成功后，锁定缓存互斥量，刷新 allContents 与 filteredContents
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                allContents = FileManager::getDirectoryContents(currentPath);
                filteredContents = allContents;
                // 标记当前目录缓存失效，下次重新加载
                FileManager::lru_dir_cache->erase(currentPath);
            }
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理图像或文本预览操作（Alt+V）
// 功能：监听 Alt+V 键盘事件，根据文件后缀决定展示图片预览或文本预览
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的内容列表
//   selected         - 当前选中项索引
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleImageTextPreview(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    ScreenInteractive& screen) 
{
    // 仅在 Alt+V 键时触发
    if (event == Event::AltV) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        std::string ext = fullPath.extension().string();
        // 定义支持的图片后缀列表
        std::vector<std::string> imageExts = {".jpg", ".jpeg", ".png", ".bmp", ".gif"};

        // 若是图片，则调用 ImageViewer 展示图像
        if (std::find(imageExts.begin(), imageExts.end(), ext) != imageExts.end()) {
            try {
                ImageViewer::ImageViewerUI::showImagePreview(fullPath.string(), screen);
            } catch (const std::exception& e) {
                std::cerr << "❌ 图片预览失败: " << e.what() << std::endl;
            }
            return true;
        }
        // 如果是普通文件，则读取前几行文本并弹出文本预览对话框
        if (fs::is_regular_file(fullPath)) {
            // 读取文件前 20 行内容
            std::string fileContent = FileManager::readFileContent(fullPath.string(), 1, 20);
            FilePreviewDialog::show(screen, fullPath.string(), fileContent);
            return true;
        }
    }
    return false;
}

// --------------------------------------------------
// 处理带范围输入的文件预览（Ctrl+P）
// 功能：监听 Ctrl+P 键盘事件，弹出对话框允许用户输入行号范围后展示文件内容
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的内容列表
//   selected         - 当前选中项索引
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleRangePreview(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    ScreenInteractive& screen) 
{
    // 仅在 Ctrl+P 键时触发
    if (event == Event::CtrlP) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        // 如果文件需要限制预览（如二进制文件等），则直接返回 true 表示已“处理”
        if (BinaryFileHandler::BinaryFileRestrictor::shouldRestrictPreview(fullPath.string()))
            return true;
        // 如果不是常规文件，则提示并返回
        if (!fs::is_regular_file(fullPath)) {
            std::cerr << "❗ 选中的项目不是文件。" << std::endl;
            return true;
        }
        // 弹出带范围输入的预览对话框
        FilePreviewDialog::showWithRangeInput(screen, fullPath.string());
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理显示文件夹详情操作（空格键）
// 功能：监听空格键事件，若选中项是文件夹，则弹出文件夹详情对话框
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的内容列表
//   selected         - 当前选中项索引
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleFolderDetails(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    ScreenInteractive& screen) 
{
    // 仅在 空格 键时触发
    if (event == Event::Character(' ')) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        // 仅对目录进行详情展示，否则不处理
        if (!FileManager::isDirectory(fullPath.string()))
            return false;

        // 目标文件夹路径
        std::string targetPath = fullPath.string();
        int fileCount = 0, folderCount = 0;
        // 存储子项权限信息 (名称, mode_t)
        std::vector<std::tuple<std::string, mode_t>> folderPermissions;
        // 存储子文件名列表
        std::vector<std::string> fileNames;

        // 调用 FileManager 函数计算当前文件夹内的文件/文件夹数量、权限、名称列表
        FileManager::calculation_current_folder_files_number(
            targetPath, fileCount, folderCount, folderPermissions, fileNames);

        // 将结果填充到 FolderDetailsDialog 的结构体中
        FolderDetailsDialog::FolderDetails details;
        details.folderPath = targetPath;
        details.fileCount = fileCount;
        details.folderCount = folderCount;
        // 处理权限元组：只保留低 9 位权限信息
        for (const auto& tup : folderPermissions) {
            std::string name;
            mode_t mode;
            std::tie(name, mode) = tup;
            details.permissions.insert({ name, mode & 0777 });
        }
        details.fileNames = fileNames;

        // 弹出文件夹详情对话框
        FolderDetailsDialog::show(screen, details);
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理新建文件操作（Ctrl+F）
// 功能：监听 Ctrl+F 键盘事件，弹出输入对话框获取新文件名，创建文件并刷新缓存
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
//   filteredContents - 过滤后的内容列表（引用，会被更新）
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleNewFile(
    Event event,
    const std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents,
    ScreenInteractive& screen) 
{
    // 仅在 Ctrl+F 键时触发
    if (event == Event::CtrlF) {
        // 弹出对话框让用户输入新文件名
        std::string newFileName = NewFileDialog::show(screen);

        if (!newFileName.empty()) {
            // 构造完整路径
            fs::path fullPath = fs::path(currentPath) / newFileName;
            try {
                // 尝试创建空文件
                if (FileManager::createFile(fullPath.string())) {
                    // 创建成功后，更新 allContents 和 filteredContents，并标记缓存失效
                    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                    allContents = FileManager::getDirectoryContents(currentPath);
                    filteredContents = allContents;
                    FileManager::lru_dir_cache->erase(currentPath);
                } else {
                    // 创建失败则打印错误
                    std::cerr << "❗ Failed to create file: " << fullPath.string() << std::endl;
                }
            } catch (const std::exception& e) {
                // 捕获并打印异常信息
                std::cerr << "❗ Error creating file: " << e.what() << std::endl;
            }
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理新建文件夹操作（Ctrl+K）
// 功能：监听 Ctrl+K 键盘事件，弹出输入对话框获取文件夹名，创建文件夹并刷新缓存
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
//   filteredContents - 过滤后的内容列表（引用，会被更新）
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleNewFolder(
    Event event,
    const std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents,
    ScreenInteractive& screen) 
{
    // 仅在 Ctrl+K 键时触发
    if (event == Event::CtrlK) {
        // 弹出对话框让用户输入新文件夹名称
        std::string dirName = NewFolderDialog::show(screen);
        if (!dirName.empty()) {
            fs::path fullPath = fs::path(currentPath) / dirName;
            try {
                // 尝试创建目录
                if (FileManager::createDirectory(fullPath.string())) {
                    // 创建成功，刷新 allContents 和 filteredContents，并标记缓存失效
                    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                    allContents = FileManager::getDirectoryContents(currentPath);
                    filteredContents = allContents;
                    FileManager::lru_dir_cache->erase(currentPath);
                } else {
                    // 创建失败则打印错误
                    std::cerr << "❗ Failed to create directory: " << fullPath.string() << std::endl;
                }
            } catch (const std::exception& e) {
                // 捕获并打印异常信息
                std::cerr << "❗ Error creating directory: " << e.what() << std::endl;
            }
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理返回上一级目录操作（Backspace 或 左箭头）
// 功能：监听 Backspace 或 Left 键，优先清除搜索状态，其次切换到上一级目录或历史记录目录
// 参数：
//   event            - 当前捕获的键盘事件
//   directoryHistory - 维护的目录历史记录对象
//   currentPath      - 当前所在目录路径（引用，会被更新）
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
//   filteredContents - 过滤后的内容列表（引用，会被更新）
//   selected         - 选中索引（引用，会被重置）
//   searchQuery      - 搜索关键字（引用，会被清空）
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleBackNavigation(
    ftxui::Event event,
    DirectoryHistory& directoryHistory,
    std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents,
    int& selected,
    std::string& searchQuery) 
{
    // 仅在 Backspace 或 左箭头 键时触发
    if (event == ftxui::Event::Backspace || event == ftxui::Event::ArrowLeft) {
        // 如果当前有搜索关键字，先退出搜索，不进行目录切换
        if (!searchQuery.empty())
            return false;

        // 如果选中项不是第一个，只重置选中状态，不切换目录
        if (selected != 0) {
            selected = 0;
            return true;
        }

        // 检查是否有父目录或历史记录可返回
        fs::path current(currentPath);
        if (current.has_parent_path() || !directoryHistory.empty()) {
            // 将当前路径加入历史记录栈
            directoryHistory.push(currentPath);

            // 优先获取父目录，否则从历史记录中 pop
            std::string newPath = current.has_parent_path()
                ? current.parent_path().string()
                : directoryHistory.pop();

            // 异步获取规范化路径，避免阻塞 UI 线程
            auto futureCanonical = std::async(std::launch::async, [newPath]() {
                return fs::canonical(newPath).string();
            });
            currentPath = futureCanonical.get();

            // 标记当前目录缓存失效
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_dir_cache->erase(currentPath);
            }

            // 使用异步文件管理器加载新目录下的内容
            FTB::GlobalAsyncFileManager::getInstance().asyncGetDirectoryContents(
                currentPath, 
                [&allContents, &filteredContents](std::vector<std::string> newContents) {
                    allContents = std::move(newContents);
                    filteredContents = allContents;
                }
            );

            // 重置搜索状态和选中项
            searchQuery.clear();
            selected = 0;
            return true;
        }
    }
    return false;
}

// --------------------------------------------------
// 进入 Vim-like 编辑模式（Ctrl+E）
// 功能：监听 Ctrl+E 键，若选中项是文件且不被限制，则读取文件内容并进入 Vim 编辑器
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的内容列表
//   selected         - 选中索引
//   vim_mode_active  - 是否处于 Vim 模式的标志（引用，可被修改）
//   vimEditor        - VimLikeEditor 指针（引用，可被分配/释放）
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleVimMode(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    bool& vim_mode_active,
    std::unique_ptr<VimLikeEditor>& vimEditor) 
{
    // 仅在 Ctrl+E 键时触发
    if (event == Event::CtrlE) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        // 如果需要限制编辑（如二进制文件），则不处理
        if (BinaryFileHandler::BinaryFileRestrictor::shouldRestrictEdit(fullPath.string()))
            return false;
        // 仅对常规文件进行编辑
        if (!FileManager::isDirectory(fullPath.string())) {
            // 如果已有 vimEditor，先返回到对象池
            if (vimEditor != nullptr) {
                FTB::VimEditorPool::getInstance().release(std::move(vimEditor));
            }
            // 从对象池获取新的 VimLikeEditor 实例
            vimEditor = FTB::VimEditorPool::getInstance().acquire();
            
            // 异步读取文件前 1000 行内容
            FTB::GlobalAsyncFileManager::getInstance().asyncReadFileContent(
                fullPath.string(), 1, 1000,
                [&vimEditor, &vim_mode_active, fullPath](std::string fileContent) {

                    // 将读取的文本按行分割后设置到编辑器中
                    std::vector<std::string> lines;
                    lines.reserve(1000);  // 预分配空间
                    std::istringstream iss(fileContent);
                    std::string line;
                    while (std::getline(iss, line))
                        lines.push_back(line);
                    vimEditor->SetContent(lines);
                    
                    // 设置文件名以启用语法高亮
                    vimEditor->SetFilename(fullPath.string());

                    // 进入编辑模式
                    vimEditor->EnterEditMode();
                    // 设置退出编辑时的回调：写回文件并更新缓存
                    vimEditor->SetOnExit([&vimEditor, &vim_mode_active, fullPath](const std::vector<std::string>& new_content) {
                        std::ostringstream oss;
                        for (const auto& l : new_content)
                            oss << l << "\n";
                        std::string updatedContent = oss.str();
                        
                        // 异步写入文件内容
                        FTB::GlobalAsyncFileManager::getInstance().asyncWriteFileContent(
                            fullPath.string(), updatedContent,
                            [&vimEditor, &vim_mode_active, fullPath](bool success) {
                                if (success) {
                                    // 标记缓存失效
                                    FileManager::lru_dir_cache->erase(fullPath.string());
                                }
                                vim_mode_active = false;
                                if (vimEditor) {
                                    FTB::VimEditorPool::getInstance().release(std::move(vimEditor));
                                }
                            }
                        );
                    });

                    // 激活 Vim 模式
                    vim_mode_active = true;
                }
            );
            return true;
        }
    }
    return false;
}

// --------------------------------------------------
// 处理删除操作（Delete 键）
// 功能：监听 Delete 键，删除选中项（文件或目录），然后刷新缓存
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的列表
//   selected         - 选中索引
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleDelete(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    std::vector<std::string>& allContents) 
{
    // 仅在 Delete 键时触发
    if (event == Event::Delete) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        try {
            // 调用 FileManager 删除文件或目录
            if (FileManager::deleteFileOrDirectory(fullPath.string())) {
                // 删除成功后，锁定缓存互斥量，更新 allContents 并刷新缓存
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                allContents = FileManager::getDirectoryContents(currentPath);
                // 强制修改 filteredContents（const_cast 避免 const 限制）
                const_cast<std::vector<std::string>&>(filteredContents) = allContents;
                FileManager::lru_dir_cache->erase(currentPath);
            } else {
                std::cerr << "❗ Failed to delete: " << fullPath.string() << std::endl;
            }
        } catch (const std::exception& e) {
            // 捕获并打印删除异常
            std::cerr << "❗ Error deleting: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理选取文件或文件夹到剪贴板（Alt+C）
// 功能：监听 Alt+C 键，将选中项路径加入 ClipboardManager
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的列表
//   selected         - 选中索引
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleChooseFile(
    Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected) 
{
    // 仅在 Alt+C 键时触发
    if (event == Event::AltC) {
        // 如果索引有效，则将路径添加到剪贴板管理器
        if (selected >= 0 && selected < static_cast<int>(filteredContents.size())) {
            fs::path fullPath = fs::path(currentPath) / filteredContents[selected];
            ClipboardManager::getInstance().addItem(fullPath.string());
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理清空剪贴板操作（Alt+G）
// 功能：监听 Alt+G 键，清空剪贴板中的所有项目
// 参数：
//   event - 当前捕获的键盘事件
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleClearClipboard(Event event) {
    if (event == Event::AltG) {
        ClipboardManager::getInstance().clear();
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理剪切模式切换操作（Ctrl+X）
// 功能：监听 Ctrl+X 键，将剪贴板管理器设置为“剪切”模式
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的列表
//   selected         - 选中索引
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleCut(
    ftxui::Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected) 
{
    // 仅在 Ctrl+X 键时触发
    if (event == ftxui::Event::CtrlX) {
        // 验证索引有效性
        auto fullPathOpt = UIManagerInternal::getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;
        // 设置剪贴板为剪切模式
        ClipboardManager::getInstance().setCutMode(true);
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理复制模式切换操作（Ctrl+T）
// 功能：监听 Ctrl+T 键，将剪贴板管理器设置为“复制”模式
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的列表
//   selected         - 选中索引
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleCopy(
    ftxui::Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected) 
{
    // 仅在 Ctrl+T 键时触发
    if (event == ftxui::Event::CtrlT) {
        // 验证索引有效性
        auto fullPathOpt = UIManagerInternal::getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;
        // 设置剪贴板为复制模式
        ClipboardManager::getInstance().setCutMode(false);
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理粘贴操作（Ctrl+N）
// 功能：监听 Ctrl+N 键，将剪贴板中的项目粘贴到当前目录，并刷新缓存
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
//   filteredContents - 过滤后的列表（引用，会被更新）
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handlePaste(
    Event event,
    const std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents) 
{
    // 仅在 Ctrl+N 键时触发
    if (event == Event::CtrlN) {
        // 如果剪贴板为空，则直接返回
        if (!ClipboardManager::getInstance().getItems().empty()) {
            bool sameDir = false;
            const auto& items = ClipboardManager::getInstance().getItems();
            // 检查剪贴板项目是否已经在当前目录下（避免同文件夹内粘贴）
            for (const auto& item : items) {
                if (fs::path(item).parent_path() == fs::path(currentPath)) {
                    sameDir = true;
                    break;
                }
            }
            // 若不在同一目录，则执行粘贴
            if (!sameDir) {
                if (ClipboardManager::getInstance().paste(currentPath)) {
                    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                    allContents = FileManager::getDirectoryContents(currentPath);
                    filteredContents = allContents;
                    FileManager::lru_dir_cache->erase(currentPath);
                }
            }
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理视频播放操作（Alt+P）
// 功能：监听 Alt+P 键，若选中项是支持的视频文件，则调用 VideoPlayerUI 播放视频
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径
//   filteredContents - 过滤后的列表
//   selected         - 选中索引
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleVideoPlay(
    ftxui::Event event,
    const std::string& currentPath,
    const std::vector<std::string>& filteredContents,
    int selected,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Alt+P 键时触发
    if (event == ftxui::Event::AltP) {
        // 获取选中项完整路径
        auto fullPathOpt = getSelectedFullPath(currentPath, filteredContents, selected);
        if (!fullPathOpt)
            return false;

        fs::path fullPath = *fullPathOpt;
        std::string ext = fullPath.extension().string();
        // 定义常见视频文件后缀
        std::vector<std::string> videoExts = {".mp4", ".avi", ".mkv", ".mov", ".flv", ".wmv"};
        // 如果后缀匹配，则调用 VideoPlayerUI 播放视频
        if (std::find(videoExts.begin(), videoExts.end(), ext) != videoExts.end()) {
            try {
                VideoPlayer::VideoPlayerUI::playVideo(fullPath.string(), screen);
            } catch (const std::exception& e) {
                std::cerr << "❌ 视频播放失败: " << e.what() << std::endl;
            }
            return true;
        }
    }
    return false;
}

// --------------------------------------------------
// 处理SSH连接操作（Ctrl+S）
// 功能：监听 Ctrl+S 键，弹出SSH连接对话框，建立SSH连接
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleSSHConnection(
    ftxui::Event event,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Ctrl+S 键时触发
    if (event == ftxui::Event::CtrlS) {
        try {
            // 创建SSH对话框
            UI::SSHDialog ssh_dialog;
            
            // 显示对话框并获取连接参数
            auto params = ssh_dialog.showDialog(screen);
            
            // 如果用户确认了连接参数
            if (!params.hostname.empty()) {
                // 创建SSH连接对象
                Connection::SSHConnection ssh_connection;
                
                // 设置状态回调
                ssh_connection.setStatusCallback([](Connection::SSHConnectionStatus status) {
                    switch (status) {
                        case Connection::SSHConnectionStatus::CONNECTING:
                            std::cout << "🔗 正在连接SSH服务器..." << std::endl;
                            break;
                        case Connection::SSHConnectionStatus::CONNECTED:
                            std::cout << "✅ SSH连接成功！" << std::endl;
                            break;
                        case Connection::SSHConnectionStatus::ERROR:
                            std::cout << "❌ SSH连接失败！" << std::endl;
                            break;
                        case Connection::SSHConnectionStatus::DISCONNECTED:
                            std::cout << "🔌 SSH连接已断开" << std::endl;
                            break;
                    }
                });
                
                // 尝试连接
                if (ssh_connection.connect(params)) {
                    std::cout << "🎉 成功连接到 " << params.hostname << ":" << params.port << std::endl;
                    std::cout << "📁 远程目录: " << params.remote_directory << std::endl;
                    
                    // 执行一个简单的命令来验证连接
                    std::string result = ssh_connection.executeCommand("pwd");
                    if (!result.empty()) {
                        std::cout << "📍 当前工作目录: " << result;
                    }
                    
                    // 这里可以添加更多的远程操作逻辑
                    // 例如：列出远程目录内容、文件传输等
                    
                    // 断开连接
                    ssh_connection.disconnect();
                } else {
                    std::cout << "❌ 连接失败: " << ssh_connection.getLastError() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ SSH连接错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理MySQL连接操作（Alt+D）
// 功能：监听 Alt+D 键，弹出MySQL数据库管理对话框
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleMySQLConnection(
    ftxui::Event event,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Alt+D 键时触发
    if (event == ftxui::Event::AltD) {
        std::cout << "🔍 Alt+D 键被按下，正在打开MySQL数据库管理器..." << std::endl;
        try {
            // 创建MySQL对话框
            UI::MySQLDialog mysql_dialog;
            
            // 设置连接回调
            mysql_dialog.setConnectionCallback([](const Connection::MySQLConnectionParams& params) {
                std::cout << "🎉 成功连接到MySQL数据库！" << std::endl;
                std::cout << "📍 主机: " << params.hostname << ":" << params.port << std::endl;
                std::cout << "👤 用户: " << params.username << std::endl;
                std::cout << "🗄️ 数据库: " << (params.database.empty() ? "未指定" : params.database) << std::endl;
                std::cout << "🌐 连接类型: " << (params.is_local ? "本地" : "远程") << std::endl;
            });
            
            std::cout << "📱 正在显示MySQL数据库管理器界面..." << std::endl;
            // 显示对话框
            mysql_dialog.showDialog(screen);
            std::cout << "✅ MySQL数据库管理器已关闭" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ MySQL连接错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理配置重载操作（Ctrl+R）
// 功能：监听 Ctrl+R 键，重新加载配置文件
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleConfigReload(
    ftxui::Event event,
    [[maybe_unused]] ftxui::ScreenInteractive& screen) 
{
    // 仅在 Ctrl+R 键时触发
    if (event == ftxui::Event::CtrlR) {
        try {
            auto config_manager = FTB::ConfigManager::GetInstance();
            if (config_manager->ReloadConfig()) {
                std::cout << "✅ 配置文件重新加载成功" << std::endl;
            } else {
                std::cout << "❌ 配置文件重新加载失败" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ 配置重载错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理主题切换操作（Ctrl+T）
// 功能：监听 Ctrl+T 键，循环切换可用主题
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleThemeSwitch(
    ftxui::Event event,
    [[maybe_unused]] ftxui::ScreenInteractive& screen) 
{
    // 仅在 Ctrl+T 键时触发
    if (event == ftxui::Event::CtrlT) {
        try {
            auto theme_manager = FTB::ThemeManager::GetInstance();
            auto available_themes = theme_manager->GetAvailableThemes();
            
            if (available_themes.size() > 1) {
                // 找到当前主题的下一个主题
                std::string current_theme = theme_manager->GetCurrentTheme();
                auto it = std::find(available_themes.begin(), available_themes.end(), current_theme);
                
                if (it != available_themes.end()) {
                    ++it;
                    if (it == available_themes.end()) {
                        it = available_themes.begin(); // 循环到第一个
                    }
                    theme_manager->ApplyTheme(*it);
                    std::cout << "🎨 主题已切换到: " << *it << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ 主题切换错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理系统信息显示操作（Alt+H）
// 功能：监听 Alt+H 键，弹出系统信息对话框，显示设备、状态、磁盘、网络等信息
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleSystemInfo(
    ftxui::Event event,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Alt+H 键时触发
    if (event == ftxui::Event::AltH) {
        std::cout << "🔍 Alt+H 键被按下，正在打开系统信息管理器..." << std::endl;
        try {
            // 创建系统信息对话框
            UI::SystemInfoDialog system_info_dialog;
            
            // 设置信息更新回调
            system_info_dialog.setInfoUpdateCallback([]() {
                std::cout << "🔄 系统信息已更新" << std::endl;
            });
            
            std::cout << "📱 正在显示系统信息管理器界面..." << std::endl;
            // 显示对话框
            system_info_dialog.showDialog(screen);
            std::cout << "✅ 系统信息管理器已关闭" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 系统信息显示错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理网络服务操作（Alt+N）
// 功能：监听 Alt+N 键，弹出网络服务管理器对话框
// 参数：
//   event            - 当前捕获的键盘事件
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleNetworkService(
    ftxui::Event event,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Alt+N 键时触发
    if (event == ftxui::Event::AltT) {
        //std::cout << "🔍 Alt+N 键被按下，正在打开网络服务管理器..." << std::endl;
        try {
            // 创建网络服务对话框
            UI::NetworkServiceDialog network_dialog;
            
            // 设置状态更新回调
            network_dialog.setStatusUpdateCallback([]() {
                std::cout << "🔄 网络信息已更新" << std::endl;
            });
            
            //std::cout << "📱 正在显示网络服务管理器界面..." << std::endl;
            // 显示对话框
            network_dialog.showDialog(screen);
            //std::cout << "✅ 网络服务管理器已关闭" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 网络服务错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

// --------------------------------------------------
// 处理目录跳转操作（Alt+J）
// 功能：监听 Alt+J 键，弹出目录跳转对话框，允许用户输入目标路径并跳转
// 参数：
//   event            - 当前捕获的键盘事件
//   currentPath      - 当前所在目录路径（引用，会被更新）
//   allContents      - 当前目录下所有内容列表（引用，会被更新）
//   filteredContents - 过滤后的内容列表（引用，会被更新）
//   selected         - 选中索引（引用，会被重置）
//   searchQuery      - 搜索关键字（引用，会被清空）
//   screen           - FTXUI 交互式屏幕引用
// 返回值：
//   true 表示事件已处理；false 表示与本操作无关
// --------------------------------------------------
bool handleJumpToDirectory(
    ftxui::Event event,
    std::string& currentPath,
    std::vector<std::string>& allContents,
    std::vector<std::string>& filteredContents,
    int& selected,
    std::string& searchQuery,
    ftxui::ScreenInteractive& screen) 
{
    // 仅在 Alt+J 键时触发
    if (event == ftxui::Event::AltJ) {
        try {
            // 创建目录跳转对话框
            UI::JumpFileContextDialog jump_dialog;
            
            // 显示对话框并获取结果
            auto params = jump_dialog.showDialog(screen);
            
            // 如果用户确认了跳转参数
            if (!params.target_path.empty()) {
                // 执行跳转逻辑
                if (FTB::JumpFileContext::executeJump(params)) {
                    // 获取规范化路径
                    std::string canonical_path = FTB::JumpFileContext::getCanonicalPath(params.target_path);
                    
                    // 更新当前路径
                    currentPath = canonical_path;
                    
                    // 清空搜索查询
                    searchQuery.clear();
                    
                    // 重置选中索引
                    selected = 0;
                    
                    // 标记当前目录缓存失效
                    {
                        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                        FileManager::lru_dir_cache->erase(currentPath);
                    }
                    
                    // 使用高效的并发方式异步加载新目录内容
                    // 立即启动异步任务，不阻塞UI线程，提高响应速度
                    [[maybe_unused]] auto future = std::async(std::launch::async, [&allContents, &filteredContents, currentPath]() {
                        try {
                            // 在后台线程中获取目录内容，使用并发优化
                            auto newContents = FileManager::getDirectoryContents(currentPath);
                            
                            // 原子性更新内容列表，避免竞态条件
                            allContents = std::move(newContents);
                            filteredContents = allContents;
                        } catch (const std::exception& e) {
                            std::cerr << "❌ 异步加载目录内容失败: " << e.what() << std::endl;
                        }
                    });
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 目录跳转错误: " << e.what() << std::endl;
        }
        return true;
    }
    return false;
}

} // namespace UIManagerInternal
