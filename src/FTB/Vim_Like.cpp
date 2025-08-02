#include "../include/FTB/Vim_Like.hpp"
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>  // 用于实现光标闪烁效果

using namespace ftxui;

// 最大可见行数常量：决定编辑器窗口一次能显示多少行内容
const int MAX_VISIBLE_LINES = 35;

// ---------------------------- 构造与析构 ----------------------------

/**
 * @brief VimLikeEditor 类的构造函数
 * 初始化编辑器的成员变量：
 *   edit_mode_     - 是否处于编辑模式，初始为 false（查看模式）
 *   cursor_line_   - 当前光标行索引，初始为 0
 *   cursor_col_    - 当前光标列索引，初始为 0
 *   scroll_offset_ - 滚动偏移（顶部可见行索引），初始为 0
 *   lines_         - 存储当前编辑内容的行列表（至少保留一行空字符串）
 *   original_lines_- 保存初始状态，用于放弃修改时恢复
 */
VimLikeEditor::VimLikeEditor()
    : edit_mode_(false), cursor_line_(0), cursor_col_(0), scroll_offset_(0) {
    lines_.push_back("");           // 至少保留一行，避免空内容导致渲染问题
    original_lines_ = lines_;       // 初始状态（空内容）
}

/**
 * @brief VimLikeEditor 类的析构函数
 * 目前没有动态分配资源，无需特殊操作
 */
VimLikeEditor::~VimLikeEditor() {}

// ---------------------------- 回调与内容设置 ----------------------------

/**
 * @brief 设置编辑器退出时的回调函数
 * 当用户按下 Ctrl+D 或 Ctrl+F 退出编辑器时，会调用该回调并传出最终或原始内容
 * @param on_exit 接受一个 vector<string> 参数的函数，用于传出行内容
 */
void VimLikeEditor::SetOnExit(std::function<void(const std::vector<std::string>&)> on_exit) {
    on_exit_ = on_exit;
}

/**
 * @brief 设置编辑器的初始内容
 * 将传入的行列表赋值给 lines_，并将其保存在 original_lines_ 以便“放弃修改”时恢复
 * @param new_lines 编辑器需要展示的行内容列表
 */
void VimLikeEditor::SetContent(const std::vector<std::string>& new_lines) {
    lines_ = new_lines;
    original_lines_ = new_lines;  // 保留一份原始内容
    if (lines_.empty()) {
        // 如果传入的是空列表，则至少保留一行空字符串
        lines_.push_back("");
        original_lines_.push_back("");
    }
    cursor_line_ = 0;     // 光标重置到第一行
    cursor_col_ = 0;      // 光标重置到第一列
    scroll_offset_ = 0;   // 滚动偏移重置
}

/**
 * @brief 进入 Vim 编辑界面（但不自动进入编辑模式）
 * 保存当前内容状态，用于放弃修改时恢复
 */
void VimLikeEditor::EnterEditMode() {
    if (lines_.empty()) {
        lines_.push_back("");        // 确保至少有一行
    }
    original_lines_ = lines_;        // 保存当前行内容状态
    cursor_line_ = 0;                // 光标行重置为第 0 行
    cursor_col_ = 0;                 // 光标列重置为第 0 列
    scroll_offset_ = 0;              // 滚动偏移重置
}

// ---------------------------- 渲染函数 ----------------------------

/**
 * @brief 渲染编辑器的 UI 元素
 * 根据当前内容、光标位置、滚动偏移、编辑/查看模式等状态，构造 FTXUI 的 Element 树返回
 * - 上方为带模式指示的标题栏
 * - 中间是带边框的行内容显示区，每行显示行号 + 内容
 *   若当前为编辑模式且是光标所在行，会在光标位置插入闪烁竖线
 * - 下方为提示信息栏
 */
Element VimLikeEditor::Render() {
    Elements rendered_lines;                                // 存放渲染后的每一行 Element
    int total_lines = static_cast<int>(lines_.size());      // 当前行数
    // 计算本次需要渲染的末尾行索引
    int end_line = std::min(scroll_offset_ + MAX_VISIBLE_LINES, total_lines);

    // 计算行号宽度：使得所有行号对齐，例如 1 位、2 位、3 位...
    int max_line_number_width = std::to_string(end_line).length();

    // 实现闪烁光标：每 500 毫秒切换一次 blink_on 状态
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    bool blink_on = ((ms / 500) % 2 == 0);

    // 遍历 visible 范围内的每一行，生成对应的 UI
    for (int i = scroll_offset_; i < end_line; ++i) {
        // 先生成左侧行号区域
        std::ostringstream oss;
        // 左对齐输出行号，并留一个空格与后面内容分隔
        oss << std::setw(max_line_number_width) << std::left << (i + 1) << " ";
        std::string line_number = oss.str();

        // 如果当前处于编辑模式且 i 行是光标所在行
        if (edit_mode_ && i == cursor_line_) {
            // 将该行文本拆分为光标左侧和右侧
            std::string left = lines_[i].substr(0, cursor_col_);
            std::string right = lines_[i].substr(cursor_col_);
            rendered_lines.push_back(
                hbox({
                    // 行号文本，使用绿色
                    text(line_number) | color(Color::Green4),
                    // 光标左侧文本
                    text(left),
                    // 闪烁光标：如果 blink_on=true 则显示 "|" 否则显示空格；使用淡蓝色
                    (blink_on ? text("|") : text(" ")) | color(Color::BlueLight),
                    // 光标右侧文本
                    text(right)
                })
            );
        } else {
            // 普通行渲染：行号为蓝色，后面跟整行内容
            rendered_lines.push_back(
                hbox({
                    text(line_number) | color(Color::SkyBlue2),
                    text(lines_[i])
                })
            );
        }
    }

    // 构造状态栏文本，区分编辑模式与查看模式
    std::string mode_text = edit_mode_ ? "📝 编辑模式" : "👀 查看模式";

    // 最终返回一个垂直布局：
    //   1. 标题栏：显示编辑器名称与模式，文本加粗并居中，背景绿色
    //   2. 分隔符
    //   3. 主编辑区：渲染行内容并绘制边框
    //   4. 提示信息：按键说明，使用橙色粗边框并居中
    return vbox({
        text("Vim-Like Editor - " + mode_text) | bold | center | bgcolor(Color::Green),
        separator(),
        vbox(rendered_lines) | border,
        text("提示：按i开始编辑|ESC退出编辑模式|Ctrl+D保存并退出|Ctrl+F放弃并退出")
            | borderHeavy | color(Color::Orange3) | center
    }) | border;
}

// ---------------------------- 键盘事件处理 ----------------------------

/**
 * @brief 处理用户输入事件
 * 根据当前是否处于编辑模式，以及不同按键的含义，执行相应的逻辑：
 *   - Ctrl+E 进入编辑器（查看模式）
 *   - 查看模式下，按 'i' 进入编辑模式
 *   - 编辑模式下，ESC 退出编辑模式
 *   - Ctrl+D 保存并退出，调用 on_exit_ 回调，将最终内容传出
 *   - Ctrl+F 放弃修改并退出，调用 on_exit_ 回调，将 original_lines_ 传出
 *   - 编辑模式下，处理 Backspace、箭头键、回车、普通字符插入等操作
 * @param event FTXUI 传入的事件（键盘按键等）
 * @return 如果事件被处理，则返回 true；否则返回 false
 */
bool VimLikeEditor::OnEvent(Event event) {
    // ==================== 全局快捷键：Ctrl+E 进入编辑器（查看模式） ====================
    if (event == Event::CtrlE) {
        edit_mode_ = false;  // 进入后先处于查看模式
        return true;
    }

    // ==================== 如果当前处于查看模式 ====================
    if (!edit_mode_) {
        // 查看模式仅允许按 'i' 键进入编辑模式
        if (event.is_character() && event.character() == "i") {
            edit_mode_ = true;
            return true;
        }
        // 其他按键不处理，返回 false
        return false;
    }

    // ==================== 如果当前处于编辑模式 ====================

    // ESC：退出编辑模式，回到查看模式
    if (event == Event::Escape) {
        edit_mode_ = false;
        return true;
    }

    // Ctrl+D：保存修改并退出编辑器
    if (event == Event::CtrlD) {
        edit_mode_ = false;
        if (on_exit_) {
            on_exit_(lines_);  // 通过回调传出当前修改后的内容
        }
        return true;
    }

    // Ctrl+F：放弃修改并退出编辑器
    if (event == Event::CtrlF) {
        edit_mode_ = false;
        if (on_exit_) {
            on_exit_(original_lines_);  // 将原始内容传出
        }
        return true;
    }

    // ==================== 编辑模式下的文本编辑操作 ====================

    // Backspace：删除光标前的字符，若在行首则合并上一行
    if (event == Event::Backspace) {
        if (cursor_col_ > 0) {
            // 当前行非空，删除当前列前一个字符
            lines_[cursor_line_].erase(cursor_col_ - 1, 1);
            cursor_col_--;
        } else if (cursor_line_ > 0) {
            // 在行首且不是第一行，将当前行内容拼接到上一行末尾，并删除当前行
            cursor_col_ = lines_[cursor_line_ - 1].size();
            lines_[cursor_line_ - 1] += lines_[cursor_line_];
            lines_.erase(lines_.begin() + cursor_line_);
            cursor_line_--;
        }
        return true;
    }

    // 光标左移：列索引大于 0 时才移动
    if (event == Event::ArrowLeft) {
        if (cursor_col_ > 0)
            cursor_col_--;
        return true;
    }
    // 光标右移：列索引小于当前行长度时移动
    if (event == Event::ArrowRight) {
        if (cursor_col_ < static_cast<int>(lines_[cursor_line_].size()))
            cursor_col_++;
        return true;
    }

    // 光标上移：行索引大于 0 时才移动，并保持列索引不超出新行长度
    // 同时如果光标行在滚动偏移之上，需要向上滚动视图
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
    // 光标下移：行索引小于总行数减1 时移动，并保持列索引不超出新行长度
    // 如果光标行越过当前视图底部，需要向下滚动视图
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

    // 回车（Return）：在当前光标位置拆分行
    if (event == Event::Return) {
        std::string current_line = lines_[cursor_line_];
        // 新行内容为光标右侧剩余部分
        std::string new_line = current_line.substr(cursor_col_);
        // 当前行保留光标左侧部分
        lines_[cursor_line_].resize(cursor_col_);
        // 将新行插入到当前行之后
        lines_.insert(lines_.begin() + cursor_line_ + 1, new_line);
        // 光标移动到新行开头
        cursor_line_++;
        cursor_col_ = 0;
        // 如果超出可见区，要向下滚动
        if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
            scroll_offset_++;
        }
        return true;
    }

    // 普通字符插入：在当前光标位置插入单个字符
    if (event.is_character()) {
        char ch = event.character()[0];
        lines_[cursor_line_].insert(cursor_col_, 1, ch);
        cursor_col_++;
        return true;
    }

    // 其他按键不处理
    return false;
}

// ---------------------------- 光标定位 ----------------------------

/**
 * @brief 将光标移动到指定的行列位置
 * 同时自动调整滚动偏移，如果目标行不在可见区，则滚动视图以将光标行置于可见范围
 * @param line 目标行索引（从 0 开始）
 * @param col  目标列索引（从 0 开始）
 */
void VimLikeEditor::MoveCursorTo(int line, int col) {
    // 限制行索引在 [0, 总行数-1] 范围内
    cursor_line_ = std::max(0, std::min(line, static_cast<int>(lines_.size()) - 1));
    // 限制列索引在 [0, 当前行长度] 范围内
    cursor_col_ = std::max(0, std::min(col, static_cast<int>(lines_[cursor_line_].size())));
    // 如果光标行在可见区上方，需要向上滚动
    if (cursor_line_ < scroll_offset_) {
        scroll_offset_ = cursor_line_;
    }
    // 如果光标行超过可见区下方，需要向下滚动
    else if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
        scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
    }
}
