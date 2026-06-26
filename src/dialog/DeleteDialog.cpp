#include "../../include/dialog/DeleteDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderDeleteConfirmPanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(55, tw - 4);

    Elements els;

    if (!state.batch_selected.empty()) {
        els.push_back(
            text(" Delete " + std::to_string(state.batch_selected.size()) + " items?")
            | color(TC("error")) | bold
        );
        els.push_back(separator());

        int count = 0;
        for (int idx : state.batch_selected) {
            if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                if (count >= 5) {
                    els.push_back(
                        text("  ... and " + std::to_string(state.batch_selected.size() - 5) + " more")
                        | color(TC("dim")) | dim
                    );
                    break;
                }
                els.push_back(
                    hbox({
                        text("  "),
                        text(state.filteredContents[idx]) | color(TC("main_fg"))
                    })
                );
                count++;
            }
        }
    } else {
        std::string item_name;
        bool is_dir = false;
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            item_name = state.filteredContents[state.selected];
            fs::path item_path = fs::path(state.currentPath) / item_name;
            is_dir = fs::is_directory(item_path);
        }

        std::string type_label = is_dir ? "directory" : "file";
        els.push_back(
            text(" Delete " + type_label + "?") | color(TC("error")) | bold
        );
        els.push_back(separator());
        els.push_back(
            hbox({ text("  ") | color(TC("dim")),
                   text(item_name) | color(TC("main_fg")) | bold })
        );
    }

    els.push_back(text(""));
    els.push_back(text(state.panel_message) | color(TC("error")));
    els.push_back(text(" Enter=Confirm  Esc=Cancel") | color(TC("dim")) | dim);

    return vbox(std::move(els)) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

static void RefreshDir(MainState& state) {
    state.cached_current_path_for_entries.clear();
    InvalidatePreviewCache();
    {
        std::lock_guard<std::mutex> lock(FileManager::cache_mutex);
        FileManager::lru_dir_cache->erase(state.currentPath);
        FileManager::lru_entry_cache->erase(state.currentPath);
    }
    state.allContents = FileManager::getDirectoryContents(state.currentPath);
    state.filteredContents = state.allContents;
    if (state.selected >= static_cast<int>(state.filteredContents.size())) {
        state.selected = std::max(0, static_cast<int>(state.filteredContents.size()) - 1);
    }
}

bool HandleDeleteConfirmEvent(MainState& state, const Event& event) {
    if (event == Event::Return) {
        try {
            if (!state.batch_selected.empty()) {
                for (int idx : state.batch_selected) {
                    if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                        fs::path item_path = fs::path(state.currentPath) / state.filteredContents[idx];
                        if (fs::is_directory(item_path))
                            fs::remove_all(item_path);
                        else
                            fs::remove(item_path);
                    }
                }
                state.batch_selected.clear();
            } else if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
                fs::path item_path = fs::path(state.currentPath) / state.filteredContents[state.selected];
                if (fs::is_directory(item_path))
                    fs::remove_all(item_path);
                else
                    fs::remove(item_path);
            }
            RefreshDir(state);
        } catch (const std::exception& e) {
            state.panel_message = " Delete failed: " + std::string(e.what());
            return true;
        }
        state.active_panel = ActivePanel::None;
        state.panel_message.clear();
        return true;
    }

    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_message.clear();
        return true;
    }

    return true;
}

}  // namespace FTB::UI
