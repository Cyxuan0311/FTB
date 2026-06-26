#include "renderer/TextSelection.hpp"

namespace FTB {

ftxui::Box g_parent_box;
ftxui::Box g_current_box;
ftxui::Box g_preview_box;

// ---- column helpers ----

static TextSelectionState& StateForCol(SelectionColumn col) {
    switch (col) {
        case SelectionColumn::Parent:  return g_parent_sel;
        case SelectionColumn::Current: return g_current_sel;
        case SelectionColumn::Preview: return g_preview_sel;
    }
    return g_preview_sel;
}

static ftxui::Box& BoxForCol(SelectionColumn col) {
    switch (col) {
        case SelectionColumn::Parent:  return g_parent_box;
        case SelectionColumn::Current: return g_current_box;
        case SelectionColumn::Preview: return g_preview_box;
    }
    return g_preview_box;
}

// ---- public API ----

SelectionColumn GetColumnAtX(int mx, int pw, int sep_w, int cw) {
    if (mx < pw) return SelectionColumn::Parent;
    if (mx < pw + sep_w + cw) return SelectionColumn::Current;
    return SelectionColumn::Preview;
}

void SelectionPress(SelectionColumn col, int mx, int my) {
    auto& s = StateForCol(col);
    s.active = true;
    s.anchor_x = mx; s.anchor_y = my;
    s.current_x = mx; s.current_y = my;
    g_active_column = col;
}

void SelectionDrag(int mx, int my) {
    auto& s = StateForCol(g_active_column);
    if (!s.active) return;
    s.current_x = mx;
    s.current_y = my;
}

std::string SelectionRelease() {
    auto& s = StateForCol(g_active_column);
    if (!s.active) { s.active = false; return {}; }
    s.active = false;

    if (s.lines.empty()) return {};

    auto& box = BoxForCol(g_active_column);

    int scroll_offset = (g_active_column == SelectionColumn::Preview) ? s.scroll_y : 0;
    int ly1 = s.anchor_y - box.y_min + scroll_offset;
    int ly2 = s.current_y - box.y_min + scroll_offset;
    int line_start = std::max(0, std::min(ly1, ly2));
    int line_end   = std::min(static_cast<int>(s.lines.size()) - 1, std::max(ly1, ly2));
    if (line_start > line_end) return {};

    int x_off = (g_active_column == SelectionColumn::Preview) ? (g_preview_line_num_width + 1) : 0;
    int lx1 = s.anchor_x - box.x_min - x_off;
    int lx2 = s.current_x - box.x_min - x_off;
    int col_start = std::max(0, std::min(lx1, lx2));
    int col_end   = std::max(0, std::max(lx1, lx2));

    std::string result;
    for (int i = line_start; i <= line_end; ++i) {
        const auto& line = s.lines[i];
        int line_len = static_cast<int>(line.size());
        if (i == line_start && i == line_end) {
            if (col_start < line_len)
                result += line.substr(col_start, std::min(col_end - col_start + 1, line_len - col_start));
        } else if (i == line_start) {
            if (col_start < line_len)
                result += line.substr(col_start);
        } else if (i == line_end) {
            result += line.substr(0, std::min(col_end + 1, line_len));
        } else {
            result += line;
        }
        if (i < line_end) result += "\n";
    }
    return result;
}

} // namespace FTB
