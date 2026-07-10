#include "dialog/NewTabDialog.hpp"
#include "browser/FileManager.hpp"
#include "preview/PreviewCache.hpp"
#include "core/TabManager.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

namespace {

std::vector<std::string> GetCompletions(const std::string& input) {
    if (input.empty()) return {};
    std::error_code ec;
    std::string base;
    std::string prefix;
    if (input.back() == '/') {
        base = input;
        prefix = "";
    } else {
        auto p = fs::path(input);
        auto parent = p.parent_path();
        if (parent.empty() || parent.string() == "." || parent.string() == "") {
            base = ".";
            prefix = input;
        } else {
            base = parent.string();
            if (base.empty()) base = ".";
            prefix = p.filename().string();
        }
    }
    if (base == "~") {
        auto home = std::getenv("HOME");
        if (!home) return {};
        base = home;
    } else if (base.size() > 1 && base[0] == '~' && base[1] == '/') {
        auto home = std::getenv("HOME");
        if (!home) return {};
        base = std::string(home) + base.substr(1);
    }
    std::vector<std::string> matches;
    for (auto it = fs::directory_iterator(base, ec); it != fs::end(it); it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        auto name = it->path().filename().string();
        if (name.rfind(prefix, 0) == 0) {
            bool is_dir = it->is_directory(ec);
            matches.push_back(is_dir ? (name + "/") : name);
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

static void UpdateSuggestion(MainState& state) {
    if (state.panel_input.empty()) {
        state.panel_suggestion.clear();
        return;
    }
    auto completions = GetCompletions(state.panel_input);
    if (completions.empty()) {
        state.panel_suggestion.clear();
        return;
    }
    std::string full;
    if (state.panel_input.back() == '/') {
        full = state.panel_input + completions[0];
    } else {
        auto p = fs::path(state.panel_input);
        auto parent = p.parent_path();
        if (parent.empty() || parent.string() == "." || parent.string() == "") {
            full = completions[0];
        } else {
            full = parent.string() + "/" + completions[0];
        }
    }
    if (full.size() > state.panel_input.size()) {
        state.panel_suggestion = full.substr(state.panel_input.size());
    } else {
        state.panel_suggestion.clear();
    }
}

} // anonymous namespace

Element RenderNewTabPanel(MainState& state, int tw, int /*th*/) {
    int pw = std::min(50, tw - 4);
    Elements els;
    els.push_back(text(" New Tab") | color(TC("title")) | bold);
    els.push_back(separator());

    if (!state.panel_suggestion.empty()) {
        els.push_back(hbox({
            text(" Path: ") | color(TC("main_fg")),
            text(state.panel_input) | color(TC("find_keyword")),
            text("\u258f") | color(TC("find_keyword")),
            text(state.panel_suggestion) | color(TC("dim")) | dim,
        }));
    } else {
        els.push_back(hbox({
            text(" Path: ") | color(TC("main_fg")),
            text(state.panel_input + "\u258f") | color(TC("find_keyword")),
        }));
    }

    els.push_back(text(state.panel_message) | color(TC("error")));
    els.push_back(text(" Enter=Open Tab  Tab=Complete  \u2192=Accept  Esc=Cancel") | color(TC("dim")) | dim);
    return vbox(els) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleNewTabEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_suggestion.clear();
        return true;
    }

    if (event == Event::Return) {
        std::string path_input = state.panel_input;
        if (path_input.empty()) path_input = state.currentPath;

        try {
            if (path_input == "~") {
                auto home = std::getenv("HOME");
                if (home) path_input = home;
            } else if (path_input.size() > 1 && path_input[0] == '~' && path_input[1] == '/') {
                auto home = std::getenv("HOME");
                if (home) path_input = std::string(home) + path_input.substr(1);
            }

            if (!fs::exists(path_input)) {
                state.panel_message = " Path does not exist!";
                return true;
            }
            fs::path target = fs::canonical(path_input);
            if (!fs::is_directory(target)) {
                state.panel_message = " Not a directory!";
                return true;
            }

            state.saveTabState();
            state.tabManager.createTab(target.string());
            state.loadTabState();

            state.active_panel = ActivePanel::None;
            state.panel_input.clear();
            state.panel_message.clear();
            state.panel_suggestion.clear();
        } catch (...) {
            state.panel_message = " Invalid path!";
        }
        return true;
    }

    if (event == Event::Tab) {
        auto completions = GetCompletions(state.panel_input);
        if (completions.empty()) return true;

        state.panel_selected = (state.panel_selected + 1) % static_cast<int>(completions.size());

        std::string full;
        if (state.panel_input.back() == '/') {
            full = state.panel_input + completions[state.panel_selected];
        } else {
            auto p = fs::path(state.panel_input);
            auto parent = p.parent_path();
            if (parent.empty() || parent.string() == "." || parent.string() == "") {
                full = completions[state.panel_selected];
            } else {
                full = parent.string() + "/" + completions[state.panel_selected];
            }
        }

        state.panel_input = full;
        state.panel_message.clear();
        UpdateSuggestion(state);
        return true;
    }

    if (event == Event::ArrowRight) {
        if (!state.panel_suggestion.empty()) {
            state.panel_input += state.panel_suggestion;
            state.panel_suggestion.clear();
        }
        return true;
    }

    if (event == Event::Backspace) {
        if (!state.panel_input.empty()) {
            state.panel_input.pop_back();
            state.panel_selected = 0;
        }
        UpdateSuggestion(state);
        return true;
    }

    if (event.is_character()) {
        state.panel_input += event.character();
        state.panel_selected = 0;
        state.panel_message.clear();
        UpdateSuggestion(state);
        return true;
    }

    return true;
}

}  // namespace FTB::UI
