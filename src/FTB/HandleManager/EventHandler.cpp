#include "FTB/HandleManager/EventHandler.hpp"
#include "FTB/ClipboardManager.hpp"
#include "FTB/FileManager.hpp"
#include "FTB/ConfigManager.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

static std::vector<std::string> GetBatchPaths(MainState& state) {
    std::vector<std::string> paths;
    if (!state.batch_selected.empty()) {
        for (int idx : state.batch_selected) {
            if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                paths.push_back(
                    (fs::path(state.currentPath) / state.filteredContents[idx]).string());
            }
        }
    } else if (state.selected >= 0 &&
               state.selected < static_cast<int>(state.filteredContents.size())) {
        paths.push_back(
            (fs::path(state.currentPath) / state.filteredContents[state.selected]).string());
    }
    return paths;
}

bool HandleFileOperationEvent(MainState& state, const Event& event) {
    // Ctrl+Y: 复制选中项
    if (event == Event::CtrlY) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            auto& clipboard = ClipboardManager::getInstance();
            clipboard.clear();
            clipboard.setCutMode(false);
            for (const auto& p : paths) {
                clipboard.addItem(p);
            }
        }
        return true;
    }

    // Ctrl+X: 剪切选中项
    if (event == Event::CtrlX) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            auto& clipboard = ClipboardManager::getInstance();
            clipboard.clear();
            clipboard.setCutMode(true);
            for (const auto& p : paths) {
                clipboard.addItem(p);
            }
        }
        return true;
    }

    // Ctrl+V: 粘贴
    if (event == Event::CtrlV) {
        auto& clipboard = ClipboardManager::getInstance();
        if (clipboard.paste(state.currentPath)) {
            state.cached_current_path_for_entries.clear();
            g_preview_cache.key.clear();
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_dir_cache->erase(state.currentPath);
                FileManager::lru_entry_cache->erase(state.currentPath);
            }
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.batch_selected.clear();
        }
        return true;
    }

    // Delete / Ctrl+D: 打开删除确认对话框
    if (event == Event::Delete || event == Event::CtrlD) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            state.active_panel = ActivePanel::DeleteConfirm;
            state.panel_message.clear();
        }
        return true;
    }

    // Ctrl+R: 重载配置
    if (event == Event::CtrlR) {
        FTB::ConfigManager::GetInstance()->ReloadConfig();
        return true;
    }

    return false;
}

}  // namespace FTB
