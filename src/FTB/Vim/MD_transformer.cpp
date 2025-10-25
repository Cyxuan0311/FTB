#include "FTB/Vim/MD_transformer.hpp"
#include <ftxui/dom/elements.hpp>
#include <regex>
#include <algorithm>

using namespace ftxui;

namespace FTB {
namespace Vim {

// ---------------------------- 构造与析构 ----------------------------

MDTransformer::MDTransformer() : scroll_offset_(0) {}

MDTransformer::~MDTransformer() {}

// ---------------------------- 主要转换方法 ----------------------------

ftxui::Element MDTransformer::TransformToElement(const std::string& markdown_text) {
    // 将文本按行分割
    std::vector<std::string> lines;
    std::istringstream stream(markdown_text);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return TransformToElement(lines);
}

ftxui::Element MDTransformer::TransformToElement(const std::vector<std::string>& markdown_lines) {
    // 安全检查：如果输入为空，返回空内容
    if (markdown_lines.empty()) {
        return text("空内容");
    }
    
    Elements elements;
    bool in_code_block = false;
    bool in_table = false;
    std::string code_block_content;
    std::string code_language;
    std::vector<std::string> table_lines;
    
    for (size_t i = 0; i < markdown_lines.size(); ++i) {
        const std::string& line = markdown_lines[i];
        
        // 处理代码块
        if (line.length() >= 3 && line.substr(0, 3) == "```") {
            if (!in_code_block) {
                // 开始代码块
                in_code_block = true;
                code_language = line.length() > 3 ? line.substr(3) : "";
                code_block_content.clear();
            } else {
                // 结束代码块
                in_code_block = false;
                if (!code_block_content.empty()) {
                    // 移除最后的换行符
                    if (code_block_content.back() == '\n') {
                        code_block_content.pop_back();
                    }
                    // 安全检查：避免空代码块
                    if (!code_block_content.empty()) {
                        int dummy_index = 0;
                        elements.push_back(ParseCodeBlock({code_block_content}, dummy_index));
                    }
                }
                code_block_content.clear();
            }
            continue;
        }
        
        if (in_code_block) {
            code_block_content += line + "\n";
            continue;
        }
        
        // 检查表格开始
        if (!in_table && IsTableLine(line)) {
            in_table = true;
            table_lines.clear();
            table_lines.push_back(line);
            continue;
        }
        
        // 处理表格行
        if (in_table) {
            if (IsTableLine(line)) {
                table_lines.push_back(line);
            } else {
                // 表格结束
                in_table = false;
                if (!table_lines.empty()) {
                    // 安全检查：确保表格有有效内容
                    bool has_valid_content = false;
                    for (const auto& table_line : table_lines) {
                        if (!table_line.empty() && table_line.find('|') != std::string::npos) {
                            has_valid_content = true;
                            break;
                        }
                    }
                    if (has_valid_content) {
                        elements.push_back(ParseTable(table_lines));
                    }
                }
                table_lines.clear();
                
                // 处理当前行
                if (!line.empty()) {
                    elements.push_back(ParseMarkdownLine(line));
                } else {
                    elements.push_back(text(""));
                }
            }
            continue;
        }
        
        // 处理普通行
        if (!line.empty()) {
            elements.push_back(ParseMarkdownLine(line));
        } else {
            elements.push_back(text(""));
        }
    }
    
    // 如果代码块没有正确关闭，也要处理
    if (in_code_block && !code_block_content.empty()) {
        // 移除最后的换行符
        if (code_block_content.back() == '\n') {
            code_block_content.pop_back();
        }
        int dummy_index = 0;
        elements.push_back(ParseCodeBlock({code_block_content}, dummy_index));
    }
    
    // 如果表格没有正确关闭，也要处理
    if (in_table && !table_lines.empty()) {
        elements.push_back(ParseTable(table_lines));
    }
    
    // 应用滚动偏移
    if (scroll_offset_ > 0 && scroll_offset_ < static_cast<int>(elements.size())) {
        elements.erase(elements.begin(), elements.begin() + scroll_offset_);
    }
    
    return vbox(elements);
}

// ---------------------------- 滚动控制 ----------------------------

void MDTransformer::SetScrollOffset(int offset) {
    scroll_offset_ = std::max(0, offset);
}

int MDTransformer::GetScrollOffset() const {
    return scroll_offset_;
}

void MDTransformer::ScrollBy(int delta) {
    scroll_offset_ = std::max(0, scroll_offset_ + delta);
}

void MDTransformer::ResetScroll() {
    scroll_offset_ = 0;
}

// ---------------------------- 私有解析方法 ----------------------------

ftxui::Element MDTransformer::ParseMarkdownLine(const std::string& line) {
    // 检查标题
    if (line.length() > 0 && line[0] == '#') {
        int level = 0;
        while (level < static_cast<int>(line.length()) && line[level] == '#') {
            level++;
        }
        if (level > 0 && level < static_cast<int>(line.length()) && line[level] == ' ') {
            return ParseHeader(line.substr(level + 1), level);
        }
    }
    
    // 检查列表项
    if ((line.length() >= 2 && line.substr(0, 2) == "- ") || 
        (line.length() >= 2 && line.substr(0, 2) == "* ")) {
        return ParseListItem(line.substr(2));
    }
    
    // 检查数字列表
    std::regex numbered_list_regex(R"(^\d+\.\s+(.+))");
    std::smatch match;
    if (std::regex_match(line, match, numbered_list_regex)) {
        // 数字列表显示 - 使用白色
        std::string numbered_display = "1. " + match[1].str();
        return text(numbered_display) | color(Color::White);
    }
    
    // 检查引用
    if (line.length() >= 2 && line.substr(0, 2) == "> ") {
        // 引用显示 - 使用白色
        std::string quote_display = "> " + line.substr(2);
        return text(quote_display) | color(Color::White) | dim;
    }
    
    // 普通段落
    return ParseFormatting(line);
}

ftxui::Element MDTransformer::ParseHeader(const std::string& line, int level) {
    // 标题显示 - 使用灰色加粗
    std::string header_display = "";
    
    // 根据级别设置不同的显示效果
    switch (level) {
        case 1:
            // H1: 大标题
            header_display += "\n";
            header_display += line + "\n";
            header_display += "═══════════════════════════════════════\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
            
        case 2:
            // H2: 中标题
            header_display += "\n";
            header_display += line + "\n";
            header_display += "───────────────────────────────────────\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
            
        case 3:
            // H3: 小标题
            header_display += line + "\n";
            header_display += "───────────────────────────────────────\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
            
        case 4:
            // H4: 更小的标题
            header_display += line + "\n";
            header_display += "───────────────────────────────────────\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
            
        case 5:
            // H5: 最小标题
            header_display += line + "\n";
            header_display += "───────────────────────────────────────\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
            
        default:
            // H6: 最小标题
            header_display += line + "\n";
            header_display += "───────────────────────────────────────\n";
            header_display += "\n";
            return text(header_display) | color(Color::GrayLight) | bold;
    }
}

ftxui::Element MDTransformer::ParseCodeBlock(const std::vector<std::string>& lines, int& /* start_index */) {
    if (lines.empty()) {
        return text("");
    }
    
    // 代码块显示 - 优化显示效果
    std::string code_display = "";
    
    // 限制最大宽度，避免在终端中显示过宽
    const size_t MAX_WIDTH = 80;
    size_t max_length = 0;
    for (const auto& line : lines) {
        max_length = std::max(max_length, line.length());
    }
    max_length = std::min(max_length, MAX_WIDTH);
    
    // 创建标题
    std::string title = "代码块";
    if (lines.size() > 0) {
        // 尝试检测语言类型
        std::string first_line = lines[0];
        if (first_line.find("#include") != std::string::npos || first_line.find("int main") != std::string::npos) {
            title += " (C++)";
        } else if (first_line.find("def ") != std::string::npos || first_line.find("import ") != std::string::npos) {
            title += " (Python)";
        } else if (first_line.find("function") != std::string::npos || first_line.find("const ") != std::string::npos) {
            title += " (JavaScript)";
        } else {
            title += " (Text)";
        }
    }
    
    // 创建顶部边框
    std::string top_border = "┌─ " + title + " ";
    while (top_border.length() < max_length + 4) {
        top_border += "─";
    }
    top_border += "┐\n";
    
    code_display += top_border;
    
    // 代码行 - 支持文本换行
    for (const auto& line : lines) {
        std::string code_line = "│ " + line;
        
        // 如果行太长，进行换行处理
        if (line.length() > MAX_WIDTH) {
            std::string truncated_line = line.substr(0, MAX_WIDTH - 3) + "...";
            code_line = "│ " + truncated_line;
        }
        
        // 填充空格到指定长度
        while (code_line.length() < top_border.length() - 1) {
            code_line += " ";
        }
        code_line += " │\n";
        code_display += code_line;
    }
    
    // 底部边框
    std::string bottom_border = "└";
    while (bottom_border.length() < top_border.length() - 1) {
        bottom_border += "─";
    }
    bottom_border += "┘";
    
    code_display += bottom_border;
    
    return text(code_display) | color(Color::Cyan) | bgcolor(Color::DarkBlue);
}

ftxui::Element MDTransformer::ParseListItem(const std::string& line) {
    // 列表项显示 - 使用白色
    std::string list_display = "• " + line;
    return text(list_display) | color(Color::White);
}

ftxui::Element MDTransformer::ParseLinks(const std::string& text) {
    // 链接显示 - 使用蓝色下划线
    std::regex link_regex(R"(\[([^\]]+)\]\(([^)]+)\))");
    std::string result = std::regex_replace(text, link_regex, std::string("[$1]($2)"));
    
    return ftxui::text(result) | color(Color::Blue) | underlined;
}

ftxui::Element MDTransformer::ParseFormatting(const std::string& text) {
    // 检查是否包含链接
    if (text.find("[") != std::string::npos && text.find("](") != std::string::npos) {
        return ParseLinks(text);
    }
    
    // 检查是否包含行内代码
    if (text.find("`") != std::string::npos) {
        std::string result = text;
        std::regex code_regex(R"(`([^`]+)`)");
        result = std::regex_replace(result, code_regex, std::string("`$1`"));
        return ftxui::text(result) | color(Color::Cyan) | bgcolor(Color::DarkBlue);
    }
    
    // 检查是否包含粗体
    if (text.find("**") != std::string::npos) {
        std::string result = text;
        std::regex bold_regex(R"(\*\*([^*]+)\*\*)");
        result = std::regex_replace(result, bold_regex, std::string("**$1**"));
        return ftxui::text(result) | color(Color::White) | bold;
    }
    
    // 检查是否包含斜体
    if (text.find("*") != std::string::npos && text.find("**") == std::string::npos) {
        std::string result = text;
        std::regex italic_regex(R"(\*([^*]+)\*)");
        result = std::regex_replace(result, italic_regex, std::string("*$1*"));
        return ftxui::text(result) | color(Color::White) | dim;
    }
    
    // 普通文本 - 使用白色显示
    return ftxui::text(text) | color(Color::White);
}

// ---------------------------- 表格解析方法 ----------------------------

/**
 * @brief 检查是否为表格行
 * @param line 要检查的行
 * @return 如果是表格行返回true
 */
bool MDTransformer::IsTableLine(const std::string& line) {
    // 检查是否包含管道符分隔符
    if (line.find('|') == std::string::npos) {
        return false;
    }
    
    // 检查是否为分隔符行（包含---或:---等）
    if (line.find("---") != std::string::npos || 
        line.find(":---") != std::string::npos ||
        line.find("---:") != std::string::npos ||
        line.find(":---:") != std::string::npos) {
        return true;
    }
    
    // 检查是否包含多个管道符（至少2个）
    size_t pipe_count = 0;
    for (char c : line) {
        if (c == '|') pipe_count++;
    }
    
    return pipe_count >= 2;
}

/**
 * @brief 解析表格
 * @param table_lines 表格行列表
 * @return 表格Element
 */
ftxui::Element MDTransformer::ParseTable(const std::vector<std::string>& table_lines) {
    if (table_lines.empty()) {
        return text("");
    }
    
    // 分离表头和分隔符行
    std::vector<std::string> header_row;
    std::vector<std::vector<std::string>> data_rows;
    std::vector<TableAlignment> alignments;
    
    bool found_separator = false;
    
    for (const auto& line : table_lines) {
        if (IsTableSeparatorLine(line)) {
            // 解析对齐方式
            alignments = ParseTableAlignments(line);
            found_separator = true;
            continue;
        }
        
        if (!found_separator) {
            // 表头行
            header_row = SplitTableRow(line);
        } else {
            // 数据行
            data_rows.push_back(SplitTableRow(line));
        }
    }
    
    // 如果没有找到分隔符，所有行都是数据行
    if (!found_separator) {
        data_rows.clear();
        for (const auto& line : table_lines) {
            data_rows.push_back(SplitTableRow(line));
        }
        // 默认左对齐
        alignments.resize(header_row.size(), TableAlignment::LEFT);
    }
    
    // 构建表格
    Elements table_elements;
    
    // 表头
    if (!header_row.empty()) {
        Elements header_cells;
        for (size_t i = 0; i < header_row.size(); ++i) {
            ftxui::Element cell = ParseFormatting(" " + header_row[i] + " ") 
                | bold 
                | color(Color::White) 
                | bgcolor(Color::Blue)
                | center;
            header_cells.push_back(cell);
        }
        table_elements.push_back(hbox(header_cells) | borderRounded);
        
        // 优化分隔线
        std::string separator_line = "";
        for (size_t i = 0; i < header_row.size(); ++i) {
            if (i > 0) separator_line += "┼";
            separator_line += "─────";
        }
        table_elements.push_back(ftxui::text(separator_line) | color(Color::Cyan) | center);
    }
    
    // 数据行
    for (size_t row_idx = 0; row_idx < data_rows.size(); ++row_idx) {
        const auto& row = data_rows[row_idx];
        Elements row_cells;
        
        for (size_t i = 0; i < row.size() && i < header_row.size(); ++i) {
            // 解析单元格内的格式化文本
            ftxui::Element cell = ParseFormatting(" " + row[i] + " ");
            
            // 根据对齐方式设置样式
            switch (alignments[i]) {
                case TableAlignment::CENTER:
                    cell = cell | center;
                    break;
                case TableAlignment::RIGHT:
                    cell = cell | center; // 暂时使用居中
                    break;
                default:
                    break;
            }
            
            // 优化交替行颜色和边框
            if (row_idx % 2 == 0) {
                cell = cell | bgcolor(Color::GrayDark) | border;
            } else {
                cell = cell | bgcolor(Color::DarkBlue) | border;
            }
            
            row_cells.push_back(cell);
        }
        table_elements.push_back(hbox(row_cells) | borderRounded);
    }
    
    // 表格显示 - 优化显示效果
    std::string table_display = "";
    
    if (header_row.empty()) {
        return text("空表格");
    }
    
    // 限制最大宽度，避免在终端中显示过宽
    const size_t MAX_COLUMN_WIDTH = 20;
    const size_t MAX_TOTAL_WIDTH = 80;
    
    // 计算每列的最大宽度
    std::vector<size_t> column_widths(header_row.size(), 0);
    for (size_t i = 0; i < header_row.size(); ++i) {
        column_widths[i] = std::max(column_widths[i], header_row[i].length());
    }
    
    for (const auto& row : data_rows) {
        for (size_t i = 0; i < row.size() && i < header_row.size(); ++i) {
            column_widths[i] = std::max(column_widths[i], row[i].length());
        }
    }
    
    // 限制列宽，避免表格过宽
    for (size_t i = 0; i < column_widths.size(); ++i) {
        column_widths[i] = std::min(column_widths[i], MAX_COLUMN_WIDTH);
    }
    
    // 检查总宽度，如果太宽则进一步限制
    size_t total_width = 0;
    for (size_t i = 0; i < column_widths.size(); ++i) {
        total_width += column_widths[i] + 3; // +3 for borders and spaces
    }
    
    if (total_width > MAX_TOTAL_WIDTH) {
        size_t reduction = total_width - MAX_TOTAL_WIDTH;
        for (size_t i = 0; i < column_widths.size() && reduction > 0; ++i) {
            size_t reduce = std::min(reduction, column_widths[i] - 5); // 保持最小宽度5
            column_widths[i] -= reduce;
            reduction -= reduce;
        }
    }
    
    // 创建表格顶部边框
    table_display += "+";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (size_t j = 0; j < column_widths[i] + 2; ++j) {
            table_display += "-";
        }
        if (i < header_row.size() - 1) {
            table_display += "+";
        }
    }
    table_display += "+\n";
    
    // 表头 - 支持文本截断
    std::string header_display = "|";
    for (size_t i = 0; i < header_row.size(); ++i) {
        std::string cell_content = header_row[i];
        
        // 截断过长的文本
        if (cell_content.length() > column_widths[i]) {
            cell_content = cell_content.substr(0, column_widths[i] - 3) + "...";
        }
        
        cell_content = " " + cell_content;
        // 填充空格到指定宽度
        while (cell_content.length() < column_widths[i] + 2) {
            cell_content += " ";
        }
        header_display += cell_content;
        if (i < header_row.size() - 1) {
            header_display += " |";
        }
    }
    header_display += " |\n";
    table_display += header_display;
    
    // 分隔线
    table_display += "+";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (size_t j = 0; j < column_widths[i] + 2; ++j) {
            table_display += "-";
        }
        if (i < header_row.size() - 1) {
            table_display += "+";
        }
    }
    table_display += "+\n";
    
    // 数据行 - 支持文本截断
    for (const auto& row : data_rows) {
        std::string row_display = "|";
        for (size_t i = 0; i < row.size() && i < header_row.size(); ++i) {
            std::string cell_content = row[i];
            
            // 截断过长的文本
            if (cell_content.length() > column_widths[i]) {
                cell_content = cell_content.substr(0, column_widths[i] - 3) + "...";
            }
            
            cell_content = " " + cell_content;
            // 填充空格到指定宽度
            while (cell_content.length() < column_widths[i] + 2) {
                cell_content += " ";
            }
            row_display += cell_content;
            if (i < row.size() - 1 && i < header_row.size() - 1) {
                row_display += " |";
            }
        }
        row_display += " |\n";
        table_display += row_display;
    }
    
    // 底部边框
    table_display += "+";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (size_t j = 0; j < column_widths[i] + 2; ++j) {
            table_display += "-";
        }
        if (i < header_row.size() - 1) {
            table_display += "+";
        }
    }
    table_display += "+";
    
    return text(table_display) | color(Color::White);
}

/**
 * @brief 检查是否为表格分隔符行
 * @param line 要检查的行
 * @return 如果是分隔符行返回true
 */
bool MDTransformer::IsTableSeparatorLine(const std::string& line) {
    return line.find("---") != std::string::npos || 
           line.find(":---") != std::string::npos ||
           line.find("---:") != std::string::npos ||
           line.find(":---:") != std::string::npos;
}

/**
 * @brief 分割表格行
 * @param line 表格行
 * @return 分割后的单元格列表
 */
std::vector<std::string> MDTransformer::SplitTableRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string current_cell;
    bool in_cell = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '|') {
            if (in_cell) {
                // 去除首尾空格
                std::string trimmed = current_cell;
                while (!trimmed.empty() && trimmed[0] == ' ') {
                    trimmed.erase(0, 1);
                }
                while (!trimmed.empty() && trimmed.back() == ' ') {
                    trimmed.pop_back();
                }
                cells.push_back(trimmed);
                current_cell.clear();
            }
            in_cell = !in_cell;
        } else if (in_cell) {
            current_cell += c;
        }
    }
    
    // 处理最后一个单元格
    if (!current_cell.empty()) {
        std::string trimmed = current_cell;
        while (!trimmed.empty() && trimmed[0] == ' ') {
            trimmed.erase(0, 1);
        }
        while (!trimmed.empty() && trimmed.back() == ' ') {
            trimmed.pop_back();
        }
        cells.push_back(trimmed);
    }
    
    return cells;
}

/**
 * @brief 解析表格对齐方式
 * @param separator_line 分隔符行
 * @return 对齐方式列表
 */
std::vector<TableAlignment> MDTransformer::ParseTableAlignments(const std::string& separator_line) {
    std::vector<TableAlignment> alignments;
    std::vector<std::string> cells = SplitTableRow(separator_line);
    
    for (const auto& cell : cells) {
        std::string trimmed = cell;
        while (!trimmed.empty() && trimmed[0] == ' ') {
            trimmed.erase(0, 1);
        }
        while (!trimmed.empty() && trimmed.back() == ' ') {
            trimmed.pop_back();
        }
        
        if (trimmed.find(":---:") != std::string::npos) {
            alignments.push_back(TableAlignment::CENTER);
        } else if (trimmed.find("---:") != std::string::npos) {
            alignments.push_back(TableAlignment::RIGHT);
        } else {
            alignments.push_back(TableAlignment::LEFT);
        }
    }
    
    return alignments;
}

} // namespace Vim
} // namespace FTB