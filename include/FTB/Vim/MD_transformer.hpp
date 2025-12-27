#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <ftxui/dom/elements.hpp>

namespace FTB {
namespace Vim {

// Markdown 元素类型（参考 flow）
enum class MarkdownType {
    NORMAL,
    HEADING1, HEADING2, HEADING3, HEADING4, HEADING5, HEADING6,
    BOLD, ITALIC, BOLD_ITALIC,
    CODE_INLINE, CODE_BLOCK,
    LIST_ITEM, LIST_ORDERED,
    QUOTE,
    LINK, IMAGE,
    HR,
    TABLE_ROW, TABLE_SEPARATOR,
    EMPTY
};

// 表格对齐方式
enum class TableAlignment { 
    LEFT, 
    CENTER, 
    RIGHT 
};

// 语法高亮类型（预留，参考 flow_syntax_highlighter）
enum class SyntaxType {
    NONE,
    C, CPP,
    PYTHON,
    JAVASCRIPT, TYPESCRIPT,
    BASH,
    GO, RUST, JAVA,
    JSON, XML,
    SQL, YAML,
    MARKDOWN
};

class MDTransformer {
public:
    MDTransformer();
    ~MDTransformer();

    // 主要转换方法
    ftxui::Element TransformToElement(const std::string& markdown_text);
    ftxui::Element TransformToElement(const std::vector<std::string>& markdown_lines);

    // 滚动控制
    void SetScrollOffset(int offset);
    int GetScrollOffset() const;
    void ScrollBy(int delta);
    void ResetScroll();

private:
    // ========== 核心解析方法（参考 flow_parser.c） ==========
    
    // 解析 Markdown 行类型
    MarkdownType ParseLineType(const std::string& line, int* level = nullptr);
    
    // 提取文本内容
    std::string ExtractHeadingText(const std::string& line);
    std::string ExtractListText(const std::string& line);
    std::string ExtractQuoteText(const std::string& line);
    
    // 行内格式解析（支持粗体、斜体、代码、链接、图片）
    ftxui::Element ParseInlineFormatting(const std::string& text);
    
    // ========== 渲染方法（参考 flow_renderer.c） ==========
    
    // 渲染单行
    ftxui::Element RenderLine(const std::string& line, MarkdownType type, int level);
    
    // 渲染标题
    ftxui::Element RenderHeading(const std::string& text, int level);
    
    // 渲染代码块
    ftxui::Element RenderCodeBlock(const std::vector<std::string>& lines, 
                                   const std::string& language = "");
    
    // 渲染列表项
    ftxui::Element RenderListItem(const std::string& text, int level, bool ordered = false);
    
    // 渲染引用块
    ftxui::Element RenderQuote(const std::string& text);
    
    // 渲染水平线
    ftxui::Element RenderHorizontalRule();
    
    // ========== 表格处理（参考 flow_formatter.c） ==========
    
    // 表格检测
    bool IsTableRow(const std::string& line);
    bool IsTableSeparator(const std::string& line);
    
    // 表格解析
    std::vector<std::string> SplitTableRow(const std::string& line);
    std::vector<TableAlignment> ParseTableAlignments(const std::string& separator);
    
    // 表格渲染
    ftxui::Element RenderTable(const std::vector<std::string>& table_lines);
    
    // 表格列宽计算（精确处理 CJK 和 emoji）
    std::vector<int> CalculateTableColumnWidths(
        const std::vector<std::string>& header,
        const std::vector<std::vector<std::string>>& data);
    
    // 格式化表格单元格
    std::string FormatTableCell(const std::string& content, 
                               int width, 
                               TableAlignment align);
    
    // ========== 字符宽度计算（参考 flow_formatter.c） ==========
    
    // UTF-8 解码
    static int DecodeUtf8(const char* s, uint32_t* codepoint);
    
    // 判断字符类型
    static bool IsEastAsianWide(uint32_t codepoint);
    static bool IsWideEmoji(uint32_t codepoint);
    
    // 计算显示宽度（考虑 CJK、emoji、ANSI 代码）
    static int CalculateDisplayWidth(const std::string& text);
    
    // ========== 辅助方法 ==========
    
    // 语法检测（预留）
    SyntaxType DetectSyntaxType(const std::string& language);
    
    // 去除 HTML 标签
    std::string StripHtmlTags(const std::string& text);
    
    // 成员变量
    int scroll_offset_;
};

} // namespace Vim
} // namespace FTB
