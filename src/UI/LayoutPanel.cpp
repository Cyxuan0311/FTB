#include "../../include/UI/LayoutPanel.hpp"
#include "../../include/FTB/ConfigManager.hpp"

namespace FTB::UI {

using namespace ftxui;

Element RenderLayoutPanel(MainState& /*state*/, int tw, int /*th*/) {
    auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
    int bar_w = std::min(50, tw - 10);
    int pw_px = std::max(1, static_cast<int>(bar_w * cfg.layout.parent_ratio));
    int cw_px = std::max(1, static_cast<int>(bar_w * cfg.layout.current_ratio));
    int dw_px = std::max(1, bar_w - pw_px - cw_px);
    auto make_bar = [](int count, const char* block) {
        std::string s;
        for (int i = 0; i < count; ++i) s += block;
        return s;
    };
    Elements els;
    els.push_back(text(" Layout Adjustment") | color(TC("title")) | bold);
    els.push_back(separator() | color(TC("main_border")));
    els.push_back(text(" Column Width Ratios") | color(TC("main_fg")));
    els.push_back(text(""));
    els.push_back(hbox({
        text(make_bar(pw_px, u8"\u2588")) | color(TC("directory")) | bold,
        text(make_bar(cw_px, u8"\u2588")) | color(TC("main_fg")) | bold,
        text(make_bar(dw_px, u8"\u2588")) | color(TC("success")) | bold,
    }));
    els.push_back(hbox({
        text(" P:" + std::to_string(static_cast<int>(cfg.layout.parent_ratio * 100)) + "% ") | color(TC("directory")) | bold,
        text(" C:" + std::to_string(static_cast<int>(cfg.layout.current_ratio * 100)) + "% ") | color(TC("main_fg")) | bold,
        text(" R:" + std::to_string(static_cast<int>(cfg.layout.preview_ratio * 100)) + "%") | color(TC("success")) | bold,
    }));
    els.push_back(separator() | color(TC("main_border")));
    els.push_back(text(" Keys:") | color(TC("title")) | bold);
    els.push_back(text("   [1] +5% Parent   [Shift+1] -5% Parent") | color(TC("main_fg")));
    els.push_back(text("   [2] +5% Current  [Shift+2] -5% Current") | color(TC("main_fg")));
    els.push_back(text("   [3] +5% Preview  [Shift+3] -5% Preview") | color(TC("main_fg")));
    els.push_back(text("   [0] Reset to default") | color(TC("main_fg")));
    els.push_back(text(""));
    els.push_back(text(" Enter=Confirm  Esc=Cancel") | color(TC("dim")) | dim);
    return vbox(els) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, GREATER_THAN, 55) | center;
}

bool HandleLayoutEvent(MainState& state, const Event& event) {
    auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
    if (event == Event::Character('1')) {
        cfg.layout.parent_ratio = std::min(0.6, cfg.layout.parent_ratio + 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('!')) {
        cfg.layout.parent_ratio = std::max(0.05, cfg.layout.parent_ratio - 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('2')) {
        cfg.layout.current_ratio = std::min(0.6, cfg.layout.current_ratio + 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('@')) {
        cfg.layout.current_ratio = std::max(0.05, cfg.layout.current_ratio - 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('3')) {
        cfg.layout.preview_ratio = std::min(0.6, cfg.layout.preview_ratio + 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('#')) {
        cfg.layout.preview_ratio = std::max(0.05, cfg.layout.preview_ratio - 0.05);
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Character('0')) {
        cfg.layout.parent_ratio  = 2.0/9.0;
        cfg.layout.current_ratio = 4.0/9.0;
        cfg.layout.preview_ratio = 3.0/9.0;
        cfg.layout.Normalize(); return true;
    }
    if (event == Event::Return) {
        // 确认时保存配置到文件
        FTB::ConfigManager::GetInstance()->SaveConfig();
        state.active_panel = ActivePanel::None;
        return true;
    }
    return true;
}

}  // namespace FTB::UI
