#include "dialog/ImagePreviewPanel.hpp"
#include "core/MainUI.hpp"
#include "config/ThemeManager.hpp"
#include "preview/ImagePreview.hpp"
#include "protocols/ImageOutputManager.hpp"
#include "browser/FileManager.hpp"

#include <filesystem>
#include <sstream>

namespace FTB {
namespace UI {

using namespace ftxui;

static Color TC(const std::string& name) {
    return ThemeManager::GetInstance()->GetThemeColor(name);
}

Element RenderImagePreviewPanel(MainState& state, int term_width, int term_height) {
    auto& tab = state.tabManager.active();
    if (tab.viewer_filepath.empty()) return text("No image file");

    std::string filePath = tab.viewer_filepath;
    std::string filename = std::filesystem::path(filePath).filename().string();

    int panel_h = term_height;
    int content_h = panel_h - 2;
    if (content_h < 3) content_h = 3;

    Elements lines;

    Element header = hbox({
        text(" " + filename + " ") | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        filler() | bgcolor(TC("status_bg")),
        text(" [Esc] Close  [Arrow/Wheel] Scroll ") | color(Color::Grey37) | bgcolor(TC("status_bg")),
    }) | size(WIDTH, EQUAL, term_width);

    lines.push_back(header);

    auto* proto = FTB::ImageOutputManager::ActiveProtocol();
    bool use_protocol = false;

    if (proto && FTB::ImageOutputManager::IsActive()) {
        std::string dummy;
        int dummy_rows;
        if (!FTB::ImageOutputManager::GetCached(filePath, dummy, dummy_rows)) {
            FTB::ImageOutputManager::ClearCurrent();
        }
    }

    int img_w = std::max(30, term_width - 4);
    int img_h = std::max(10, content_h - 2);

    if (proto) {
        std::string img_data;
        int img_rows = 0, render_w = 0, render_h = 0;
        bool have_cached = FTB::ImageOutputManager::GetCached(filePath, img_data, img_rows, &render_w, &render_h);
        bool has_failed = FTB::ImageOutputManager::HasFailed(filePath);

        if (!have_cached && !has_failed) {
            int pixel_w = img_w * 9;
            int pixel_h = std::max(5, term_height - 5) * 18;
            if (!FTB::ImageOutputManager::IsEncoding(filePath)) {
                FTB::ImageOutputManager::EncodeAsync(filePath, pixel_w, pixel_h);
            }
        }

        if (have_cached && img_rows > 0) {
            use_protocol = true;
            int preview_start_row = 2;
            int preview_start_col = 0;
            int max_chars_h = std::max(5, term_height - 6);
            FTB::ImageOutputManager::SetPending(
                filePath, img_data, preview_start_row, preview_start_col,
                img_rows, max_chars_h, term_width, render_w, render_h);
            lines.push_back(text("  [" + FTB::ImageOutputManager::ProtocolName()
                + "] " + std::to_string(img_rows) + " rows (press Esc to close)")
                | color(TC("dim")) | dim | size(WIDTH, EQUAL, term_width));
        } else if (!has_failed) {
            use_protocol = true;
            lines.push_back(text("  Encoding image...") | color(TC("dim")) | dim);
        }
    }

    if (!use_protocol) {
        FTB::ImagePreview::LoadAsync(filePath, img_w, img_h);

        FTB::ImageCache img_cache;
        bool cached = FTB::ImagePreview::GetCached(filePath, img_cache);
        if (cached && img_cache.is_image) {
            int scroll_y = state.viewer_scroll_y;
            int visible_lines = content_h - 1;
            int total_lines = static_cast<int>(img_cache.image_lines.size());
            int start_line = std::min(scroll_y, std::max(0, total_lines - visible_lines));
            int end_line = std::min(start_line + visible_lines, total_lines);

            for (int i = start_line; i < end_line; ++i) {
                const auto& line = img_cache.image_lines[i];
                Elements char_els;
                if (!line.half_block_bottom.empty()) {
                    for (size_t x = 0; x < line.pixels.size(); ++x) {
                        auto& top = line.pixels[x];
                        auto& bot = line.half_block_bottom[x];
                        if (top.r == bot.r && top.g == bot.g && top.b == bot.b) {
                            char_els.push_back(
                                text(L"\u2588") |
                                color(Color::RGB(top.r, top.g, top.b))
                            );
                        } else {
                            char_els.push_back(
                                text(L"\u2584") |
                                color(Color::RGB(bot.r, bot.g, bot.b)) |
                                bgcolor(Color::RGB(top.r, top.g, top.b))
                            );
                        }
                    }
                } else {
                    for (const auto& p : line.pixels) {
                        char_els.push_back(
                            text(L"\u2588") |
                            color(Color::RGB(p.r, p.g, p.b))
                        );
                    }
                }
                lines.push_back(hbox(char_els));
            }

            // Fill remaining space
            for (int i = end_line; i < start_line + visible_lines; ++i) {
                lines.push_back(text(""));
            }

            std::string info = "  " + filename + "  " +
                std::to_string(img_cache.width) + "x" + std::to_string(img_cache.height) + "px  "
                + "(" + std::to_string(total_lines) + " rows)";
            if (total_lines > visible_lines) {
                info += "  [scroll: " + std::to_string(start_line) + "/" + std::to_string(total_lines - visible_lines) + "]";
            }
            lines.push_back(text(info) | color(TC("dim")) | dim);
        } else if (cached && img_cache.failed) {
            lines.push_back(text("  Failed to load image preview") | color(Color::Red) | center);
        } else {
            lines.push_back(text("  Loading image...") | color(TC("dim")) | dim | center);
        }
    }

    if (lines.size() < 2) {
        lines.push_back(text("") | size(HEIGHT, EQUAL, content_h));
    }

    return vbox(std::move(lines)) | bgcolor(TC("main_bg")) | flex;
}

bool HandleImagePreviewEvent(MainState& state, ftxui::Event& event) {
    auto& tab = state.tabManager.active();
    if (tab.type != TabType::ImagePreview) return false;

    if (event == Event::Escape) {
        auto* proto = FTB::ImageOutputManager::ActiveProtocol();
        if (proto) {
            FTB::ImageOutputManager::ClearCurrent();
        }
        state.saveTabState();
        int idx = state.tabManager.activeIndex();
        if (state.tabManager.isImageTab(idx) && state.tabManager.canClose()) {
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        if (state.tabManager.isImageTab(idx) && !state.tabManager.canClose()) {
            const char* home = std::getenv("HOME");
            state.tabManager.createTab(home ? home : "/");
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        return false;
    }

    // Scrolling
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.viewer_scroll_y > 0) {
            state.viewer_scroll_y -= 1;
        }
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        state.viewer_scroll_y += 1;
        return true;
    }
    if (event.is_mouse()) {
        if (event.mouse().button == Mouse::WheelUp) {
            if (state.viewer_scroll_y > 0) {
                state.viewer_scroll_y = std::max(0, state.viewer_scroll_y - 3);
            }
            return true;
        }
        if (event.mouse().button == Mouse::WheelDown) {
            state.viewer_scroll_y += 3;
            return true;
        }
    }

    return false;
}

}
}
