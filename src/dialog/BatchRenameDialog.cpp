#include "../../include/dialog/BatchRenameDialog.hpp"
#include "../../include/browser/FileManager.hpp"
#include "../../include/config/ConfigManager.hpp"
#include "../../include/utils/StatusMessage.hpp"
#include <filesystem>
#include <regex>
#include <set>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

static std::vector<std::pair<std::string, std::string>> GetBatchRenameItems(MainState& state) {
    std::vector<std::pair<std::string, std::string>> items;
    if (!state.batch_selected.empty()) {
        for (int idx : state.batch_selected) {
            if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size())) {
                items.push_back({state.filteredContents[idx], ""});
            }
        }
    } else if (state.selected >= 0 &&
               state.selected < static_cast<int>(state.filteredContents.size())) {
        items.push_back({state.filteredContents[state.selected], ""});
    }
    return items;
}

static std::string ApplyRegex(const std::string& name,
                               const std::string& pattern,
                               const std::string& replacement) {
    try {
        std::regex re(pattern);
        return std::regex_replace(name, re, replacement);
    } catch (const std::regex_error&) {
        return name;
    }
}

static void ComputePreview(MainState& state) {
    state.batchrename_preview.clear();
    auto items = GetBatchRenameItems(state);
    for (auto& item : items) {
        std::string new_name = ApplyRegex(item.first,
                                           state.batchrename_pattern,
                                           state.batchrename_replacement);
        if (new_name != item.first && !new_name.empty()) {
            state.batchrename_preview.push_back(item.first + " -> " + new_name);
        } else if (new_name.empty()) {
            state.batchrename_preview.push_back(item.first + " -> (empty - skipped)");
        }
    }
}

Element RenderBatchRenamePanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(60, tw - 4);
    auto items = GetBatchRenameItems(state);

    Elements rows;
    rows.push_back(
        hbox({
            text(" Batch Rename") | color(TC("title")) | bold,
            text("  (" + std::to_string(items.size()) + " items)") | color(TC("dim")),
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    rows.push_back(text(" Pattern:"));
    std::string p_marker = (state.batchrename_field == 0) ? " > " : "   ";
    rows.push_back(
        hbox({
            text(p_marker) | color(TC("indicator")),
            text(state.batchrename_pattern + "_") | color(TC("syn_keyword")),
        })
    );

    rows.push_back(text(" Replacement:"));
    std::string r_marker = (state.batchrename_field == 1) ? " > " : "   ";
    rows.push_back(
        hbox({
            text(r_marker) | color(TC("indicator")),
            text(state.batchrename_replacement + "_") | color(TC("find_keyword")),
        })
    );

    if (!state.batchrename_pattern.empty()) {
        ComputePreview(state);
        if (!state.batchrename_preview.empty()) {
            rows.push_back(separator() | color(TC("main_border")));
            rows.push_back(text(" Preview:") | color(TC("dim")));
            int count = 0;
            for (const auto& line : state.batchrename_preview) {
                if (count >= 10) {
                    rows.push_back(
                        text("  ... and " + std::to_string(state.batchrename_preview.size() - 10) + " more")
                        | color(TC("dim")));
                    break;
                }
                rows.push_back(text("  " + line) | color(TC("main_fg")));
                count++;
            }
        } else {
            rows.push_back(text(" (no changes)") | color(TC("dim")));
        }
    }

    rows.push_back(text(""));
    rows.push_back(text(state.panel_message) | color(TC("error")));
    rows.push_back(separator() | color(TC("main_border")));
    rows.push_back(
        hbox({
            text(" Tab Switch  ") | color(TC("syn_keyword")) | bold,
            text("Enter Execute  ") | color(TC("syn_keyword")) | bold,
            text("Esc Cancel") | color(TC("dim")),
            filler()
        })
    );

    return vbox(std::move(rows)) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleBatchRenameEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.batchrename_pattern.clear();
        state.batchrename_replacement.clear();
        state.batchrename_field = 0;
        state.batchrename_preview.clear();
        state.panel_message.clear();
        return true;
    }

    if (event == Event::Tab) {
        state.batchrename_field = 1 - state.batchrename_field;
        return true;
    }

    if (event == Event::Return) {
        auto items = GetBatchRenameItems(state);
        if (items.empty()) {
            state.panel_message = "No files selected";
            return true;
        }

        if (state.batchrename_pattern.empty()) {
            state.panel_message = "Pattern cannot be empty";
            return true;
        }

        std::regex re;
        try {
            re = std::regex(state.batchrename_pattern);
        } catch (const std::regex_error& e) {
            state.panel_message = std::string("Invalid regex: ") + e.what();
            return true;
        }

        std::set<std::string> used_names;
        size_t renamed = 0;
        size_t skipped = 0;

        for (const auto& item : items) {
            std::string new_name = std::regex_replace(item.first, re, state.batchrename_replacement);
            if (new_name.empty() || new_name == item.first) {
                skipped++;
                continue;
            }

            fs::path old_path = fs::path(state.currentPath) / item.first;
            fs::path new_path = fs::path(state.currentPath) / new_name;

            if (used_names.count(new_name)) {
                skipped++;
                continue;
            }
            used_names.insert(new_name);

            if (fs::exists(new_path)) {
                skipped++;
                continue;
            }

            if (FileManager::renameFileOrDirectory(old_path.string(), new_path.string())) {
                renamed++;
            } else {
                skipped++;
            }
        }

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
        if (state.selected >= static_cast<int>(state.filteredContents.size())) {
            state.selected = std::max(0, static_cast<int>(state.filteredContents.size()) - 1);
        }

        state.active_panel = ActivePanel::None;
        state.batchrename_pattern.clear();
        state.batchrename_replacement.clear();
        state.batchrename_field = 0;
        state.batchrename_preview.clear();
        state.panel_message.clear();

        StatusMessage::Show("Renamed " + std::to_string(renamed) +
                            " item(s), skipped " + std::to_string(skipped));
        return true;
    }

    if (event == Event::Backspace) {
        if (state.batchrename_field == 0 && !state.batchrename_pattern.empty()) {
            state.batchrename_pattern.pop_back();
        } else if (state.batchrename_field == 1 && !state.batchrename_replacement.empty()) {
            state.batchrename_replacement.pop_back();
        }
        return true;
    }

    if (event.is_character()) {
        char c = event.character()[0];
        if (state.batchrename_field == 0) {
            state.batchrename_pattern += c;
        } else {
            state.batchrename_replacement += c;
        }
        return true;
    }

    return true;
}

}  // namespace FTB::UI
