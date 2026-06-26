#include "editor/NanoEditor.hpp"
#include "config/ThemeManager.hpp"
#include <algorithm>
#include <sstream>

namespace FTB {
namespace Editor {

using namespace ftxui;

// ─── Theme color helper ──────────────────────────────────────────────

inline Color TC(const std::string& name) {
    return FTB::ThemeManager::GetInstance()->GetThemeColor(name);
}

// ═══════════════════════════════════════════════════════════════════════
// GapBuffer Implementation
// ═══════════════════════════════════════════════════════════════════════

GapBuffer::GapBuffer()
    : buffer_(256, '\0'), gap_start_(0), gap_end_(256) {}

void GapBuffer::Insert(char ch) {
    if (GapSize() < 2) ExpandGap(128);
    buffer_[gap_start_++] = ch;
}

void GapBuffer::Insert(const std::string& str) {
    for (char ch : str) Insert(ch);
}

void GapBuffer::DeleteBack() {
    if (gap_start_ > 0) gap_start_--;
}

void GapBuffer::DeleteForward() {
    if (gap_end_ < static_cast<int>(buffer_.size())) gap_end_++;
}

void GapBuffer::MoveLeft() {
    if (gap_start_ > 0) {
        buffer_[--gap_end_] = buffer_[--gap_start_];
    }
}

void GapBuffer::MoveRight() {
    if (gap_end_ < static_cast<int>(buffer_.size())) {
        buffer_[gap_start_++] = buffer_[gap_end_++];
    }
}

void GapBuffer::MoveToLineStart() {
    while (gap_start_ > 0) {
        MoveLeft();
    }
}

void GapBuffer::MoveToLineEnd() {
    while (gap_end_ < static_cast<int>(buffer_.size())) {
        MoveRight();
    }
}

std::string GapBuffer::GetText() const {
    std::string result;
    result.reserve(Length());
    for (int i = 0; i < gap_start_; ++i) result += buffer_[i];
    for (int i = gap_end_; i < static_cast<int>(buffer_.size()); ++i) result += buffer_[i];
    return result;
}

void GapBuffer::SetText(const std::string& text) {
    buffer_.resize(text.size() + 128, '\0');
    for (size_t i = 0; i < text.size(); ++i) buffer_[i] = text[i];
    gap_start_ = static_cast<int>(text.size());
    gap_end_ = static_cast<int>(buffer_.size());
}

char GapBuffer::CharAt(int pos) const {
    if (pos < gap_start_) return buffer_[pos];
    if (pos + GapSize() < static_cast<int>(buffer_.size()))
        return buffer_[pos + GapSize()];
    return '\0';
}

void GapBuffer::MoveGapTo(int pos) {
    if (pos < 0) pos = 0;
    int len = Length();
    if (pos > len) pos = len;

    if (pos < gap_start_) {
        // Move gap left
        int count = gap_start_ - pos;
        for (int i = 0; i < count; ++i) {
            buffer_[--gap_end_] = buffer_[--gap_start_];
        }
    } else if (pos > gap_start_) {
        // Move gap right
        int count = pos - gap_start_;
        for (int i = 0; i < count; ++i) {
            buffer_[gap_start_++] = buffer_[gap_end_++];
        }
    }
}

void GapBuffer::ExpandGap(int min_size) {
    int old_size = static_cast<int>(buffer_.size());
    int new_size = old_size + std::max(min_size, old_size);
    std::vector<char> new_buffer(new_size, '\0');

    // Copy before gap
    for (int i = 0; i < gap_start_; ++i) new_buffer[i] = buffer_[i];
    // Copy after gap
    int new_gap_end = new_size - (old_size - gap_end_);
    for (int i = gap_end_; i < old_size; ++i) new_buffer[new_gap_end + (i - gap_end_)] = buffer_[i];

    buffer_ = std::move(new_buffer);
    gap_end_ = new_gap_end;
}

// ═══════════════════════════════════════════════════════════════════════
// NanoEditor Implementation
// ═══════════════════════════════════════════════════════════════════════

NanoEditor::NanoEditor() {
    lines_.emplace_back();
    original_lines_.emplace_back();
    SaveState();
}

NanoEditor::~NanoEditor() = default;

// ─── Content Management ──────────────────────────────────────────────

void NanoEditor::SetContent(const std::vector<std::string>& new_lines) {
    lines_.clear();
    for (const auto& line : new_lines) {
        GapBuffer gb;
        gb.SetText(line);
        lines_.push_back(std::move(gb));
    }
    if (lines_.empty()) lines_.emplace_back();

    original_lines_ = lines_;
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_offset_ = 0;
    desired_col_ = 0;

    // Reset undo
    undo_history_.clear();
    undo_index_ = -1;
    SaveState();
}

std::vector<std::string> NanoEditor::GetContent() const {
    std::vector<std::string> result;
    result.reserve(lines_.size());
    for (const auto& gb : lines_) result.push_back(gb.GetText());
    return result;
}

void NanoEditor::SetOnExit(std::function<void(const std::vector<std::string>&)> on_exit) {
    on_exit_ = std::move(on_exit);
}

void NanoEditor::Enter() {
    if (lines_.empty()) lines_.emplace_back();
    original_lines_ = lines_;
    cursor_line_ = 0;
    cursor_col_ = 0;
    scroll_offset_ = 0;
    desired_col_ = 0;
    ShowStatus("^O=Save ^X=Exit ^K=Cut ^U=Paste ^W=Search ^V=PageDn ^Y=PageUp ^_=GoLn ^C=Pos Alt+M=MD");
}

// ─── Rendering ───────────────────────────────────────────────────────

Element NanoEditor::Render() {
    if (markdown_preview_mode_) {
        return RenderMarkdownPreview();
    }

    // Update horizontal scroll before rendering
    UpdateHScroll();

    int total_lines = static_cast<int>(lines_.size());
    int end_line = std::min(scroll_offset_ + MAX_VISIBLE_LINES, total_lines);
    int content_width = EDITOR_WIDTH - LINE_NUMBER_WIDTH;

    Elements rendered_lines;
    rendered_lines.reserve(end_line - scroll_offset_);

    for (int i = scroll_offset_; i < end_line; ++i) {
        // Line number
        std::string line_number = std::to_string(i + 1);
        int max_line_width = std::to_string(end_line).length();
        line_number.resize(max_line_width, ' ');
        line_number += " │";

        // Get line text and apply horizontal scroll
        std::string line_text = lines_[i].GetText();

        // Render with syntax highlighting (only on cursor line)
        bool is_cursor_line = (i == cursor_line_);
        Element line_content;

        if (is_cursor_line) {
            // Render the visible portion of the line
            int start = h_scroll_offset_;
            int end = std::min(start + content_width, static_cast<int>(line_text.size()));
            std::string visible_text = (start < static_cast<int>(line_text.size()))
                ? line_text.substr(start, end - start) : "";
            int adjusted_cursor = cursor_col_ - h_scroll_offset_;

            line_content = syntax_highlighter_.RenderLine(visible_text, true, adjusted_cursor);
        } else {
            // Non-cursor line: just show visible portion
            int start = h_scroll_offset_;
            int end = std::min(start + content_width, static_cast<int>(line_text.size()));
            std::string visible_text = (start < static_cast<int>(line_text.size()))
                ? line_text.substr(start, end - start) : "";
            line_content = syntax_highlighter_.RenderLine(visible_text, false, -1);
        }

        // Current line highlight
        if (is_cursor_line) {
            rendered_lines.push_back(
                hbox({
                    text(line_number) | color(Color::SkyBlue2) | bold,
                    text(" "),
                    line_content | size(WIDTH, LESS_THAN, content_width),
                }) | bgcolor(TC("selection_bg"))
            );
        } else {
            rendered_lines.push_back(
                hbox({
                    text(line_number) | color(Color::SkyBlue2),
                    text(" "),
                    line_content | size(WIDTH, LESS_THAN, content_width),
                })
            );
        }
    }

    // Fill remaining lines with empty space
    for (int i = end_line; i < scroll_offset_ + MAX_VISIBLE_LINES; ++i) {
        std::string line_number = std::to_string(i + 1);
        int max_line_width = std::to_string(scroll_offset_ + MAX_VISIBLE_LINES).length();
        line_number.resize(max_line_width, ' ');
        line_number += " │";
        rendered_lines.push_back(
            hbox({text(line_number) | color(Color::Grey37), text(" ")})
        );
    }

    // Build the editor layout with fixed width
    return vbox({
        // Title bar
        hbox({
            text(" FTB Editor") | color(TC("main_bg")) | bold | bgcolor(TC("syn_keyword")),
            text(" " + current_filename_ + " ") | color(TC("status_fg")) | bgcolor(TC("status_bg")),
            filler() | bgcolor(TC("status_bg")),
            text(" " + std::to_string(cursor_line_ + 1) + "," + std::to_string(cursor_col_ + 1) + " ") | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        }) | size(WIDTH, EQUAL, EDITOR_WIDTH),
        // Content area
        vbox(rendered_lines) | flex | size(WIDTH, EQUAL, EDITOR_WIDTH) | bgcolor(TC("dialog_bg")),
        // Status message
        RenderStatusBar(),
        // Shortcut bar
        RenderShortcutBar(),
    }) | borderRounded | size(WIDTH, EQUAL, EDITOR_WIDTH + 2);
}

Element NanoEditor::RenderStatusBar() const {
    if (status_message_.empty() && !search_mode_) {
        return text("") | bgcolor(TC("status_bg"));
    }

    std::string msg = search_mode_ ? ("Search: " + search_query_) : status_message_;
    return text(" " + msg) | color(TC("status_fg")) | bgcolor(TC("status_bg"));
}

Element NanoEditor::RenderShortcutBar() const {
    // Nano-style shortcut bar at the bottom
    auto shortcut = [](const std::string& key, const std::string& desc) {
        return hbox({
            text("^" + key) | color(TC("syn_keyword")) | bold,
            text(desc) | color(TC("dim")),
        });
    };

    return hbox({
        shortcut("G", "Help  "),
        shortcut("O", "Save  "),
        shortcut("X", "Exit  "),
        shortcut("K", "Cut  "),
        shortcut("U", "Paste  "),
        shortcut("W", "Search  "),
        shortcut("V", "PageDn  "),
        shortcut("Y", "PageUp  "),
        shortcut("_", "GoLn  "),
        shortcut("C", "Pos  "),
    }) | bgcolor(TC("main_bg"));
}

Element NanoEditor::RenderMarkdownPreview() {
    std::vector<std::string> content = GetContent();
    auto preview = md_transformer_.TransformToElement(content);

    return vbox({
        hbox({
            text(" FTB Editor") | color(TC("main_bg")) | bold | bgcolor(TC("syn_keyword")),
            text(" Markdown Preview ") | color(TC("status_fg")) | bgcolor(TC("status_bg")),
            filler() | bgcolor(TC("status_bg")),
            text(" Alt+M=Toggle ") | color(TC("dim")) | bgcolor(TC("status_bg")),
        }),
        preview | flex | border,
        RenderShortcutBar(),
    });
}

// ─── Event Handling ──────────────────────────────────────────────────

bool NanoEditor::OnEvent(Event event) {
    // Markdown preview mode
    if (markdown_preview_mode_) {
        if (event == Event::AltM || event == Event::Escape) {
            markdown_preview_mode_ = false;
            return true;
        }
        if (event.is_mouse()) {
            if (event.mouse().button == Mouse::WheelUp) {
                HandlePreviewScroll(-3);
                return true;
            }
            if (event.mouse().button == Mouse::WheelDown) {
                HandlePreviewScroll(3);
                return true;
            }
        }
        return false;
    }

    // Go to line mode (Ctrl+_)
    if (goto_line_mode_) {
        if (event == Event::Escape) {
            goto_line_mode_ = false;
            goto_line_input_.clear();
            ShowStatus("");
            return true;
        }
        if (event == Event::Return) {
            goto_line_mode_ = false;
            int target = 1;
            try {
                target = std::stoi(goto_line_input_);
            } catch (...) {
                target = 1;
            }
            target = std::max(1, std::min(target, static_cast<int>(lines_.size())));
            cursor_line_ = target - 1;
            cursor_col_ = 0;
            desired_col_ = 0;
            UpdateScrollOffset();
            ShowStatus("Moved to line " + std::to_string(target));
            goto_line_input_.clear();
            return true;
        }
        if (event == Event::Backspace) {
            if (!goto_line_input_.empty()) goto_line_input_.pop_back();
            return true;
        }
        if (event.is_character()) {
            char c = event.character()[0];
            if (c >= '0' && c <= '9') {
                goto_line_input_ += c;
            }
            return true;
        }
        return true;
    }

    // Search mode (Ctrl+W)
    if (search_mode_) {
        if (event == Event::Escape) {
            search_mode_ = false;
            search_query_.clear();
            ShowStatus("");
            return true;
        }
        if (event == Event::Return) {
            search_mode_ = false;
            Search(search_query_);
            return true;
        }
        if (event == Event::Backspace) {
            if (!search_query_.empty()) search_query_.pop_back();
            return true;
        }
        if (event.is_character()) {
            search_query_ += event.character();
            return true;
        }
        return true;
    }

    // Mouse scroll
    if (event.is_mouse()) {
        if (event.mouse().button == Mouse::WheelUp) {
            if (scroll_offset_ > 0)
                scroll_offset_ = std::max(0, scroll_offset_ - 3);
            return true;
        }
        if (event.mouse().button == Mouse::WheelDown) {
            int max_scroll = std::max(0, static_cast<int>(lines_.size()) - MAX_VISIBLE_LINES);
            if (scroll_offset_ < max_scroll)
                scroll_offset_ = std::min(max_scroll, scroll_offset_ + 3);
            return true;
        }
    }

    // ── Nano-style Ctrl shortcuts ──

    // Ctrl+A: Move to beginning of line
    if (event == Event::CtrlA) {
        cursor_col_ = 0;
        desired_col_ = 0;
        return true;
    }

    // Ctrl+E: Move to end of line
    if (event == Event::CtrlE) {
        cursor_col_ = static_cast<int>(lines_[cursor_line_].GetText().size());
        desired_col_ = cursor_col_;
        return true;
    }

    // Ctrl+B: Move back one character
    if (event == Event::CtrlB) {
        return OnEvent(Event::ArrowLeft);
    }

    // Ctrl+F: Move forward one character
    if (event == Event::CtrlF) {
        return OnEvent(Event::ArrowRight);
    }

    // Ctrl+P: Move up one line
    if (event == Event::CtrlP) {
        return OnEvent(Event::ArrowUp);
    }

    // Ctrl+N: Move down one line
    if (event == Event::CtrlN) {
        return OnEvent(Event::ArrowDown);
    }

    // Ctrl+D: Delete character at cursor (like Delete key)
    if (event == Event::CtrlD) {
        return OnEvent(Event::Delete);
    }

    // Ctrl+H: Backspace (some terminals send Ctrl+H for Backspace)
    if (event == Event::CtrlH) {
        return OnEvent(Event::Backspace);
    }

    // Ctrl+I: Tab
    if (event == Event::CtrlI) {
        return OnEvent(Event::Tab);
    }

    // Ctrl+M: Enter/Return
    if (event == Event::CtrlM) {
        return OnEvent(Event::Return);
    }

    // Ctrl+G: Help
    if (event == Event::CtrlG) {
        ShowStatus("^O=Save ^X=Exit ^K=Cut ^U=Paste ^W=Search ^V=PageDn ^Y=PageUp ^_=Goto"
                   " ^A/E=Home/End ^B/F=Left/Right ^P/N=Up/Down ^C=Pos Alt+M=MD Alt+U=Undo Alt+E=Redo");
        return true;
    }

    // Ctrl+O: Save (write out)
    if (event == Event::CtrlO) {
        if (on_exit_) on_exit_(GetContent());
        ShowStatus("File saved");
        return true;
    }

    // Ctrl+X: Exit editor
    if (event == Event::CtrlX) {
        if (on_exit_) on_exit_(GetContent());
        return true;
    }

    // Ctrl+K: Cut current line (like nano)
    if (event == Event::CtrlK) {
        SaveState();
        clipboard_ = lines_[cursor_line_].GetText();
        if (lines_.size() > 1) {
            lines_.erase(lines_.begin() + cursor_line_);
            if (cursor_line_ >= static_cast<int>(lines_.size()))
                cursor_line_ = static_cast<int>(lines_.size()) - 1;
        } else {
            lines_[0].SetText("");
        }
        cursor_col_ = 0;
        desired_col_ = 0;
        ShowStatus("Line cut");
        return true;
    }

    // Ctrl+U: Paste at cursor position (nano-style, inline)
    if (event == Event::CtrlU) {
        if (!clipboard_.empty()) {
            SaveState();
            lines_[cursor_line_].MoveGapTo(cursor_col_);
            lines_[cursor_line_].Insert(clipboard_);
            cursor_col_ += static_cast<int>(clipboard_.size());
            desired_col_ = cursor_col_;
            ShowStatus("Pasted");
        }
        return true;
    }

    // Ctrl+W: Search (wraps around)
    if (event == Event::CtrlW) {
        search_mode_ = true;
        search_query_.clear();
        ShowStatus("Search:");
        return true;
    }

    // Ctrl+_: Go to line (nano-style, Ctrl+_ sends byte 0x1F)
    if (event == Event::Special("\x1F")) {
        goto_line_mode_ = true;
        goto_line_input_.clear();
        ShowStatus("Go to line:");
        return true;
    }

    // Ctrl+V: PageDown
    if (event == Event::CtrlV) {
        PageDown();
        return true;
    }

    // Ctrl+Y: PageUp
    if (event == Event::CtrlY) {
        PageUp();
        return true;
    }

    // Ctrl+Z: Undo
    if (event == Event::CtrlZ) {
        Undo();
        ShowStatus("Undo");
        return true;
    }

    // Alt+U: Undo (nano modern binding)
    if (event == Event::AltU) {
        Undo();
        ShowStatus("Undo");
        return true;
    }

    // Alt+E: Redo (nano modern binding)
    if (event == Event::AltE) {
        Redo();
        ShowStatus("Redo");
        return true;
    }

    // Alt+M: Toggle markdown preview
    if (event == Event::AltM) {
        ToggleMarkdownPreview();
        return true;
    }

    // Ctrl+C: Show cursor position (nano-style)
    if (event == Event::CtrlC) {
        int line_num = cursor_line_ + 1;
        int col_num = cursor_col_ + 1;
        int total_lines = static_cast<int>(lines_.size());
        int total_chars = 0;
        for (const auto& l : lines_)
            total_chars += static_cast<int>(l.GetText().size());
        ShowStatus("line " + std::to_string(line_num) + "/" + std::to_string(total_lines) +
                   ", col " + std::to_string(col_num) + ", chars " + std::to_string(total_chars));
        return true;
    }

    // Ctrl+L: Refresh screen
    if (event == Event::CtrlL) {
        ShowStatus("Screen refreshed");
        return true;
    }

    // ── Navigation ──

    // Arrow keys
    if (event == Event::ArrowLeft) {
        if (cursor_col_ > 0) {
            cursor_col_--;
            desired_col_ = cursor_col_;
            lines_[cursor_line_].MoveGapTo(cursor_col_);
        } else if (cursor_line_ > 0) {
            cursor_line_--;
            cursor_col_ = static_cast<int>(lines_[cursor_line_].GetText().size());
            desired_col_ = cursor_col_;
            UpdateScrollOffset();
        }
        return true;
    }

    if (event == Event::ArrowRight) {
        int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
        if (cursor_col_ < line_len) {
            cursor_col_++;
            desired_col_ = cursor_col_;
        } else if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            cursor_line_++;
            cursor_col_ = 0;
            desired_col_ = 0;
            UpdateScrollOffset();
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        if (cursor_line_ > 0) {
            cursor_line_--;
            int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
            cursor_col_ = std::min(desired_col_, line_len);
            UpdateScrollOffset();
        }
        return true;
    }

    if (event == Event::ArrowDown) {
        if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            cursor_line_++;
            int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
            cursor_col_ = std::min(desired_col_, line_len);
            UpdateScrollOffset();
        }
        return true;
    }

    // Home: move to line start
    if (event == Event::Home) {
        cursor_col_ = 0;
        desired_col_ = 0;
        return true;
    }

    // End: move to line end
    if (event == Event::End) {
        cursor_col_ = static_cast<int>(lines_[cursor_line_].GetText().size());
        desired_col_ = cursor_col_;
        return true;
    }

    // PageUp
    if (event == Event::PageUp) {
        PageUp();
        return true;
    }

    // PageDown
    if (event == Event::PageDown) {
        PageDown();
        return true;
    }

    // ── Text editing ──

    // Backspace
    if (event == Event::Backspace) {
        SaveState();
        if (cursor_col_ > 0) {
            lines_[cursor_line_].MoveGapTo(cursor_col_);
            lines_[cursor_line_].DeleteBack();
            cursor_col_--;
            desired_col_ = cursor_col_;
        } else if (cursor_line_ > 0) {
            std::string current_text = lines_[cursor_line_].GetText();
            int prev_len = static_cast<int>(lines_[cursor_line_ - 1].GetText().size());
            lines_[cursor_line_ - 1].MoveGapTo(prev_len);
            lines_[cursor_line_ - 1].Insert(current_text);
            lines_.erase(lines_.begin() + cursor_line_);
            cursor_line_--;
            cursor_col_ = prev_len;
            desired_col_ = cursor_col_;
            UpdateScrollOffset();
        }
        return true;
    }

    // Delete
    if (event == Event::Delete) {
        SaveState();
        int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
        if (cursor_col_ < line_len) {
            lines_[cursor_line_].MoveGapTo(cursor_col_);
            lines_[cursor_line_].DeleteForward();
        } else if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            std::string next_text = lines_[cursor_line_ + 1].GetText();
            lines_[cursor_line_].MoveGapTo(line_len);
            lines_[cursor_line_].Insert(next_text);
            lines_.erase(lines_.begin() + cursor_line_ + 1);
        }
        return true;
    }

    // Enter: split line with auto-indent
    if (event == Event::Return) {
        SaveState();
        lines_[cursor_line_].MoveGapTo(cursor_col_);
        std::string before_cursor = lines_[cursor_line_].GetText().substr(0, cursor_col_);
        std::string after_cursor = lines_[cursor_line_].GetText().substr(cursor_col_);

        // Preserve leading whitespace (auto-indent)
        std::string indent;
        for (char ch : before_cursor) {
            if (ch == ' ' || ch == '\t') indent += ch;
            else break;
        }

        lines_[cursor_line_].SetText(before_cursor);

        GapBuffer new_line;
        new_line.SetText(indent + after_cursor);
        lines_.insert(lines_.begin() + cursor_line_ + 1, std::move(new_line));

        cursor_line_++;
        cursor_col_ = static_cast<int>(indent.size());
        desired_col_ = cursor_col_;
        UpdateScrollOffset();
        return true;
    }

    // Tab
    if (event == Event::Tab) {
        SaveState();
        lines_[cursor_line_].MoveGapTo(cursor_col_);
        lines_[cursor_line_].Insert("    ");
        cursor_col_ += 4;
        desired_col_ = cursor_col_;
        return true;
    }

    // Regular character input
    if (event.is_character()) {
        const std::string& ch = event.character();
        if (!ch.empty()) {
            SaveState();
            lines_[cursor_line_].MoveGapTo(cursor_col_);
            lines_[cursor_line_].Insert(ch);
            cursor_col_ += static_cast<int>(ch.size());
            desired_col_ = cursor_col_;
        }
        return true;
    }

    return false;
}

// ─── Cursor Positioning ──────────────────────────────────────────────

void NanoEditor::MoveCursorTo(int line, int col) {
    cursor_line_ = std::max(0, std::min(line, static_cast<int>(lines_.size()) - 1));
    cursor_col_ = std::max(0, std::min(col, static_cast<int>(lines_[cursor_line_].GetText().size())));
    desired_col_ = cursor_col_;
    UpdateScrollOffset();
}

// ─── Syntax Highlighting ─────────────────────────────────────────────

void NanoEditor::SetLanguage(Language lang) {
    syntax_highlighter_.SetLanguage(lang);
}

Language NanoEditor::GetLanguage() const {
    return syntax_highlighter_.GetLanguage();
}

void NanoEditor::SetFilename(const std::string& filename) {
    current_filename_ = filename;
    Language detected = SyntaxHighlighter::DetectLanguage(filename);
    syntax_highlighter_.SetLanguage(detected);
}

// ─── Undo/Redo ───────────────────────────────────────────────────────

void NanoEditor::SaveState() {
    // Remove future states
    if (undo_index_ >= 0 && undo_index_ < static_cast<int>(undo_history_.size()) - 1) {
        undo_history_.erase(undo_history_.begin() + undo_index_ + 1, undo_history_.end());
    }

    UndoState state;
    state.lines = GetContent();
    state.cursor_line = cursor_line_;
    state.cursor_col = cursor_col_;
    undo_history_.push_back(std::move(state));
    undo_index_ = static_cast<int>(undo_history_.size()) - 1;

    // Limit history size
    if (undo_history_.size() > 1000) {
        undo_history_.erase(undo_history_.begin(), undo_history_.begin() + 100);
        undo_index_ -= 100;
    }
}

void NanoEditor::Undo() {
    if (!CanUndo()) return;
    undo_index_--;
    const auto& state = undo_history_[undo_index_];
    SetContent(state.lines);
    cursor_line_ = state.cursor_line;
    cursor_col_ = state.cursor_col;
    desired_col_ = cursor_col_;
}

void NanoEditor::Redo() {
    if (!CanRedo()) return;
    undo_index_++;
    const auto& state = undo_history_[undo_index_];
    SetContent(state.lines);
    cursor_line_ = state.cursor_line;
    cursor_col_ = state.cursor_col;
    desired_col_ = cursor_col_;
}

bool NanoEditor::CanUndo() const {
    return undo_index_ > 0;
}

bool NanoEditor::CanRedo() const {
    return undo_index_ < static_cast<int>(undo_history_.size()) - 1;
}

// ─── Search ──────────────────────────────────────────────────────────

void NanoEditor::Search(const std::string& query) {
    if (query.empty()) return;

    // Search forward from current position
    for (int i = cursor_line_; i < static_cast<int>(lines_.size()); ++i) {
        std::string line = lines_[i].GetText();
        size_t pos = line.find(query, (i == cursor_line_) ? cursor_col_ : 0);
        if (pos != std::string::npos) {
            cursor_line_ = i;
            cursor_col_ = static_cast<int>(pos);
            desired_col_ = cursor_col_;
            UpdateScrollOffset();
            ShowStatus("Found at line " + std::to_string(i + 1));
            return;
        }
    }
    ShowStatus("Not found: " + query);
}

void NanoEditor::Replace(const std::string& old_text, const std::string& new_text) {
    SaveState();
    std::string line = lines_[cursor_line_].GetText();
    size_t pos = line.find(old_text);
    if (pos != std::string::npos) {
        line.replace(pos, old_text.size(), new_text);
        lines_[cursor_line_].SetText(line);
        cursor_col_ = static_cast<int>(pos + new_text.size());
        desired_col_ = cursor_col_;
        ShowStatus("Replaced");
    } else {
        ShowStatus("Not found");
    }
}

void NanoEditor::ReplaceAll(const std::string& old_text, const std::string& new_text) {
    SaveState();
    int count = 0;
    for (auto& gb : lines_) {
        std::string line = gb.GetText();
        size_t pos = 0;
        while ((pos = line.find(old_text, pos)) != std::string::npos) {
            line.replace(pos, old_text.size(), new_text);
            pos += new_text.size();
            count++;
        }
        gb.SetText(line);
    }
    ShowStatus("Replaced " + std::to_string(count) + " occurrences");
}

// ─── Word Movement ───────────────────────────────────────────────────

void NanoEditor::MoveToPreviousWord() {
    std::string line = lines_[cursor_line_].GetText();
    int pos = cursor_col_ - 1;

    // Skip spaces
    while (pos > 0 && std::isspace(line[pos])) pos--;
    // Skip word characters
    while (pos > 0 && !std::isspace(line[pos - 1])) pos--;

    cursor_col_ = std::max(0, pos);
    desired_col_ = cursor_col_;
}

void NanoEditor::MoveToNextWord() {
    std::string line = lines_[cursor_line_].GetText();
    int pos = cursor_col_;

    // Skip current word
    while (pos < static_cast<int>(line.size()) && !std::isspace(line[pos])) pos++;
    // Skip spaces
    while (pos < static_cast<int>(line.size()) && std::isspace(line[pos])) pos++;

    if (pos >= static_cast<int>(line.size()) && cursor_line_ < static_cast<int>(lines_.size()) - 1) {
        cursor_line_++;
        cursor_col_ = 0;
        UpdateScrollOffset();
    } else {
        cursor_col_ = pos;
    }
    desired_col_ = cursor_col_;
}

// ─── Page Movement ───────────────────────────────────────────────────

void NanoEditor::PageUp() {
    cursor_line_ = std::max(0, cursor_line_ - MAX_VISIBLE_LINES);
    int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
    cursor_col_ = std::min(desired_col_, line_len);
    UpdateScrollOffset();
}

void NanoEditor::PageDown() {
    cursor_line_ = std::min(static_cast<int>(lines_.size()) - 1,
                            cursor_line_ + MAX_VISIBLE_LINES);
    int line_len = static_cast<int>(lines_[cursor_line_].GetText().size());
    cursor_col_ = std::min(desired_col_, line_len);
    UpdateScrollOffset();
}

// ─── Markdown Preview ────────────────────────────────────────────────

void NanoEditor::ToggleMarkdownPreview() {
    markdown_preview_mode_ = !markdown_preview_mode_;
    if (markdown_preview_mode_) {
        ShowStatus("Markdown preview mode");
    } else {
        ShowStatus("Editor mode");
    }
}

bool NanoEditor::IsMarkdownPreviewMode() const {
    return markdown_preview_mode_;
}

void NanoEditor::HandlePreviewScroll(int delta) {
    md_transformer_.ScrollBy(delta);
}

// ─── Internal ────────────────────────────────────────────────────────

void NanoEditor::UpdateScrollOffset() {
    if (cursor_line_ < scroll_offset_) {
        scroll_offset_ = cursor_line_;
    } else if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
        scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
    }
}

void NanoEditor::UpdateHScroll() {
    int content_width = EDITOR_WIDTH - LINE_NUMBER_WIDTH - 2;  // -2 for padding
    int margin = 5;  // Keep 5 chars visible margin

    // Scroll right if cursor is past visible area
    if (cursor_col_ >= h_scroll_offset_ + content_width - margin) {
        h_scroll_offset_ = cursor_col_ - content_width + margin + 1;
    }
    // Scroll left if cursor is before visible area
    if (cursor_col_ < h_scroll_offset_ + margin) {
        h_scroll_offset_ = std::max(0, cursor_col_ - margin);
    }
}

void NanoEditor::ShowStatus(const std::string& msg) {
    status_message_ = msg;
    status_timeout_ = 300;  // Show for ~300 frames (~5 seconds at 60fps)
}

void NanoEditor::EnsureLineExists() {
    if (lines_.empty()) lines_.emplace_back();
}

} // namespace Editor
} // namespace FTB
