#ifndef VIM_LIKE_HPP
#define VIM_LIKE_HPP

#include <string>
#include <vector>
#include <functional>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

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

private:
    bool edit_mode_;
    int cursor_line_;
    int cursor_col_;
    int scroll_offset_;  // ***新增：滚动偏移量***
    std::vector<std::string> lines_;
    std::vector<std::string> original_lines_;  // 新增：保存原始内容
    std::function<void(const std::vector<std::string>&)> on_exit_;
};

#endif // VIM_LIKE_HPP
