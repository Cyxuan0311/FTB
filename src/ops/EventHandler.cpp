#include "ops/EventHandler.hpp"
#include "browser/ClipboardManager.hpp"
#include "browser/FileManager.hpp"
#include "config/ConfigManager.hpp"
#include "utils/StatusMessage.hpp"

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
    // d: 移入回收站
    if (event == Event::Character('d')) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            size_t success = 0;
            for (const auto& p : paths) {
                if (FileManager::moveToTrash(p))
                    success++;
            }
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            {
                std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                FileManager::lru_dir_cache->erase(state.currentPath);
                FileManager::lru_entry_cache->erase(state.currentPath);
            }
            UpdateCurrentEntryCache(state);
            state.filteredContents = state.allContents;
            if (state.selected >= static_cast<int>(state.filteredContents.size()))
                state.selected = std::max(0, static_cast<int>(state.filteredContents.size()) - 1);
            state.batch_selected.clear();
            if (success > 0)
                StatusMessage::Show("Moved " + std::to_string(success) + " item(s) to trash");
            else
                StatusMessage::Show("Trash failed: unable to move file(s)");
        }
        return true;
    }

    // y: 复制选中项 (vim yank)
    if (event == Event::Character('y')) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            auto& clipboard = ClipboardManager::getInstance();
            clipboard.clear();
            clipboard.setCutMode(false);
            for (const auto& p : paths) {
                clipboard.addItem(p);
            }
            StatusMessage::Show(std::to_string(paths.size()) + " item(s) copied");
        }
        return true;
    }

    // x: 剪切选中项
    if (event == Event::Character('x')) {
        auto paths = GetBatchPaths(state);
        if (!paths.empty()) {
            auto& clipboard = ClipboardManager::getInstance();
            clipboard.clear();
            clipboard.setCutMode(true);
            for (const auto& p : paths) {
                clipboard.addItem(p);
            }
            StatusMessage::Show(std::to_string(paths.size()) + " item(s) cut");
        }
        return true;
    }

    // p: 粘贴 (vim paste)
    if (event == Event::Character('p')) {
        auto& clipboard = ClipboardManager::getInstance();
        if (clipboard.paste(state.currentPath)) {
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
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
