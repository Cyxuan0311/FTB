#include "../../include/dialog/NewFolderDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderNewFolderPanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(55, tw - 4);
    return vbox({
        text(" New Folder") | color(TC("title")) | bold,
        separator(),
        hbox({ text(" Folder name: ") | color(TC("main_fg")), text(state.panel_input + "\u258f") | color(TC("find_keyword")) }),
        text(state.panel_message) | color(TC("error")),
        text(" Enter=Confirm  Esc=Cancel") | color(TC("dim")) | dim,
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleNewFolderEvent(MainState& state, const Event& event) {
    if (event == Event::Return) {
        if (!state.panel_input.empty()) {
            fs::path newPath = fs::path(state.currentPath) / state.panel_input;
            if (FileManager::createDirectory(newPath.string())) {
                state.cached_current_path_for_entries.clear();
                InvalidatePreviewCache();
                {
                    std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
                    FileManager::lru_dir_cache->erase(state.currentPath);
                    FileManager::lru_entry_cache->erase(state.currentPath);
                }
                state.allContents = FileManager::getDirectoryContents(state.currentPath);
                state.filteredContents = state.allContents;
                state.active_panel = ActivePanel::None;
                state.panel_input.clear();
                state.panel_message.clear();
            } else {
                state.panel_message = " Create folder failed!";
            }
        }
        return true;
    }
    if (event == Event::Backspace) {
        if (!state.panel_input.empty()) state.panel_input.pop_back();
        return true;
    }
    if (event.is_character()) {
        state.panel_input += event.character();
        return true;
    }
    return true;
}

}  // namespace FTB::UI
