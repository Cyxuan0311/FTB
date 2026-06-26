#include "dialog/ClipboardDialog.hpp"
#include "browser/ClipboardManager.hpp"
#ifdef FTB_ENABLE_ICONS
#include "renderer/IconMapper.hpp"
#endif
#include <filesystem>

namespace fs = std::filesystem;

namespace FTB::UI {

using namespace ftxui;

Element RenderClipboardPanel(MainState& state, int tw, int /*th*/) {
    auto& clipboard = ClipboardManager::getInstance();
    const auto& items = clipboard.getItems();

    int pw = std::min(50, tw - 4);
    Elements els;

    std::string mode_str = !items.empty() && clipboard.hasModeSelected()
        ? (clipboard.isCutMode() ? " CUT " : " COPY ")
        : "";
    Color mode_color = clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied");

    els.push_back(hbox({
        text(" Clipboard") | color(TC("title")) | bold,
        filler(),
        text(mode_str) | color(mode_color) | bold,
    }));
    els.push_back(separator());

    if (items.empty()) {
        els.push_back(text("  (empty)") | color(TC("dim")));
    } else {
        int max_visible = std::max(1, (tw - 6) / 2);
        int total = static_cast<int>(items.size());
        int scroll = state.panel_selected;
        if (scroll < 0) scroll = 0;
        if (scroll >= total) scroll = total - 1;

        int start = std::max(0, scroll - max_visible / 2);
        int end = std::min(total, start + max_visible);

        for (int i = start; i < end; ++i) {
            bool is_selected = (i == scroll);
            fs::path p(items[i]);
            auto filename = p.filename().string();
#ifdef FTB_ENABLE_ICONS
            auto icon = FTB::Icons::GetIconForPath(p, fs::is_directory(p));
#else
            std::string icon = fs::is_directory(p) ? "d " : "  ";
#endif
            auto line = text("  " + icon + filename);

            if (is_selected) {
                line = line | color(TC("main_bg")) | bgcolor(TC("find_keyword"));
            } else if (clipboard.hasModeSelected()) {
                line = line | color(clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied"));
            } else {
                line = line | color(TC("main_fg"));
            }
            els.push_back(line);
        }
    }

    els.push_back(separator());
    els.push_back(hbox({
        text(" Total: " + std::to_string(items.size()) + " items") | color(TC("dim")),
        filler(),
    }));

    els.push_back(text(" Esc=Close  j/k/Arrows=Scroll") | color(TC("dim")) | dim);

    return vbox(els) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | center;
}

bool HandleClipboardEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        state.panel_selected = 0;
        return true;
    }

    auto& clipboard = ClipboardManager::getInstance();
    const auto& items = clipboard.getItems();
    int total = static_cast<int>(items.size());
    if (total == 0) return true;

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) state.panel_selected--;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < total - 1) state.panel_selected++;
        return true;
    }
    return true;
}

}  // namespace FTB::UI
