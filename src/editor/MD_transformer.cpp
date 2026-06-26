#include "editor/MD_transformer.hpp"
#include <ftxui/dom/elements.hpp>
#include <regex>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cctype>

using namespace ftxui;

namespace FTB {
namespace Editor {

// ==================== 字符宽度计算（直接移植自 flow_formatter.c） ====================

// UTF-8 解码，返回 Unicode 码点和字节数
int MDTransformer::DecodeUtf8(const char* s, uint32_t* codepoint) {
    unsigned char c = (unsigned char)*s;
    
    if (c < 0x80) {
        // 单字节字符（ASCII）
        *codepoint = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2 字节字符
        *codepoint = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        // 3 字节字符
        *codepoint = ((c & 0x0F) << 12) | 
                     (((unsigned char)s[1] & 0x3F) << 6) | 
                     ((unsigned char)s[2] & 0x3F);
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        // 4 字节字符
        *codepoint = ((c & 0x07) << 18) | 
                     (((unsigned char)s[1] & 0x3F) << 12) | 
                     (((unsigned char)s[2] & 0x3F) << 6) | 
                     ((unsigned char)s[3] & 0x3F);
        return 4;
    }
    
    // 无效的 UTF-8 序列
    *codepoint = 0xFFFD;  // 替换字符
    return 1;
}

// 判断是否为 East Asian Wide/Fullwidth 字符（直接移植自 flow_formatter.c）
bool MDTransformer::IsEastAsianWide(uint32_t codepoint) {
    // CJK 统一表意文字（常用汉字）
    if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||     // CJK Unified Ideographs
        (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||     // CJK Extension A
        (codepoint >= 0x20000 && codepoint <= 0x2A6DF) ||   // CJK Extension B
        (codepoint >= 0x2A700 && codepoint <= 0x2B73F) ||   // CJK Extension C
        (codepoint >= 0x2B740 && codepoint <= 0x2B81F) ||   // CJK Extension D
        (codepoint >= 0x2B820 && codepoint <= 0x2CEAF) ||   // CJK Extension E
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||     // CJK Compatibility Ideographs
        (codepoint >= 0x2F800 && codepoint <= 0x2FA1F)) {   // CJK Compatibility Supplement
        return true;
    }
    
    // 全角字符
    if ((codepoint >= 0xFF01 && codepoint <= 0xFF60) ||     // Fullwidth Forms
        (codepoint >= 0xFFE0 && codepoint <= 0xFFE6)) {
        return true;
    }
    
    // Hangul Syllables (韩文音节)
    if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) {
        return true;
    }
    
    // Hiragana & Katakana (日文平假名和片假名)
    if ((codepoint >= 0x3040 && codepoint <= 0x309F) ||     // Hiragana
        (codepoint >= 0x30A0 && codepoint <= 0x30FF)) {     // Katakana
        return true;
    }
    
    // 其他 East Asian 宽字符
    if ((codepoint >= 0x1100 && codepoint <= 0x115F) ||     // Hangul Jamo
        (codepoint >= 0x2E80 && codepoint <= 0x2EFF) ||     // CJK Radicals Supplement
        (codepoint >= 0x2F00 && codepoint <= 0x2FDF) ||     // Kangxi Radicals
        (codepoint >= 0x3000 && codepoint <= 0x303F) ||     // CJK Symbols and Punctuation
        (codepoint >= 0x3200 && codepoint <= 0x32FF) ||     // Enclosed CJK Letters
        (codepoint >= 0x3300 && codepoint <= 0x33FF)) {     // CJK Compatibility
        return true;
    }
    
    return false;
}

// 判断是否为宽字符 emoji（直接移植自 flow_emoji.c）
bool MDTransformer::IsWideEmoji(uint32_t codepoint) {
    // Emoji & Symbols
    if ((codepoint >= 0x1F300 && codepoint <= 0x1F9FF) ||   // Misc Symbols and Pictographs
        (codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||   // Emoticons
        (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||   // Transport and Map
        (codepoint >= 0x2600 && codepoint <= 0x26FF) ||     // Misc symbols
        (codepoint >= 0x2700 && codepoint <= 0x27BF) ||     // Dingbats
        (codepoint >= 0x1FA00 && codepoint <= 0x1FA6F) ||   // Extended Pictographic
        (codepoint >= 0x1F900 && codepoint <= 0x1F9FF)) {   // Supplemental Symbols and Pictographs
        return true;
    }
    return false;
}

// 计算显示宽度（精确处理 East Asian 字符、emoji、ANSI 代码）
// 直接移植自 flow_formatter.c 的 calculate_display_width
int MDTransformer::CalculateDisplayWidth(const std::string& text) {
    if (text.empty()) return 0;
    
    int width = 0;
    const char* p = text.c_str();
    
    while (*p) {
        // 处理 ANSI 转义序列：\033[ ... m（不计入宽度）
        if (*p == '\033' && p[1] == '[') {
            p += 2;  // 跳过 \033[
            // 跳过所有参数和终止符 'm'
            while (*p && *p != 'm') {
                p++;
            }
            if (*p == 'm') {
                p++;  // 跳过 'm'
            }
            continue;  // ANSI 代码不计入宽度
        }
        
        // 处理普通字符
        uint32_t codepoint;
        int bytes = DecodeUtf8(p, &codepoint);
        
        if (bytes == 0 || bytes > 4) {
            // 无效的 UTF-8 序列，跳过
            p++;
            continue;
        }
        
        // 首先检查是否为宽字符 emoji（在终端中占 2 个显示宽度）
        if (IsWideEmoji(codepoint)) {
            width += 2;
        } else if (IsEastAsianWide(codepoint)) {
            // East Asian Wide 字符占 2 个显示宽度（中文字符等）
            width += 2;
        } else if (codepoint >= 0x20 && codepoint < 0x7F) {
            // 可打印 ASCII 字符占 1 个宽度
            width += 1;
        } else if (codepoint == 0x09) {
            // Tab 字符占 8 个宽度（可调整）
            width += 8;
        } else if (codepoint == 0x200D) {
            // Zero Width Joiner (ZWJ) - 用于组合 emoji，不计入宽度
            width += 0;
        } else if (codepoint >= 0xFE00 && codepoint <= 0xFE0F) {
            // Variation Selectors - 用于 emoji 变体，不计入宽度
            width += 0;
        } else if (codepoint >= 0x80) {
            // 其他 Unicode 字符默认占 1 个宽度
            width += 1;
        }
        // 控制字符（< 0x20，除了 Tab）不计入宽度
        
        p += bytes;
    }
    
    return width;
}

// ==================== 构造与析构 ====================

MDTransformer::MDTransformer() : scroll_offset_(0) {}

MDTransformer::~MDTransformer() {}

// ==================== Markdown 行解析（移植自 flow_parser.c） ====================

// 解析 Markdown 行类型
MarkdownType MDTransformer::ParseLineType(const std::string& line, int* level) {
    if (line.empty()) {
        return MarkdownType::EMPTY;
    }
    
    const char* p = line.c_str();
    
    // 跳过前导空格和 tab（用于计算缩进层级）
    int indent_count = 0;
    while (*p == ' ' || *p == '\t') {
        if (*p == '\t') {
            indent_count += 2;  // tab 算 2 个空格
            } else {
            indent_count++;
        }
        p++;
    }
    int indent_level = indent_count / 2;  // 每 2 个空格一级
    
    const char* trimmed = p;
    
    // 检查水平线 --- 或 ***
    if (std::strncmp(trimmed, "---", 3) == 0 || std::strncmp(trimmed, "***", 3) == 0) {
        bool all_same = true;
        char first_char = trimmed[0];
        for (int i = 1; trimmed[i] != '\0' && trimmed[i] != '\n'; i++) {
            if (trimmed[i] != first_char && trimmed[i] != ' ' && trimmed[i] != '\t') {
                all_same = false;
                            break;
                        }
                    }
        if (all_same && std::strlen(trimmed) >= 3) {
            return MarkdownType::HR;
        }
    }
    
    // 检查标题 (# 到 ######)
    if (trimmed[0] == '#') {
        int count = 0;
        while (trimmed[count] == '#') {
            count++;
        }
        if (count >= 1 && count <= 6 && (trimmed[count] == ' ' || trimmed[count] == '\t')) {
            if (level) *level = count;  // 标题的 level 是 # 的数量
            switch (count) {
                case 1: return MarkdownType::HEADING1;
                case 2: return MarkdownType::HEADING2;
                case 3: return MarkdownType::HEADING3;
                case 4: return MarkdownType::HEADING4;
                case 5: return MarkdownType::HEADING5;
                case 6: return MarkdownType::HEADING6;
            }
        }
    }
    
    // 检查引用 (>)
    if (trimmed[0] == '>') {
        return MarkdownType::QUOTE;
    }
    
    // 检查列表项 (-, *, +)
    if ((trimmed[0] == '-' || trimmed[0] == '*' || trimmed[0] == '+') && 
        (trimmed[1] == ' ' || trimmed[1] == '\t')) {
        if (level) *level = indent_level;  // 传递缩进层级
        return MarkdownType::LIST_ITEM;
    }
    
    // 检查有序列表 (1. 2. 等)
    if (std::isdigit(trimmed[0])) {
        int num = 0;
        int i = 0;
        while (std::isdigit(trimmed[i])) {
            num = num * 10 + (trimmed[i] - '0');
            i++;
        }
        if (trimmed[i] == '.' && (trimmed[i+1] == ' ' || trimmed[i+1] == '\t')) {
            if (level) *level = indent_level;  // 传递缩进层级
            return MarkdownType::LIST_ORDERED;
        }
    }
    
    // 检查代码块标记 (```)
    if (std::strncmp(trimmed, "```", 3) == 0) {
        return MarkdownType::CODE_BLOCK;
    }
    
    // 检查表格行
    if (IsTableRow(line)) {
        if (IsTableSeparator(line)) {
            return MarkdownType::TABLE_SEPARATOR;
        }
        return MarkdownType::TABLE_ROW;
    }
    
    return MarkdownType::NORMAL;
}

// 提取标题文本（去除 # 标记）
std::string MDTransformer::ExtractHeadingText(const std::string& line) {
    const char* trimmed = line.c_str();
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    int count = 0;
    while (trimmed[count] == '#') {
        count++;
    }
    
    while (trimmed[count] == ' ' || trimmed[count] == '\t') {
        count++;
    }
    
    std::string result = trimmed + count;
    
    // 去除尾部空格和换行
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\n')) {
        result.pop_back();
    }
    
    return result;
}

// 提取列表项文本
std::string MDTransformer::ExtractListText(const std::string& line) {
    const char* trimmed = line.c_str();
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    // 跳过列表标记
    if (trimmed[0] == '-' || trimmed[0] == '*' || trimmed[0] == '+') {
        trimmed += 2; // 跳过 "- " 或 "* " 或 "+ "
    } else if (std::isdigit(trimmed[0])) {
        while (std::isdigit(trimmed[0])) {
            trimmed++;
        }
        if (trimmed[0] == '.') {
            trimmed += 2; // 跳过 ". "
        }
    }
    
    std::string result = trimmed;
    
    // 去除尾部换行
    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

// 提取引用文本
std::string MDTransformer::ExtractQuoteText(const std::string& line) {
    const char* trimmed = line.c_str();
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    if (trimmed[0] == '>') {
        trimmed++;
        if (trimmed[0] == ' ') {
            trimmed++;
        }
    }
    
    std::string result = trimmed;
    
    // 去除尾部换行
    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

// ==================== 行内格式解析（移植自 flow_parser.c 的 parse_inline_formatting） ====================

ftxui::Element MDTransformer::ParseInlineFormatting(const std::string& input_text) {
    Elements elements;
    const char* p = input_text.c_str();
    int len = input_text.length();
    std::string buffer;
    
    int i = 0;
    while (i < len) {
        // 检查嵌套图片链接 [![alt](img-url)](link-url)（必须优先处理）
        if (p[i] == '[' && i + 1 < len && p[i + 1] == '!' && i + 2 < len && p[i + 2] == '[') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            // 解析嵌套图片链接
            i++; // 跳过第一个 [
            if (p[i] == '!' && i + 1 < len && p[i + 1] == '[') {
                i += 2; // 跳过 ![
                int alt_start = i;
                while (i < len && p[i] != ']') i++;
                if (i < len && p[i] == ']' && i + 1 < len && p[i + 1] == '(') {
                    std::string alt = input_text.substr(alt_start, i - alt_start);
                    i += 2; // 跳过 ](
                    // int img_url_start = i;  // 未使用
                    while (i < len && p[i] != ')') i++;
                    if (i < len && p[i] == ')') {
                        i++; // 跳过 )
                        // 检查外层链接
                        if (i < len && p[i] == ']' && i + 1 < len && p[i + 1] == '(') {
                            i += 2; // 跳过 ](
                            int link_start = i;
                            while (i < len && p[i] != ')') i++;
                            if (i < len && p[i] == ')') {
                                std::string link_url = input_text.substr(link_start, i - link_start);
                                i++; // 跳过 )
                                
                                // 格式化为 "🖼️ alt → link"
                                elements.push_back(ftxui::text("🖼️ " + alt) | color(Color::Magenta));
                                elements.push_back(ftxui::text(" → ") | color(Color::GrayDark));
                                elements.push_back(ftxui::text(link_url) | color(Color::BlueLight) | underlined);
                                continue;
                            }
                        }
                    }
                }
            }
        }
        
        // 检查普通图片链接 ![alt](url)
        if (p[i] == '!' && i + 1 < len && p[i + 1] == '[') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i += 2; // 跳过 ![
            int alt_start = i;
            while (i < len && p[i] != ']') i++;
            if (i < len && p[i] == ']' && i + 1 < len && p[i + 1] == '(') {
                std::string alt = input_text.substr(alt_start, i - alt_start);
                i += 2; // 跳过 ](
                // int url_start = i;  // 未使用
                while (i < len && p[i] != ')') i++;
                if (i < len && p[i] == ')') {
                    i++; // 跳过 )
                    // 格式化为 "🖼️ alt"
                    elements.push_back(ftxui::text("🖼️ " + alt) | color(Color::Magenta));
                    continue;
                }
            }
        }
        
        // 检查普通链接 [text](url)
        if (p[i] == '[') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i++; // 跳过 [
            int text_start = i;
            while (i < len && p[i] != ']') i++;
            if (i < len && p[i] == ']' && i + 1 < len && p[i + 1] == '(') {
                std::string link_text = input_text.substr(text_start, i - text_start);
                i += 2; // 跳过 ](
                // int url_start = i;  // 未使用
                while (i < len && p[i] != ')') i++;
                if (i < len && p[i] == ')') {
                    i++; // 跳过 )
                    elements.push_back(ftxui::text(link_text) | color(Color::BlueLight) | underlined);
                    continue;
                }
            }
        }
        
        // 检查行内代码 `code`
        if (p[i] == '`') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i++; // 跳过 `
            int code_start = i;
            while (i < len && p[i] != '`') i++;
            if (i < len) {
                std::string code_text = input_text.substr(code_start, i - code_start);
                i++; // 跳过 `
                elements.push_back(ftxui::text(code_text) | color(Color::Yellow) | bgcolor(Color::RGB(40, 40, 40)));
                continue;
            }
        }
        
        // 检查粗体+斜体 ***text***
        if (i + 2 < len && p[i] == '*' && p[i + 1] == '*' && p[i + 2] == '*') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i += 3; // 跳过 ***
            int start = i;
            while (i + 2 < len && !(p[i] == '*' && p[i + 1] == '*' && p[i + 2] == '*')) i++;
            if (i + 2 < len) {
                std::string bold_italic_text = input_text.substr(start, i - start);
                i += 3; // 跳过 ***
                elements.push_back(ftxui::text(bold_italic_text) | color(Color::White) | bold);
                continue;
            }
        }
        
        // 检查粗体 **text**
        if (i + 1 < len && p[i] == '*' && p[i + 1] == '*') {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i += 2; // 跳过 **
            int start = i;
            while (i + 1 < len && !(p[i] == '*' && p[i + 1] == '*')) i++;
            if (i + 1 < len) {
                std::string bold_text = input_text.substr(start, i - start);
                i += 2; // 跳过 **
                elements.push_back(ftxui::text(bold_text) | color(Color::White) | bold);
                continue;
            }
        }
        
        // 检查斜体 *text*（确保不是 ** 的一部分）
        if (p[i] == '*' && (i == 0 || p[i - 1] != '*') && (i + 1 >= len || p[i + 1] != '*')) {
            // 保存之前的文本
            if (!buffer.empty()) {
                elements.push_back(ftxui::text(buffer) | color(Color::White));
                buffer.clear();
            }
            
            i++; // 跳过 *
            int start = i;
            while (i < len && p[i] != '*') i++;
            if (i < len) {
                std::string italic_text = input_text.substr(start, i - start);
                i++; // 跳过 *
                elements.push_back(ftxui::text(italic_text) | color(Color::GrayLight) | dim);
                continue;
            }
        }
        
        // 普通字符
        buffer += p[i++];
    }
    
    // 添加剩余文本
    if (!buffer.empty()) {
        elements.push_back(ftxui::text(buffer) | color(Color::White));
    }
    
    if (elements.empty()) {
        return ftxui::text(input_text) | color(Color::White);
    }
    
    return hbox(elements);
}

// ==================== 表格处理（移植自 flow_formatter.c） ====================

// 检测表格行
bool MDTransformer::IsTableRow(const std::string& line) {
    if (line.empty()) return false;
    
    // 跳过前导空格
    const char* p = line.c_str();
    while (*p == ' ' || *p == '\t') p++;
    
    // 表格行必须包含 | 分隔符
    if (line.find('|') == std::string::npos) {
        return false;
    }
    
    // 检查是否包含多个 | 分隔符（至少 2 个，表示至少有 1 列）
    int pipe_count = 0;
    for (char c : line) {
        if (c == '|') {
            pipe_count++;
        }
    }
    
    return pipe_count >= 2;
}

// 检测表格分隔行
bool MDTransformer::IsTableSeparator(const std::string& line) {
    if (line.empty()) return false;
    
    // 检查是否只包含 |、-、:、空格
    const char* p = line.c_str();
    bool has_pipe = false;
    bool has_dash = false;
    
    while (*p != '\0' && *p != '\n') {
        if (*p == '|') {
            has_pipe = true;
        } else if (*p == '-' || *p == ':') {
            has_dash = true;
        } else if (*p != ' ' && *p != '\t') {
            return false;
        }
        p++;
    }
    
    return has_pipe && has_dash;
}

// 分割表格行（移植自 flow_formatter.c 的 parse_table_cells）
std::vector<std::string> MDTransformer::SplitTableRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string current_cell;
    
    const char* p = line.c_str();
    
    // 跳过前导空格
    while (*p == ' ' || *p == '\t') p++;
    
    // 检查行是否以 | 开头
    bool starts_with_pipe = (*p == '|');
    if (starts_with_pipe) {
        p++;
        while (*p == ' ' || *p == '\t') p++;
    }
    
    // 解析单元格
    while (*p != '\0' && *p != '\n') {
        if (*p == '|') {
            // 检查是否是最后一个 |
            const char* next = p + 1;
            while (*next == ' ' || *next == '\t') next++;
            bool is_last_pipe = (*next == '\0' || *next == '\n');
            
            // 保存当前单元格
            // 去除首尾空格
            std::string trimmed = current_cell;
            while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\t')) {
                trimmed.erase(0, 1);
            }
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
                trimmed.pop_back();
            }
            cells.push_back(trimmed);
            current_cell.clear();
            
            if (is_last_pipe) {
        break;
    }
    
            p++;
            while (*p == ' ' || *p == '\t') p++;
        } else {
            current_cell += *p++;
        }
    }
    
    return cells;
}

// 解析表格对齐方式
std::vector<TableAlignment> MDTransformer::ParseTableAlignments(const std::string& separator) {
    std::vector<TableAlignment> alignments;
    std::vector<std::string> cells = SplitTableRow(separator);
    
    for (const auto& cell : cells) {
        std::string trimmed = cell;
        while (!trimmed.empty() && (trimmed[0] == ' ' || trimmed[0] == '\t')) {
            trimmed.erase(0, 1);
        }
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
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

// 计算表格列宽（移植自 flow_formatter.c 的 calculate_single_table_widths）
std::vector<int> MDTransformer::CalculateTableColumnWidths(
    const std::vector<std::string>& header,
    const std::vector<std::vector<std::string>>& data) {
    
    std::vector<int> column_widths(header.size(), 0);
    
    // 计算表头宽度
    for (size_t i = 0; i < header.size(); ++i) {
        column_widths[i] = CalculateDisplayWidth(header[i]);
    }
    
    // 计算数据行宽度
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < header.size(); ++i) {
            int width = CalculateDisplayWidth(row[i]);
            column_widths[i] = std::max(column_widths[i], width);
        }
    }
    
    // 限制最大列宽
    const int MAX_COLUMN_WIDTH = 30;
    for (size_t i = 0; i < column_widths.size(); ++i) {
        if (column_widths[i] > MAX_COLUMN_WIDTH) {
            column_widths[i] = MAX_COLUMN_WIDTH;
        }
        // 确保最小宽度
        if (column_widths[i] < 3) {
            column_widths[i] = 3;
        }
    }
    
    return column_widths;
}

// 格式化表格单元格（带截断和填充）
std::string MDTransformer::FormatTableCell(const std::string& content, 
                                           int width, 
                                           TableAlignment align) {
    // 移除格式化标记
    std::string clean_content = content;
    
    std::regex bold_regex(R"(\*\*([^*]+)\*\*)");
    clean_content = std::regex_replace(clean_content, bold_regex, "$1");
    
    std::regex italic_regex(R"(\*([^*]+)\*)");
    clean_content = std::regex_replace(clean_content, italic_regex, "$1");
    
    std::regex code_regex(R"(`([^`]+)`)");
    clean_content = std::regex_replace(clean_content, code_regex, "$1");
    
    int display_width = CalculateDisplayWidth(clean_content);
    
    // 如果内容过长，截断
    if (display_width > width) {
        std::string truncated;
        int current_width = 0;
        const char* p = clean_content.c_str();
        
        while (*p && current_width < width - 2) {
            uint32_t codepoint;
            int bytes = DecodeUtf8(p, &codepoint);
            
            int char_width = 1;
            if (IsWideEmoji(codepoint) || IsEastAsianWide(codepoint)) {
                char_width = 2;
            }
            
            if (current_width + char_width <= width - 2) {
                for (int i = 0; i < bytes; ++i) {
                    truncated += p[i];
                }
                current_width += char_width;
            } else {
                break;
            }
            
            p += bytes;
        }
        
        return truncated + "..";
    }
    
    // 根据对齐方式填充空格
    int padding = width - display_width;
    if (padding < 0) padding = 0;
    
    std::string result;
    switch (align) {
        case TableAlignment::CENTER: {
            int left_padding = padding / 2;
            int right_padding = padding - left_padding;
            result = std::string(left_padding, ' ') + clean_content + 
                     std::string(right_padding, ' ');
            break;
        }
        case TableAlignment::RIGHT:
            result = std::string(padding, ' ') + clean_content;
            break;
        case TableAlignment::LEFT:
        default:
            result = clean_content + std::string(padding, ' ');
            break;
    }
    
    return result;
}

// 渲染表格
ftxui::Element MDTransformer::RenderTable(const std::vector<std::string>& table_lines) {
    if (table_lines.empty()) {
        return text("");
    }
    
    // 分离表头和分隔符行
    std::vector<std::string> header_row;
    std::vector<std::vector<std::string>> data_rows;
    std::vector<TableAlignment> alignments;
    
    bool found_separator = false;
    
    for (const auto& line : table_lines) {
        if (IsTableSeparator(line)) {
            alignments = ParseTableAlignments(line);
            found_separator = true;
            continue;
        }
        
        if (!found_separator) {
            header_row = SplitTableRow(line);
        } else {
            data_rows.push_back(SplitTableRow(line));
        }
    }
    
    if (header_row.empty()) {
        return ftxui::text("Empty table");
    }
    
    // 动态计算列宽
    std::vector<int> column_widths = CalculateTableColumnWidths(header_row, data_rows);
    
    // 确保对齐方式数量与列数匹配
    while (alignments.size() < header_row.size()) {
        alignments.push_back(TableAlignment::LEFT);
    }
    
    // 渲染表格
    Elements table_elements;
    
    // 顶部边框
    std::string top_border = "┌";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (int j = 0; j < column_widths[i] + 2; ++j) {
            top_border += "─";
        }
        if (i < header_row.size() - 1) {
            top_border += "┬";
        }
    }
    top_border += "┐";
    table_elements.push_back(ftxui::text(top_border) | color(Color::GrayLight));
    
    // 表头行
    std::string header_display = "│";
    for (size_t i = 0; i < header_row.size(); ++i) {
        std::string formatted_cell = FormatTableCell(
            header_row[i], 
            column_widths[i], 
            alignments[i]
        );
        header_display += " " + formatted_cell + " │";
    }
    table_elements.push_back(ftxui::text(header_display) | color(Color::White) | bold);
    
    // 分隔线
    std::string separator = "├";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (int j = 0; j < column_widths[i] + 2; ++j) {
            separator += "─";
        }
        if (i < header_row.size() - 1) {
            separator += "┼";
        }
    }
    separator += "┤";
    table_elements.push_back(ftxui::text(separator) | color(Color::GrayLight));
    
    // 数据行
    for (const auto& row : data_rows) {
        std::string row_display = "│";
        for (size_t i = 0; i < header_row.size(); ++i) {
            std::string cell_content = (i < row.size()) ? row[i] : "";
            std::string formatted_cell = FormatTableCell(
                cell_content, 
                column_widths[i],
                alignments[i]
            );
            row_display += " " + formatted_cell + " │";
        }
        table_elements.push_back(ftxui::text(row_display) | color(Color::White));
    }
    
    // 底部边框
    std::string bottom_border = "└";
    for (size_t i = 0; i < header_row.size(); ++i) {
        for (int j = 0; j < column_widths[i] + 2; ++j) {
            bottom_border += "─";
        }
        if (i < header_row.size() - 1) {
            bottom_border += "┴";
        }
    }
    bottom_border += "┘";
    table_elements.push_back(ftxui::text(bottom_border) | color(Color::GrayLight));
    
    return vbox(table_elements);
}

// ==================== 渲染方法（移植自 flow_renderer.c） ====================

// 渲染标题
ftxui::Element MDTransformer::RenderHeading(const std::string& heading_text, int level) {
    Color header_color;
    
    switch (level) {
        case 1:
            header_color = Color::RGB(129, 161, 193);  // 青色
            return vbox({
                ftxui::text(""),
                ftxui::text(heading_text) | color(header_color) | bold,
                ftxui::text("")
            });
        case 2:
            header_color = Color::RGB(129, 161, 193);
            return vbox({
                ftxui::text(""),
                ftxui::text(heading_text) | color(header_color) | bold,
                ftxui::text("")
            });
        case 3:
            header_color = Color::RGB(111, 161, 193);
            return vbox({
                ftxui::text(heading_text) | color(header_color) | bold,
                ftxui::text("")
            });
        case 4:
            header_color = Color::RGB(111, 161, 193);
            return ftxui::text(heading_text) | color(header_color) | bold;
        case 5:
            header_color = Color::RGB(177, 148, 193);
            return ftxui::text(heading_text) | color(header_color) | bold;
        default:
            header_color = Color::RGB(177, 148, 193);
            return ftxui::text(heading_text) | color(header_color);
    }
}

// 渲染代码块
ftxui::Element MDTransformer::RenderCodeBlock(const std::vector<std::string>& lines, 
                                              const std::string& /* language */) {
    if (lines.empty()) {
        return ftxui::text("");
    }
    
    Elements code_elements;
    const size_t MAX_WIDTH = 100;
    
    // 添加顶部空行
    code_elements.push_back(ftxui::text(""));
    
    for (const auto& line : lines) {
        std::string code_line = line;
        
        // 智能截断
        if (code_line.length() > MAX_WIDTH) {
            size_t cut_pos = MAX_WIDTH - 3;
            while (cut_pos > 0 && code_line[cut_pos] != ' ' && cut_pos > MAX_WIDTH - 10) {
                cut_pos--;
            }
            if (cut_pos == 0 || cut_pos <= MAX_WIDTH - 10) {
                cut_pos = MAX_WIDTH - 3;
            }
            code_line = code_line.substr(0, cut_pos) + "...";
        }
        
        // 添加左侧边框
        Elements line_elements;
        line_elements.push_back(ftxui::text("│ ") | color(Color::RGB(100, 100, 100)));
        line_elements.push_back(ftxui::text(code_line) | color(Color::RGB(200, 200, 200)));
        
        code_elements.push_back(
            hbox(line_elements) | 
            bgcolor(Color::RGB(35, 35, 35))
        );
    }
    
    // 添加底部空行
    code_elements.push_back(ftxui::text(""));
    
    return vbox(code_elements) | 
           borderLight | 
           color(Color::RGB(180, 180, 180));
}

// 渲染列表项
ftxui::Element MDTransformer::RenderListItem(const std::string& item_text, int level, bool /* ordered */) {
    // 使用不同的符号表示不同层级
    std::string bullet = "●";
    Color bullet_color = Color::RGB(100, 180, 255);
    
    if (level == 1) {
        bullet = "○";
        bullet_color = Color::RGB(150, 200, 255);
    } else if (level >= 2) {
        bullet = "▸";
        bullet_color = Color::RGB(180, 210, 255);
    }
    
    Elements list_elements;
    
    // 添加缩进
    if (level > 0) {
        list_elements.push_back(ftxui::text(std::string(level * 2, ' ')));
    }
    
    list_elements.push_back(ftxui::text(bullet + " ") | color(bullet_color));
    list_elements.push_back(ParseInlineFormatting(item_text));
    
    return hbox(list_elements);
}

// 渲染引用块
ftxui::Element MDTransformer::RenderQuote(const std::string& quote_text) {
    Elements quote_elements;
    quote_elements.push_back(ftxui::text("▌") | color(Color::RGB(100, 150, 200)));
    quote_elements.push_back(ftxui::text(" "));
    quote_elements.push_back(ParseInlineFormatting(quote_text) | color(Color::RGB(160, 160, 180)));
    
    return hbox(quote_elements) | bgcolor(Color::RGB(40, 40, 45));
}

// 渲染水平线
ftxui::Element MDTransformer::RenderHorizontalRule() {
    return vbox({
        ftxui::text(""),
        separator() | color(Color::GrayLight),
        ftxui::text("")
    });
}

// 渲染单行
ftxui::Element MDTransformer::RenderLine(const std::string& line, MarkdownType type, int level) {
    switch (type) {
        case MarkdownType::HEADING1:
        case MarkdownType::HEADING2:
        case MarkdownType::HEADING3:
        case MarkdownType::HEADING4:
        case MarkdownType::HEADING5:
        case MarkdownType::HEADING6: {
            std::string heading_text = ExtractHeadingText(line);
            return RenderHeading(heading_text, level);
        }
        
        case MarkdownType::QUOTE: {
            std::string quote_text = ExtractQuoteText(line);
            return RenderQuote(quote_text);
        }
        
        case MarkdownType::LIST_ITEM: {
            std::string list_text = ExtractListText(line);
            return RenderListItem(list_text, level, false);
        }
        
        case MarkdownType::LIST_ORDERED: {
            std::string list_text = ExtractListText(line);
            return RenderListItem(list_text, level, true);
        }
        
        case MarkdownType::HR:
            return RenderHorizontalRule();
        
        case MarkdownType::EMPTY:
            return ftxui::text("");
        
        case MarkdownType::NORMAL:
        default:
            return ParseInlineFormatting(line);
    }
}

// ==================== 主要转换方法 ====================

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
    if (markdown_lines.empty()) {
        return ftxui::text("Empty content");
    }
    
    Elements elements;
    bool in_code_block = false;
    bool in_table = false;
    std::vector<std::string> code_block_lines;
    std::string code_language;
    std::vector<std::string> table_lines;
    
    for (size_t i = 0; i < markdown_lines.size(); ++i) {
        const std::string& line = markdown_lines[i];
        int level = 0;
        MarkdownType type = ParseLineType(line, &level);
        
        // 处理代码块
        if (type == MarkdownType::CODE_BLOCK) {
            if (!in_code_block) {
                // 开始代码块
                in_code_block = true;
                code_language = (line.length() > 3) ? line.substr(3) : "";
                code_block_lines.clear();
            } else {
                // 结束代码块
                in_code_block = false;
                if (!code_block_lines.empty()) {
                    elements.push_back(RenderCodeBlock(code_block_lines, code_language));
                }
                code_block_lines.clear();
                code_language.clear();
            }
            continue;
        }
        
        if (in_code_block) {
            code_block_lines.push_back(line);
            continue;
        }
        
        // 处理表格
        if (type == MarkdownType::TABLE_ROW || type == MarkdownType::TABLE_SEPARATOR) {
            if (!in_table) {
                in_table = true;
                table_lines.clear();
            }
            table_lines.push_back(line);
            continue;
        } else if (in_table) {
            // 表格结束
            in_table = false;
            if (!table_lines.empty()) {
                elements.push_back(RenderTable(table_lines));
            }
            table_lines.clear();
        }
        
        // 渲染普通行
        elements.push_back(RenderLine(line, type, level));
    }
    
    // 处理未关闭的代码块
    if (in_code_block && !code_block_lines.empty()) {
        elements.push_back(RenderCodeBlock(code_block_lines, code_language));
    }
    
    // 处理未关闭的表格
    if (in_table && !table_lines.empty()) {
        elements.push_back(RenderTable(table_lines));
    }
    
    // 应用滚动偏移
    if (scroll_offset_ > 0 && scroll_offset_ < static_cast<int>(elements.size())) {
        elements.erase(elements.begin(), elements.begin() + scroll_offset_);
    }
    
    return vbox(elements);
}

// ==================== 滚动控制 ====================

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

// ==================== 辅助方法 ====================

SyntaxType MDTransformer::DetectSyntaxType(const std::string& language) {
    std::string lang_lower = language;
    std::transform(lang_lower.begin(), lang_lower.end(), lang_lower.begin(), ::tolower);
    
    if (lang_lower == "c") return SyntaxType::C;
    if (lang_lower == "cpp" || lang_lower == "c++") return SyntaxType::CPP;
    if (lang_lower == "py" || lang_lower == "python") return SyntaxType::PYTHON;
    if (lang_lower == "js" || lang_lower == "javascript") return SyntaxType::JAVASCRIPT;
    if (lang_lower == "ts" || lang_lower == "typescript") return SyntaxType::TYPESCRIPT;
    if (lang_lower == "sh" || lang_lower == "bash") return SyntaxType::BASH;
    if (lang_lower == "go") return SyntaxType::GO;
    if (lang_lower == "rust" || lang_lower == "rs") return SyntaxType::RUST;
    if (lang_lower == "java") return SyntaxType::JAVA;
    if (lang_lower == "json") return SyntaxType::JSON;
    if (lang_lower == "xml" || lang_lower == "html") return SyntaxType::XML;
    if (lang_lower == "sql") return SyntaxType::SQL;
    if (lang_lower == "yaml" || lang_lower == "yml") return SyntaxType::YAML;
    if (lang_lower == "md" || lang_lower == "markdown") return SyntaxType::MARKDOWN;
    
    return SyntaxType::NONE;
}

std::string MDTransformer::StripHtmlTags(const std::string& text) {
    std::string result;
    bool in_tag = false;
    
    for (char c : text) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            result += c;
        }
    }
    
    return result;
}

} // namespace Editor
} // namespace FTB
