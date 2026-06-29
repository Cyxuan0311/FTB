#include "../../include/dialog/NewFileDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderNewFilePanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(50, tw - 4);
    return vbox({
        text(" New File") | color(TC("title")) | bold,
        separator() | color(TC("main_border")),
        text(""),
        hbox({
            text(" "),
            text(state.panel_input + "\u258f") | color(TC("find_keyword")),
        }),
        text(""),
        (!state.panel_message.empty()
            ? text(" " + state.panel_message) | color(TC("error"))
            : text("")),
        (!state.panel_message.empty() ? text("") : text("")),
        hbox({
            text(" "),
            text("[Enter]") | color(TC("dim")) | dim,
            text(" Confirm") | color(TC("dim")) | dim,
            text("    "),
            text("[Esc]") | color(TC("dim")) | dim,
            text(" Cancel") | color(TC("dim")) | dim,
            filler(),
        }),
    }) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleNewFileEvent(MainState& state, const Event& event) {
    if (event == Event::Return) {
        if (!state.panel_input.empty()) {
            fs::path newPath = fs::path(state.currentPath) / state.panel_input;
            if (FileManager::createFile(newPath.string())) {
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
                state.panel_message = " Create file failed!";
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
