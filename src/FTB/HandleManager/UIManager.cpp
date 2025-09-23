#include "FTB/HandleManager/UIManager.hpp"
#include "FTB/HandleManager/UIManagerInternal.hpp"  // 包含内部辅助函数声明
#include "UI/MySQLDialog.hpp"  // 包含MySQL对话框

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sstream>
#include <future>

namespace UIManager {

bool handleEvents(ftxui::Event event, DirectoryHistory& directoryHistory,
                  std::string& currentPath, std::vector<std::string>& allContents,
                  std::vector<std::string>& filteredContents, int& selected,
                  std::string& searchQuery, ftxui::ScreenInteractive& screen,
                  std::atomic<bool>& refresh_ui, bool& vim_mode_active,
                  VimLikeEditor*& vimEditor) {
    try {
        if (event == ftxui::Event::Escape) {
            refresh_ui = false;
            screen.Exit();
            return true;
        }
        if (UIManagerInternal::handleRename(event, currentPath, allContents, filteredContents, selected, screen))
            return true;
        if (UIManagerInternal::handleImageTextPreview(event, currentPath, filteredContents, selected, screen))
            return true;
        if (UIManagerInternal::handleRangePreview(event, currentPath, filteredContents, selected, screen))
            return true;
        if (UIManagerInternal::handleFolderDetails(event, currentPath, filteredContents, selected, screen))
            return true;
        if (UIManagerInternal::handleNewFile(event, currentPath, allContents, filteredContents, screen))
            return true;
        if (UIManagerInternal::handleNewFolder(event, currentPath, allContents, filteredContents, screen))
            return true;
        if (UIManagerInternal::handleBackNavigation(event, directoryHistory, currentPath, allContents, filteredContents, selected, searchQuery))
            return true;
        if (UIManagerInternal::handleVimMode(event, currentPath, filteredContents, selected, vim_mode_active, vimEditor))
            return true;
        if (UIManagerInternal::handleDelete(event, currentPath, filteredContents, selected, allContents))
            return true;
        if (UIManagerInternal::handleCopy(event, currentPath, filteredContents, selected))
            return true;
        if (UIManagerInternal::handleClearClipboard(event))
            return true;
        if(UIManagerInternal::handleChooseFile(event, currentPath, filteredContents, selected))
            return true;
        if (UIManagerInternal::handleCut(event, currentPath, filteredContents, selected))
            return true;
        if (UIManagerInternal::handleCopy(event, currentPath, filteredContents, selected))
            return true;
        if (UIManagerInternal::handlePaste(event, currentPath, allContents, filteredContents))
            return true;
        if (UIManagerInternal::handleVideoPlay(event, currentPath, filteredContents, selected, screen))
            return true;
        if (UIManagerInternal::handleSSHConnection(event, screen))
            return true;
        if (UIManagerInternal::handleMySQLConnection(event, screen))
            return true;
        if (UIManagerInternal::handleConfigReload(event, screen))
            return true;
        if (UIManagerInternal::handleThemeSwitch(event, screen))
            return true;
    } catch (const std::exception& e) {
        std::cerr << "❗ Error: " << e.what() << std::endl;
    }
    return false;
}

std::pair<ftxui::Component, ftxui::Component> initializeUI(std::string& searchQuery,
                std::vector<std::string>& filteredContents,
                int& selected, DirectoryHistory& directoryHistory,
                std::string& currentPath,
                std::vector<std::string>& allContents) {
    auto searchInput = ftxui::Input(&searchQuery, "🔍 搜索...");
    ftxui::MenuOption menu_option;
    menu_option.on_enter = [&] {
    // 异步进入下一级目录
    // UIManager.cpp: 修改 initializeUI 中异步调用
    [[maybe_unused]] auto unused_future = std::async(std::launch::async, [&]() {
        FileManager::enterDirectory(directoryHistory, currentPath, filteredContents, selected);
        auto newContents = FileManager::getDirectoryContents(currentPath);
        // 注意：LRU缓存会自动管理目录内容，这里不需要手动更新
        allContents = newContents;
        filteredContents = newContents;
        searchQuery.clear();
    });

    };
    auto selector = ftxui::Menu(&filteredContents, &selected, menu_option);
    auto mouse_component = ftxui::CatchEvent(selector, [&](ftxui::Event event) {
    if (event.is_mouse()) {
        auto mouse = event.mouse();
        if (mouse.motion == ftxui::Mouse::Moved)
        return true;
        if (mouse.button == ftxui::Mouse::Left && mouse.motion == ftxui::Mouse::Pressed)
        return true;
    }
    return false;
    });
    auto container = ftxui::Container::Vertical({searchInput, mouse_component});
    return std::make_pair(container, searchInput);
}

} // namespace UIManager
