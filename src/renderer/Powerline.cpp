#include "../../include/renderer/Powerline.hpp"
#include "../../include/renderer/detail_element.hpp"

namespace FTB {

using namespace ftxui;

// ---- 构建 Powerline 段落 (右箭头) ----
Element PowerlineSegment(
    const std::string& content,
    Color bg_color,
    Color fg_color,
    Color next_bg_color,
    bool bold_flag,
    const std::string& separator
) {
    auto text_el = text(" " + content + " ") | color(fg_color);
    if (bold_flag) text_el = text_el | bold;

    if (separator.empty()) {
        return hbox({
            text_el | bgcolor(bg_color),
            filler() | bgcolor(next_bg_color)
        });
    }

    return hbox({
        text_el | bgcolor(bg_color),
        text(separator) | color(bg_color) | bgcolor(next_bg_color)
    });
}

// ---- 构建 Powerline 标题栏 ----
Element PowerlineHeader(
    const std::string& title,
    Color bg_color,
    Color fg_color,
    int width
) {
    auto title_el = hbox({
        text(" " + title + " ") | color(fg_color) | bold | bgcolor(bg_color),
        text(PowerlineChars::RightArrow) | color(bg_color) | bgcolor(TC("main_bg"))
    });

    if (width > 0) {
        return title_el | size(WIDTH, EQUAL, width);
    }
    return title_el;
}

// ---- 构建 Powerline 状态栏 ----
Element PowerlineStatusBar(Elements segments) {
    return hbox(segments) | bgcolor(TC("status_bg"));
}

// ---- 构建 Powerline 左侧段落 (右箭头) ----
Element PowerlineSegmentLeft(
    const std::string& content,
    Color bg_color,
    Color fg_color,
    Color next_bg_color,
    bool bold_flag,
    const std::string& separator
) {
    return PowerlineSegment(content, bg_color, fg_color, next_bg_color, bold_flag, separator);
}

// ---- 构建 Powerline 右侧段落 (左箭头) ----
Element PowerlineSegmentRight(
    const std::string& content,
    Color bg_color,
    Color fg_color,
    Color prev_bg_color,
    bool bold_flag,
    const std::string& separator
) {
    auto text_el = text(" " + content + " ") | color(fg_color);
    if (bold_flag) text_el = text_el | bold;

    if (separator.empty()) {
        return hbox({
            filler() | bgcolor(prev_bg_color),
            text_el | bgcolor(bg_color)
        });
    }

    return hbox({
        text(separator) | color(bg_color) | bgcolor(prev_bg_color),
        text_el | bgcolor(bg_color)
    });
}

// ---- 根据风格名称获取状态栏分隔符字符 ----
const char* GetStatusBarSeparator(const std::string& style) {
    if (style == "powerline")   return PowerlineChars::RightArrow;
    if (style == "rounded")     return PowerlineChars::RightRound;
    if (style == "thin")        return PowerlineChars::RightArrowThin;
    if (style == "arrow")       return "\u25b8";
    if (style == "bar")         return "\u2502";
    if (style == "minimal")     return "";
    return PowerlineChars::RightArrow;
}

}  // namespace FTB
