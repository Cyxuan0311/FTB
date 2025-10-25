#pragma once

#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>

namespace FTB {
namespace Vim {

// 表格对齐方式（在命名空间级别声明，便于在实现文件中直接使用）
enum class TableAlignment { LEFT, CENTER, RIGHT };

class MDTransformer {
public:
    MDTransformer();
    ~MDTransformer();

    // 将整段 Markdown 文本转换为 Element
    ftxui::Element TransformToElement(const std::string& markdown_text);
    // 将按行的 Markdown 文本转换为 Element
    ftxui::Element TransformToElement(const std::vector<std::string>& markdown_lines);

    // 预览滚动控制
    void SetScrollOffset(int offset);
    int GetScrollOffset() const;
    void ScrollBy(int delta);
    void ResetScroll();

private:
    // 行级解析与渲染
    ftxui::Element ParseMarkdownLine(const std::string& line);
    ftxui::Element ParseHeader(const std::string& line, int level);
    ftxui::Element ParseCodeBlock(const std::vector<std::string>& lines, int& start_index);
    ftxui::Element ParseListItem(const std::string& line);
    ftxui::Element ParseLinks(const std::string& text);
    ftxui::Element ParseFormatting(const std::string& text);

    // 表格解析
    bool IsTableLine(const std::string& line);
    ftxui::Element ParseTable(const std::vector<std::string>& table_lines);
    bool IsTableSeparatorLine(const std::string& line);
    std::vector<std::string> SplitTableRow(const std::string& line);
    std::vector<TableAlignment> ParseTableAlignments(const std::string& separator_line);

    // 滚动偏移
    int scroll_offset_;
};

} // namespace Vim
} // namespace FTB


