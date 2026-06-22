#pragma once

// ─── FTB Nano-style Editor ───────────────────────────────────────────
//
// A nano-style text editor with:
//   - Block cursor (like nvim)
//   - Gap buffer for efficient editing (like nvim's buffer)
//   - Syntax highlighting via SyntaxHighlighter
//   - Nano-style keybindings
//   - Markdown preview (Alt+M)
// ─────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "FTB/Editor/SyntaxHighlighter.hpp"
#include "FTB/Editor/MD_transformer.hpp"

namespace FTB {
namespace Editor {

// ─── Gap Buffer ──────────────────────────────────────────────────────
// High-performance text buffer inspired by nvim's buffer design.
// The gap buffer keeps a "gap" at the cursor position, making
// insertions and deletions at the cursor O(1) amortized.

class GapBuffer {
public:
    GapBuffer();

    // Insert character at cursor (gap) position
    void Insert(char ch);
    void Insert(const std::string& str);

    // Delete character before/after cursor
    void DeleteBack();
    void DeleteForward();

    // Cursor movement
    void MoveLeft();
    void MoveRight();
    void MoveToLineStart();
    void MoveToLineEnd();

    // Get current cursor position within the buffer
    int GetCursorPos() const { return gap_start_; }

    // Get the full text content
    std::string GetText() const;

    // Set text content (replaces all)
    void SetText(const std::string& text);

    // Get character at position
    char CharAt(int pos) const;

    // Move gap to specified position (needed by NanoEditor)
    void MoveGapTo(int pos);

    // Total content length (excluding gap)
    int Length() const { return static_cast<int>(buffer_.size()) - GapSize(); }

private:
    std::vector<char> buffer_;
    int gap_start_;     // Start of gap
    int gap_end_;       // End of gap (one past last gap char)

    int GapSize() const { return gap_end_ - gap_start_; }
    void ExpandGap(int min_size);
};

// ─── Nano Editor ─────────────────────────────────────────────────────

class NanoEditor {
public:
    NanoEditor();
    ~NanoEditor();

    // Content management
    void SetContent(const std::vector<std::string>& new_lines);
    std::vector<std::string> GetContent() const;

    // Callbacks
    void SetOnExit(std::function<void(const std::vector<std::string>&)> on_exit);

    // Enter editor (starts in edit mode, like nano)
    void Enter();

    // Rendering
    ftxui::Element Render();

    // Event handling
    bool OnEvent(ftxui::Event event);

    // Cursor positioning
    void MoveCursorTo(int line, int col);

    // Syntax highlighting
    void SetLanguage(Language lang);
    Language GetLanguage() const;
    void SetFilename(const std::string& filename);

    // Undo/Redo
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;

    // Search
    void Search(const std::string& query);
    void Replace(const std::string& old_text, const std::string& new_text);
    void ReplaceAll(const std::string& old_text, const std::string& new_text);

    // Word movement
    void MoveToPreviousWord();
    void MoveToNextWord();

    // Page movement
    void PageUp();
    void PageDown();

    // Markdown preview
    void ToggleMarkdownPreview();
    bool IsMarkdownPreviewMode() const;

    // Scroll
    void HandlePreviewScroll(int delta);

private:
    // ── Buffer ──
    // Line-based storage: each line is a GapBuffer for O(1) editing
    std::vector<GapBuffer> lines_;
    std::vector<GapBuffer> original_lines_;  // For discard

    // ── Cursor ──
    int cursor_line_ = 0;
    int cursor_col_ = 0;
    int scroll_offset_ = 0;       // Vertical scroll offset
    int h_scroll_offset_ = 0;     // Horizontal scroll offset
    int desired_col_ = 0;  // Remember column for vertical movement

    // ── State ──
    bool search_mode_ = false;
    std::string search_query_;
    bool goto_line_mode_ = false;
    std::string goto_line_input_;
    std::string status_message_;
    int status_timeout_ = 0;  // Frames remaining for status message

    // ── Callbacks ──
    std::function<void(const std::vector<std::string>&)> on_exit_;

    // ── Syntax highlighting ──
    SyntaxHighlighter syntax_highlighter_;
    std::string current_filename_;

    // ── Undo/Redo ──
    struct UndoState {
        std::vector<std::string> lines;
        int cursor_line;
        int cursor_col;
    };
    std::vector<UndoState> undo_history_;
    int undo_index_ = -1;

    // ── Clipboard ──
    std::string clipboard_;

    // ── Markdown preview ──
    bool markdown_preview_mode_ = false;
    MDTransformer md_transformer_;

    // ── Max visible lines ──
    static constexpr int MAX_VISIBLE_LINES = 40;
    static constexpr int EDITOR_WIDTH = 90;    // Fixed editor content width
    static constexpr int LINE_NUMBER_WIDTH = 6; // Width for line numbers

    // ── Internal methods ──
    void SaveState();
    void UpdateScrollOffset();
    void UpdateHScroll();
    ftxui::Element RenderStatusBar() const;
    ftxui::Element RenderShortcutBar() const;
    ftxui::Element RenderMarkdownPreview();
    void ShowStatus(const std::string& msg);
    void EnsureLineExists();
};

} // namespace Editor
} // namespace FTB
