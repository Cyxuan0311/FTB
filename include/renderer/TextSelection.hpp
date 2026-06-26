#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace FTB {

struct TextSelectionState {
    bool active = false;
    int anchor_y = 0, anchor_x = 0;
    int current_y = 0, current_x = 0;
    std::vector<std::string> lines;
    int scroll_y = 0;
};

enum class SelectionColumn { Parent, Current, Preview };

// Single ODR-safe instances across all translation units (C++17 inline)
inline TextSelectionState g_parent_sel;
inline TextSelectionState g_current_sel;
inline TextSelectionState g_preview_sel;

// Reflect boxes — defined in TextSelection.cpp
extern ftxui::Box g_parent_box;
extern ftxui::Box g_current_box;
extern ftxui::Box g_preview_box;

// Preview-specific line number width (for x-offset calculation in SelectionRelease)
inline int g_preview_line_num_width = 1;
inline int g_preview_max_lines = 10;

// Column where current selection started (set on Press, used by Drag/Release)
inline SelectionColumn g_active_column = SelectionColumn::Preview;

// Determine column from mouse x-coordinate
SelectionColumn GetColumnAtX(int mouse_x, int parent_width, int sep_width, int current_width);

// Selection operations
void SelectionPress(SelectionColumn col, int mx, int my);   // on Pressed: start selection
void SelectionDrag(int mx, int my);                          // on Moved: update coordinates
std::string SelectionRelease();                              // on Released: extract text, clean up

} // namespace FTB
