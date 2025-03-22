#include "../include/FTB/Vim_Like.hpp"
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <sstream>
#include <chrono>  // 添加头文件

using namespace ftxui;

const int MAX_VISIBLE_LINES = 35; // 设置最大可见行数

VimLikeEditor::VimLikeEditor()
    : edit_mode_(false), cursor_line_(0), cursor_col_(0), scroll_offset_(0) {
    lines_.push_back("");  // 至少一行空文本
    original_lines_ = lines_; // 初始化原始内容
}

VimLikeEditor::~VimLikeEditor() {}

void VimLikeEditor::SetOnExit(std::function<void(const std::vector<std::string>&)> on_exit) {
    on_exit_ = on_exit;
}

void VimLikeEditor::SetContent(const std::vector<std::string>& new_lines) {
    lines_ = new_lines;
    original_lines_ = new_lines; // 保存原始内容
    if (lines_.empty()) {
        lines_.push_back("");
        original_lines_.push_back("");
    }
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_offset_ = 0;
}

void VimLikeEditor::EnterEditMode() {
    if (lines_.empty()) {
        lines_.push_back("");
    }
    // 进入 Vim 界面时保存一份原始内容
    original_lines_ = lines_;
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_offset_ = 0;
    // 默认进入查看模式，需按 i 进入编辑模式
}

Element VimLikeEditor::Render() {
    Elements rendered_lines;
    int total_lines = static_cast<int>(lines_.size());
    int end_line = std::min(scroll_offset_ + MAX_VISIBLE_LINES, total_lines);

    // 根据当前时间计算闪烁状态：每500毫秒切换一次
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    bool blink_on = ((ms / 500) % 2 == 0);

    for (int i = scroll_offset_; i < end_line; ++i) {
        if (edit_mode_ && i == cursor_line_) {
            std::string left = lines_[i].substr(0, cursor_col_);
            std::string right = lines_[i].substr(cursor_col_);
            rendered_lines.push_back(
                hbox({
                    text(left),
                    // 根据 blink_on 状态显示光标或者空格，从而达到闪烁效果
                    (blink_on ? text("|") : text(" ")) | color(Color::BlueLight),
                    text(right)
                })
            );
        } else {
            rendered_lines.push_back(text(lines_[i]));
        }
    }

    // 根据当前模式显示状态栏
    std::string mode_text = edit_mode_ ? "📝 编辑模式" : "👀 查看模式";

    return vbox({
        text("Vim-Like Editor - " + mode_text) | bold | center | bgcolor(Color::Green),
        separator(),
        vbox(rendered_lines) | border,
        text("提示：按 i 开始编辑 | ESC 退出编辑模式 | Ctrl+D 保存退出 | Ctrl+F 放弃退出")
            | borderHeavy | color(Color::Orange3) | center
    }) | border;
}

bool VimLikeEditor::OnEvent(Event event) {
    // 按 Ctrl+E 进入 Vim 界面，但默认进入查看模式
    if (event == Event::CtrlE) {
        edit_mode_ = false;
        return true;
    }

    // 如果当前不是编辑模式，则只有按 'i' 才进入编辑模式
    if (!edit_mode_) {
        if (event.is_character() && event.character() == "i") {
            edit_mode_ = true;
            return true;
        }
        return false;
    }

    // 按 ESC 退出编辑模式，但不关闭 Vim 界面
    if (event == Event::Escape) {
        edit_mode_ = false;
        return true;
    }

    // Ctrl+D 保存修改并退出 Vim 界面
    if (event == Event::CtrlD) {
        edit_mode_ = false;
        if (on_exit_) {
            on_exit_(lines_); // 返回修改后的内容
        }
        return true;
    }

    // Ctrl+F 退出 Vim 界面，放弃修改，恢复原始内容
    if (event == Event::CtrlF) {
        edit_mode_ = false;
        if (on_exit_) {
            on_exit_(original_lines_); // 返回原始内容，放弃修改
        }
        return true;
    }

    // 以下处理编辑模式下的其他按键
    if (event == Event::Backspace) {
        if (cursor_col_ > 0) {
            lines_[cursor_line_].erase(cursor_col_ - 1, 1);
            cursor_col_--;
        } else if (cursor_line_ > 0) {
            cursor_col_ = lines_[cursor_line_ - 1].size();
            lines_[cursor_line_ - 1] += lines_[cursor_line_];
            lines_.erase(lines_.begin() + cursor_line_);
            cursor_line_--;
        }
        return true;
    }
    if (event == Event::ArrowLeft) {
        if (cursor_col_ > 0)
            cursor_col_--;
        return true;
    }
    if (event == Event::ArrowRight) {
        if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size()))
            cursor_col_++;
        return true;
    }
    if (event == Event::ArrowUp) {
        if (cursor_line_ > 0) {
            cursor_line_--;
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_--;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            cursor_line_++;
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
            if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
                scroll_offset_++;
            }
        }
        return true;
    }
    if (event == Event::Return) {
        std::string current_line = lines_[cursor_line_];
        std::string new_line = current_line.substr(cursor_col_);
        lines_[cursor_line_].resize(cursor_col_);
        lines_.insert(lines_.begin() + cursor_line_ + 1, new_line);
        cursor_line_++;
        cursor_col_ = 0;
        if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
            scroll_offset_++;
        }
        return true;
    }
    if (event.is_character()) {
        char ch = event.character()[0];
        lines_[cursor_line_].insert(cursor_col_, 1, ch);
        cursor_col_++;
        return true;
    }
    return false;
}

void VimLikeEditor::MoveCursorTo(int line, int col) {
    cursor_line_ = std::max(0, std::min(line, static_cast<int>(lines_.size()) - 1));
    cursor_col_ = std::max(0, std::min(col, static_cast<int>(lines_[cursor_line_].size())));
    if (cursor_line_ < scroll_offset_) {
        scroll_offset_ = cursor_line_;
    } else if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
        scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
    }
}
