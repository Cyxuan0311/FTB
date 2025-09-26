#include "FTB/Vim/Vim_Like.hpp"
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
    : edit_mode_(false), cursor_line_(0), cursor_col_(0), scroll_offset_(0), undo_index_(0), move_repeat_count_(0) {
    lines_.push_back("");           // 至少保留一行，避免空内容导致渲染问题
    original_lines_ = lines_;       // 初始状态（空内容）
    
    // 初始化撤销/重做历史
    undo_history_.push_back(lines_);
    
    // 初始化移动时间
    last_move_time_ = std::chrono::steady_clock::now();
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
    // bool blink_on = ((ms / 500) % 2 == 0); // 暂时注释掉，避免未使用变量警告

    // 预分配空间，避免动态扩容
    rendered_lines.reserve(end_line - scroll_offset_);

    // 缓存行号字符串，避免重复计算
    std::string line_number_prefix;
    line_number_prefix.reserve(max_line_number_width + 1);
    line_number_prefix.resize(max_line_number_width, ' ');
    line_number_prefix += " ";

    // 遍历 visible 范围内的每一行，生成对应的 UI
    for (int i = scroll_offset_; i < end_line; ++i) {
        // 先生成左侧行号区域 - 使用更高效的字符串构建
        std::string line_number = std::to_string(i + 1);
        line_number.resize(max_line_number_width, ' ');
        line_number += " ";

        // 使用语法高亮渲染行
        ftxui::Element line_content = syntax_highlighter_.RenderLine(
            lines_[i], edit_mode_ && i == cursor_line_, cursor_col_);
        
        rendered_lines.emplace_back(
            hbox({
                text(line_number) | color(Color::SkyBlue2),
                line_content
            })
        );
    }

    // 构造状态栏文本，区分编辑模式与查看模式
    std::string mode_text = edit_mode_ ? "📝 编辑模式" : "👀 查看模式";
    
    // 添加语言类型信息
    std::string language_text = "";
    switch (syntax_highlighter_.GetLanguage()) {
        case FTB::Vim::Language::C:
            language_text = " | C";
            break;
        case FTB::Vim::Language::CPP:
            language_text = " | C++";
            break;
        case FTB::Vim::Language::GO:
            language_text = " | Go";
            break;
        case FTB::Vim::Language::PYTHON:
            language_text = " | Python";
            break;
        default:
            language_text = " | 无语法高亮";
            break;
    }

    // 最终返回一个垂直布局：
    //   1. 标题栏：显示编辑器名称与模式，文本加粗并居中，背景绿色
    //   2. 分隔符
    //   3. 主编辑区：渲染行内容并绘制边框
    //   4. 提示信息：按键说明，使用橙色粗边框并居中
    return vbox({
        text("Vim-Like Editor - " + mode_text + language_text) | bold | center | bgcolor(Color::Green),
        separator(),
        vbox(rendered_lines) | border,
        vbox({
            text("📝 编辑操作：按i开始编辑|ESC退出编辑模式|Ctrl+D保存并退出|Ctrl+F放弃并退出|Ctrl+Z撤销|Ctrl+Y重做") | center,
            text("光标移动：箭头键移动|Home/End行首行尾|Ctrl+G文件开头|Ctrl+H文件末尾|PageUp/Down翻页|Ctrl+X删除行|Ctrl+C复制行|Ctrl+V粘贴") | center
        }) | borderHeavy | color(Color::Orange3)
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

    // Ctrl+Z：撤销操作
    if (event == Event::CtrlZ) {
        Undo();
        return true;
    }

    // Ctrl+Y：重做操作
    if (event == Event::CtrlY) {
        Redo();
        return true;
    }

    // Ctrl+X：删除当前行
    if (event == Event::CtrlX) {
        SaveState(); // 保存状态用于撤销
        if (lines_.size() > 1) {
            lines_.erase(lines_.begin() + cursor_line_);
            // 调整光标位置
            if (cursor_line_ >= static_cast<int>(lines_.size())) {
                cursor_line_ = static_cast<int>(lines_.size()) - 1;
            }
            cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
        }
        return true;
    }

    // Ctrl+C：复制当前行
    if (event == Event::CtrlC) {
        clipboard_ = lines_[cursor_line_];
        return true;
    }

    // Ctrl+V：粘贴
    if (event == Event::CtrlV) {
        if (!clipboard_.empty()) {
            SaveState(); // 保存状态用于撤销
            lines_[cursor_line_].insert(cursor_col_, clipboard_);
            cursor_col_ += static_cast<int>(clipboard_.length());
        }
        return true;
    }

    // Ctrl+F：搜索功能（在编辑模式下）
    if (event == Event::CtrlF && edit_mode_) {
        // 这里可以集成一个简单的搜索对话框
        // 暂时使用一个简单的搜索实现
        return true;
    }

    // ==================== 编辑模式下的文本编辑操作 ====================

    // Backspace：删除光标前的字符，若在行首则合并上一行
    if (event == Event::Backspace) {
        SaveState(); // 保存状态用于撤销
        if (cursor_col_ > 0) {
            // 当前行非空，删除当前列前一个字符
            lines_[cursor_line_].erase(cursor_col_ - 1, 1);
            cursor_col_--;
        } else if (cursor_line_ > 0) {
            // 在行首且不是第一行，将当前行内容拼接到上一行末尾，并删除当前行
            int prev_line_length = static_cast<int>(lines_[cursor_line_ - 1].size());
            cursor_col_ = prev_line_length;
            lines_[cursor_line_ - 1] += lines_[cursor_line_];
            lines_.erase(lines_.begin() + cursor_line_);
            cursor_line_--;
        }
        return true;
    }

    // 光标左移：列索引大于 0 时才移动
    if (event == Event::ArrowLeft) {
        UpdateMoveSpeed(); // 更新移动速度
        if (cursor_col_ > 0) {
            cursor_col_--;
        } else if (cursor_line_ > 0) {
            // 在行首时，移动到上一行的末尾
            cursor_line_--;
            cursor_col_ = static_cast<int>(lines_[cursor_line_].size());
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_ = cursor_line_;
            }
        }
        return true;
    }
    // 光标右移：列索引小于当前行长度时移动
    if (event == Event::ArrowRight) {
        UpdateMoveSpeed(); // 更新移动速度
        int line_length = static_cast<int>(lines_[cursor_line_].size());
        if (cursor_col_ < line_length) {
            cursor_col_++;
        } else if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            // 在行尾时，移动到下一行的开头
            cursor_line_++;
            cursor_col_ = 0;
            if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
                scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
            }
        }
        return true;
    }

    // Vim风格快捷键：Home键 - 移动到行首
    if (event == Event::Home) {
        cursor_col_ = 0;
        return true;
    }
    // Vim风格快捷键：End键 - 移动到行尾
    if (event == Event::End) {
        cursor_col_ = static_cast<int>(lines_[cursor_line_].size());
        return true;
    }

    // 使用字符组合键来实现快速移动
    // 按 'g' 然后 'g' 移动到文件开头
    if (event.is_character() && event.character() == "g") {
        // 这里可以实现一个简单的状态机来处理 'gg' 命令
        // 暂时使用 Ctrl+G 作为替代
        return false; // 让其他处理程序处理
    }
    
    // 使用 Ctrl+G 移动到文件开头
    if (event == Event::CtrlG) {
        cursor_line_ = 0;
        cursor_col_ = 0;
        scroll_offset_ = 0;
        return true;
    }
    
    // 使用 Ctrl+H 移动到文件末尾
    if (event == Event::CtrlH) {
        cursor_line_ = static_cast<int>(lines_.size()) - 1;
        cursor_col_ = static_cast<int>(lines_[cursor_line_].size());
        if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
            scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
        }
        return true;
    }

    // PageUp：向上翻页
    if (event == Event::PageUp) {
        PageUp();
        return true;
    }
    // PageDown：向下翻页
    if (event == Event::PageDown) {
        PageDown();
        return true;
    }

    // 光标上移：行索引大于 0 时才移动，并保持列索引不超出新行长度
    // 同时如果光标行在滚动偏移之上，需要向上滚动视图
    if (event == Event::ArrowUp) {
        if (cursor_line_ > 0) {
            cursor_line_--;
            // 优化：缓存行长度，避免重复计算
            int new_line_length = static_cast<int>(lines_[cursor_line_].size());
            cursor_col_ = std::min(cursor_col_, new_line_length);
            
            // 优化滚动计算：只在必要时更新滚动偏移
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_ = cursor_line_;
            }
        }
        return true;
    }
    // 光标下移：行索引小于总行数减1 时移动，并保持列索引不超出新行长度
    // 如果光标行越过当前视图底部，需要向下滚动视图
    if (event == Event::ArrowDown) {
        int total_lines = static_cast<int>(lines_.size());
        if (cursor_line_ < total_lines - 1) {
            cursor_line_++;
            // 优化：缓存行长度，避免重复计算
            int new_line_length = static_cast<int>(lines_[cursor_line_].size());
            cursor_col_ = std::min(cursor_col_, new_line_length);
            
            // 优化滚动计算：只在必要时更新滚动偏移
            if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
                scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
            }
        }
        return true;
    }

    // 回车（Return）：在当前光标位置拆分行
    if (event == Event::Return) {
        SaveState(); // 保存状态用于撤销
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
        const std::string& ch = event.character();
        if (!ch.empty()) {
            SaveState(); // 保存状态用于撤销
            lines_[cursor_line_].insert(cursor_col_, ch);
            cursor_col_ += static_cast<int>(ch.length());
        }
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

// ---------------------------- 语法高亮相关方法 ----------------------------

/**
 * @brief 设置当前语言类型
 * @param lang 语言类型
 */
void VimLikeEditor::SetLanguage(FTB::Vim::Language lang) {
    syntax_highlighter_.SetLanguage(lang);
}

/**
 * @brief 获取当前语言类型
 * @return 语言类型
 */
FTB::Vim::Language VimLikeEditor::GetLanguage() const {
    return syntax_highlighter_.GetLanguage();
}

/**
 * @brief 设置文件名并自动检测语言类型
 * @param filename 文件名
 */
void VimLikeEditor::SetFilename(const std::string& filename) {
    current_filename_ = filename;
    FTB::Vim::Language detected_lang = FTB::Vim::SyntaxHighlighter::DetectLanguage(filename);
    syntax_highlighter_.SetLanguage(detected_lang);
}

// ---------------------------- 撤销/重做功能 ----------------------------

/**
 * @brief 保存当前状态到撤销历史
 */
void VimLikeEditor::SaveState() {
    // 删除当前索引之后的所有历史记录
    undo_history_.erase(undo_history_.begin() + undo_index_ + 1, undo_history_.end());
    
    // 添加新状态
    undo_history_.push_back(lines_);
    undo_index_ = static_cast<int>(undo_history_.size()) - 1;
    
    // 限制历史记录数量，避免内存过度使用
    const int MAX_HISTORY = 50;
    if (undo_history_.size() > MAX_HISTORY) {
        undo_history_.erase(undo_history_.begin());
        undo_index_--;
    }
}

/**
 * @brief 撤销上一次操作
 */
void VimLikeEditor::Undo() {
    if (CanUndo()) {
        undo_index_--;
        lines_ = undo_history_[undo_index_];
        
        // 调整光标位置，确保不超出范围
        cursor_line_ = std::min(cursor_line_, static_cast<int>(lines_.size()) - 1);
        if (cursor_line_ < 0) cursor_line_ = 0;
        cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
    }
}

/**
 * @brief 重做上一次撤销的操作
 */
void VimLikeEditor::Redo() {
    if (CanRedo()) {
        undo_index_++;
        lines_ = undo_history_[undo_index_];
        
        // 调整光标位置，确保不超出范围
        cursor_line_ = std::min(cursor_line_, static_cast<int>(lines_.size()) - 1);
        if (cursor_line_ < 0) cursor_line_ = 0;
        cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
    }
}

/**
 * @brief 检查是否可以撤销
 */
bool VimLikeEditor::CanUndo() const {
    return undo_index_ > 0;
}

/**
 * @brief 检查是否可以重做
 */
bool VimLikeEditor::CanRedo() const {
    return undo_index_ < static_cast<int>(undo_history_.size()) - 1;
}

// ---------------------------- 搜索和替换功能 ----------------------------

/**
 * @brief 搜索文本
 * @param query 搜索查询
 */
void VimLikeEditor::Search(const std::string& query) {
    if (query.empty()) return;
    
    // 从当前光标位置开始搜索
    for (int i = cursor_line_; i < static_cast<int>(lines_.size()); ++i) {
        size_t pos = lines_[i].find(query, (i == cursor_line_) ? cursor_col_ : 0);
        if (pos != std::string::npos) {
            cursor_line_ = i;
            cursor_col_ = static_cast<int>(pos);
            // 确保光标在可见区域内
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_ = cursor_line_;
            } else if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
                scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
            }
            return;
        }
    }
    
    // 如果没找到，从头开始搜索
    for (int i = 0; i < cursor_line_; ++i) {
        size_t pos = lines_[i].find(query);
        if (pos != std::string::npos) {
            cursor_line_ = i;
            cursor_col_ = static_cast<int>(pos);
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_ = cursor_line_;
            }
            return;
        }
    }
}

/**
 * @brief 替换当前光标位置的文本
 * @param old_text 要替换的文本
 * @param new_text 替换为的文本
 */
void VimLikeEditor::Replace(const std::string& old_text, const std::string& new_text) {
    if (old_text.empty()) return;
    
    SaveState(); // 保存状态用于撤销
    
    for (int i = 0; i < static_cast<int>(lines_.size()); ++i) {
        size_t pos = lines_[i].find(old_text);
        if (pos != std::string::npos) {
            lines_[i].replace(pos, old_text.length(), new_text);
            cursor_line_ = i;
            cursor_col_ = static_cast<int>(pos + new_text.length());
            return;
        }
    }
}

/**
 * @brief 替换所有匹配的文本
 * @param old_text 要替换的文本
 * @param new_text 替换为的文本
 */
void VimLikeEditor::ReplaceAll(const std::string& old_text, const std::string& new_text) {
    if (old_text.empty()) return;
    
    SaveState(); // 保存状态用于撤销
    
    int replace_count = 0;
    for (int i = 0; i < static_cast<int>(lines_.size()); ++i) {
        size_t pos = 0;
        while ((pos = lines_[i].find(old_text, pos)) != std::string::npos) {
            lines_[i].replace(pos, old_text.length(), new_text);
            pos += new_text.length();
            replace_count++;
        }
    }
}

// ---------------------------- 高级光标移动功能 ----------------------------

/**
 * @brief 移动到上一个单词的开头
 */
void VimLikeEditor::MoveToPreviousWord() {
    if (cursor_line_ < 0 || cursor_line_ >= static_cast<int>(lines_.size())) return;
    
    const std::string& line = lines_[cursor_line_];
    
    // 如果当前在行首，尝试移动到上一行的末尾
    if (cursor_col_ == 0) {
        if (cursor_line_ > 0) {
            cursor_line_--;
            cursor_col_ = static_cast<int>(lines_[cursor_line_].size());
            if (cursor_line_ < scroll_offset_) {
                scroll_offset_ = cursor_line_;
            }
        }
        return;
    }
    
    // 跳过当前单词的剩余部分
    int pos = cursor_col_ - 1;
    while (pos > 0 && std::isalnum(line[pos])) {
        pos--;
    }
    
    // 跳过空白字符
    while (pos > 0 && std::isspace(line[pos])) {
        pos--;
    }
    
    // 找到单词开头
    while (pos > 0 && std::isalnum(line[pos - 1])) {
        pos--;
    }
    
    cursor_col_ = pos;
}

/**
 * @brief 移动到下一个单词的开头
 */
void VimLikeEditor::MoveToNextWord() {
    if (cursor_line_ < 0 || cursor_line_ >= static_cast<int>(lines_.size())) return;
    
    const std::string& line = lines_[cursor_line_];
    int line_length = static_cast<int>(line.size());
    
    // 如果当前在行尾，尝试移动到下一行的开头
    if (cursor_col_ >= line_length) {
        if (cursor_line_ < static_cast<int>(lines_.size()) - 1) {
            cursor_line_++;
            cursor_col_ = 0;
            if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
                scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
            }
        }
        return;
    }
    
    // 跳过当前单词
    int pos = cursor_col_;
    while (pos < line_length && std::isalnum(line[pos])) {
        pos++;
    }
    
    // 跳过空白字符
    while (pos < line_length && std::isspace(line[pos])) {
        pos++;
    }
    
    cursor_col_ = pos;
}

/**
 * @brief 向上翻页
 */
void VimLikeEditor::PageUp() {
    int page_size = MAX_VISIBLE_LINES - 2; // 保留一些重叠
    int new_line = cursor_line_ - page_size;
    
    if (new_line < 0) {
        new_line = 0;
    }
    
    cursor_line_ = new_line;
    cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
    
    // 调整滚动偏移
    if (cursor_line_ < scroll_offset_) {
        scroll_offset_ = cursor_line_;
    }
}

/**
 * @brief 向下翻页
 */
void VimLikeEditor::PageDown() {
    int page_size = MAX_VISIBLE_LINES - 2; // 保留一些重叠
    int total_lines = static_cast<int>(lines_.size());
    int new_line = cursor_line_ + page_size;
    
    if (new_line >= total_lines) {
        new_line = total_lines - 1;
    }
    
    cursor_line_ = new_line;
    cursor_col_ = std::min(cursor_col_, static_cast<int>(lines_[cursor_line_].size()));
    
    // 调整滚动偏移
    if (cursor_line_ >= scroll_offset_ + MAX_VISIBLE_LINES) {
        scroll_offset_ = cursor_line_ - MAX_VISIBLE_LINES + 1;
    }
}

/**
 * @brief 更新移动速度，支持连续按键加速
 */
void VimLikeEditor::UpdateMoveSpeed() {
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_move_time_).count();
    
    // 如果移动间隔小于200ms，认为是连续移动
    if (time_diff < 200) {
        move_repeat_count_++;
    } else {
        move_repeat_count_ = 0;
    }
    
    last_move_time_ = now;
}
