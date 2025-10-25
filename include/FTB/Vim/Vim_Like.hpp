#ifndef VIM_LIKE_HPP
#define VIM_LIKE_HPP

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "FTB/Vim/SyntaxHighlighter.hpp"
#include "FTB/Vim/MD_transformer.hpp"

class VimLikeEditor {
public:
    VimLikeEditor();
    ~VimLikeEditor();

    void SetOnExit(std::function<void(const std::vector<std::string>&)> on_exit);
    void SetContent(const std::vector<std::string>& new_lines);
    void EnterEditMode();
    ftxui::Element Render();
    bool OnEvent(ftxui::Event event);
    void MoveCursorTo(int line, int col);
    
    // 语法高亮相关方法
    void SetLanguage(FTB::Vim::Language lang);
    FTB::Vim::Language GetLanguage() const;
    void SetFilename(const std::string& filename);
    
    // 撤销/重做功能
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
    // 搜索和替换功能
    void Search(const std::string& query);
    void Replace(const std::string& old_text, const std::string& new_text);
    void ReplaceAll(const std::string& old_text, const std::string& new_text);
    
    // 高级光标移动功能
    void MoveToPreviousWord();
    void MoveToNextWord();
    void PageUp();
    void PageDown();
    
    // Markdown预览功能
    void ToggleMarkdownPreview();
    bool IsMarkdownPreviewMode() const;
    void SetMarkdownPreviewMode(bool enabled);
    void HandlePreviewScroll(int delta);

private:
    bool edit_mode_;
    int cursor_line_;
    int cursor_col_;
    int scroll_offset_;  // ***新增：滚动偏移量***
    std::vector<std::string> lines_;
    std::vector<std::string> original_lines_;  // 新增：保存原始内容
    std::function<void(const std::vector<std::string>&)> on_exit_;
    
    // 语法高亮相关
    FTB::Vim::SyntaxHighlighter syntax_highlighter_;
    std::string current_filename_;
    
    // 撤销/重做历史
    std::vector<std::vector<std::string>> undo_history_;
    int undo_index_;
    
    // 剪贴板
    std::string clipboard_;
    
    // 光标移动优化
    std::chrono::steady_clock::time_point last_move_time_;
    int move_repeat_count_;
    
    // Markdown预览相关
    bool markdown_preview_mode_;
    FTB::Vim::MDTransformer md_transformer_;
    
    // 辅助方法
    void SaveState();
    void UpdateMoveSpeed();
    ftxui::Element RenderMarkdownPreview();
};

#endif // VIM_LIKE_HPP
