#include "flow.h"
#include "flow_emoji.h"
#include "flow_html.h"
#include "../include/common.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

// 前向声明 parse_inline_formatting
extern void parse_inline_formatting(const char *text, char *output, int max_len, StyleType style);

// 移除 HTML 标签
void strip_html_tags(const char *input, char *output, int max_len) {
    if (input == NULL || output == NULL || max_len <= 0) {
        return;
    }
    
    int len = strlen(input);
    int out_pos = 0;
    bool in_tag = false;
    
    for (int i = 0; i < len && out_pos < max_len - 1; i++) {
        if (input[i] == '<') {
            in_tag = true;
            continue;
        }
        if (input[i] == '>') {
            in_tag = false;
            continue;
        }
        if (!in_tag) {
            output[out_pos++] = input[i];
        }
    }
    output[out_pos] = '\0';
}

// 处理图片链接 ![alt](url) -> 完全隐藏（类似 glow）
// 也处理嵌套格式 [![alt](url)](link) -> 完全隐藏
void format_image_link(const char *text, char *output, int max_len) {
    if (text == NULL || output == NULL || max_len <= 0) {
        if (output && max_len > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    int out_pos = 0;
    const char *p = text;
    
    while (*p && out_pos < max_len - 1) {
        // 检查图片链接 ![alt](url) 或嵌套格式 [![alt](url)](link)
        if (p[0] == '!' && p[1] == '[') {
            const char *alt_start = p + 2;
            const char *alt_end = strchr(alt_start, ']');
            
            if (alt_end != NULL && alt_end[1] == '(') {
                const char *url_start = alt_end + 2;
                const char *url_end = strchr(url_start, ')');
                
                if (url_end != NULL) {
                    // 检查是否有嵌套链接 [![alt](url)](link)
                    const char *next = url_end + 1;
                    if (next[0] == ']' && next[1] == '(') {
                        // 嵌套格式，跳过外层链接
                        const char *outer_url_start = next + 2;
                        const char *outer_url_end = strchr(outer_url_start, ')');
                        if (outer_url_end != NULL) {
                            next = outer_url_end + 1;
                        }
                    }
                    
                    // 完全跳过图片链接，不输出任何内容（类似 glow）
                    p = next;
                    continue;
                }
            }
        }
        
        // 普通字符
        output[out_pos++] = *p++;
    }
    output[out_pos] = '\0';
}

// 检测表格行
bool is_table_row(const char *line) {
    if (line == NULL) return false;
    
    // 跳过前导空格
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    
    // 表格行必须以 | 开头（标准 markdown 表格格式）
    if (*p != '|') {
        return false;
    }
    
    // 检查是否包含多个 | 分隔符（至少 2 个，表示至少有 1 列）
    int pipe_count = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            pipe_count++;
        }
    }
    
    // 表格行至少需要 2 个 |（| 列1 | 列2 |）
    // 但也要排除链接中的 |（如 [text](url|alt)）
    // 简单检查：如果包含 ]( 或 ![，可能是链接，需要更严格的检查
    if (strstr(line, "](") != NULL || strstr(line, "![") != NULL) {
        // 可能是链接，检查是否是真正的表格行
        // 真正的表格行：| 列1 | 列2 | 列3 |
        // 链接格式：[text](url) 或 ![alt](url)
        // 如果 | 不在链接中，且行以 | 开头，则是表格行
        // 简化：如果包含 ]( 且 | 的数量较少，可能是链接
        if (pipe_count < 3) {
            return false;  // 链接通常只有 1-2 个 |
        }
    }
    
    return pipe_count >= 2;
}

// 检测表格分隔行
bool is_table_separator(const char *line) {
    if (line == NULL) return false;
    
    // 检查是否只包含 |、-、:、空格
    const char *p = line;
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

// 判断是否为East Asian Wide/Fullwidth字符（基于Unicode East_Asian_Width属性）
// emoji判断逻辑已移至 flow_emoji.c

static bool is_east_asian_wide(unsigned int codepoint) {
    // CJK统一表意文字（常用汉字）
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
    
    // 其他East Asian宽字符
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

// UTF-8解码，返回Unicode码点和字节数
static int decode_utf8(const char *s, unsigned int *codepoint) {
    unsigned char c = (unsigned char)*s;
    
    if (c < 0x80) {
        // 单字节字符（ASCII）
        *codepoint = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2字节字符
        *codepoint = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        // 3字节字符
        *codepoint = ((c & 0x0F) << 12) | 
                     (((unsigned char)s[1] & 0x3F) << 6) | 
                     ((unsigned char)s[2] & 0x3F);
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        // 4字节字符
        *codepoint = ((c & 0x07) << 18) | 
                     (((unsigned char)s[1] & 0x3F) << 12) | 
                     (((unsigned char)s[2] & 0x3F) << 6) | 
                     ((unsigned char)s[3] & 0x3F);
        return 4;
    }
    
    // 无效的UTF-8序列
    *codepoint = 0xFFFD;  // 替换字符
    return 1;
}

// 计算显示宽度（精确处理East Asian字符，ANSI代码不计入宽度）
static int calculate_display_width(const char *text) {
    if (text == NULL) return 0;
    
    int width = 0;
    const char *p = text;
    
    while (*p) {
        // 处理 ANSI 转义序列：\033[ ... m
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
        unsigned int codepoint;
        int bytes = decode_utf8(p, &codepoint);
        
        if (bytes == 0 || bytes > 4) {
            // 无效的 UTF-8 序列，跳过
            p++;
            continue;
        }
        
        // 首先检查是否为宽字符emoji（在终端中占2个显示宽度）
        if (is_wide_emoji(codepoint)) {
            width += 2;
        } else if (is_east_asian_wide(codepoint)) {
            // East Asian Wide字符占2个显示宽度（中文字符等）
            width += 2;
        } else if (codepoint >= 0x20 && codepoint < 0x7F) {
            // 可打印ASCII字符占1个宽度
            width += 1;
        } else if (codepoint == 0x09) {
            // Tab字符占8个宽度（可调整）
            width += 8;
        } else if (codepoint == 0x200D) {
            // Zero Width Joiner (ZWJ) - 用于组合emoji，不计入宽度
            width += 0;
        } else if (codepoint >= 0xFE00 && codepoint <= 0xFE0F) {
            // Variation Selectors - 用于emoji变体，不计入宽度
            width += 0;
        } else if (codepoint >= 0x80) {
            // 其他Unicode字符默认占1个宽度
            width += 1;
        }
        // 控制字符（< 0x20，除了Tab）不计入宽度
        
        p += bytes;
    }
    
    return width;
}

// 统一的单元格预处理管线（按正确顺序处理）
// 处理顺序：render_html_content -> format_image_link -> parse_inline_formatting
static void preprocess_cell(const char *input, char *output, int max_len, StyleType style) {
    if (input == NULL || output == NULL || max_len <= 0) {
        if (output && max_len > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    char buf1[1024];
    char buf2[1024];
    
    // 第一步：解析 HTML 标签（转换为带颜色的文本）
    // 如果包含 HTML 标签，使用新的 HTML 解析器；否则直接使用原文本
    if (strchr(input, '<') != NULL && strchr(input, '>') != NULL) {
        render_html_content(input, buf1, sizeof(buf1), style);
    } else {
        strncpy(buf1, input, sizeof(buf1) - 1);
        buf1[sizeof(buf1) - 1] = '\0';
    }
    
    // 第二步：处理图片链接（移除图片标记）
    format_image_link(buf1, buf2, sizeof(buf2));
    
    // 第三步：解析内联格式（粗体、斜体、代码等）
    parse_inline_formatting(buf2, output, max_len, style);
}

// 解析表格行，提取单元格内容
static void parse_table_cells(const char *line, char cells[][512], int *cell_count, int max_cells) {
    if (line == NULL || cells == NULL || cell_count == NULL) {
        if (cell_count) *cell_count = 0;
        return;
    }
    
    *cell_count = 0;
    const char *p = line;
    
    // 跳过前导空格
    while (*p == ' ' || *p == '\t') p++;
    
    // 检查行是否以 | 开头和结尾（标准 markdown 表格格式：| 列1 | 列2 | 列3 |）
    bool starts_with_pipe = (*p == '|');
    const char *line_end = p + strlen(p);
    while (line_end > p && (line_end[-1] == ' ' || line_end[-1] == '\t' || line_end[-1] == '\n' || line_end[-1] == '\r')) {
        line_end--;
    }
    bool ends_with_pipe = (line_end > p && line_end[-1] == '|');
    
    // 如果行以 | 开头，跳过它（但不要计入列数）
    if (starts_with_pipe) {
        p++;
        while (*p == ' ' || *p == '\t') p++;
    }
    
    // 解析单元格
    char *cell = cells[*cell_count];
    int cell_pos = 0;
    
    while (*p != '\0' && *p != '\n' && *cell_count < max_cells) {
        if (*p == '|') {
            // 检查是否是最后一个 |（行以 | 结尾，且后面只有空白字符）
            const char *next = p + 1;
            while (*next == ' ' || *next == '\t') next++;
            bool is_last_pipe = (ends_with_pipe && (*next == '\0' || *next == '\n' || *next == '\r'));
            
            // 保存当前单元格
            cell[cell_pos] = '\0';
            // 去除首尾空格
            char *start = cell;
            while (*start == ' ' || *start == '\t') start++;
            int len = strlen(start);
            while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\t')) {
                start[--len] = '\0';
            }
            // 即使单元格为空，也计入列数（因为 | | 表示空单元格）
            if (len > 0) {
                memmove(cell, start, len + 1);
            } else {
                cell[0] = '\0';
            }
            (*cell_count)++;
            
            // 如果是最后一个 |，停止解析
            if (is_last_pipe) {
                break;
            }
            
            if (*cell_count < max_cells) {
                cell = cells[*cell_count];
            }
            cell_pos = 0;
            p++;
            // 跳过 | 后的空格
            while (*p == ' ' || *p == '\t') p++;
        } else {
            if (cell_pos < 511) {
                cell[cell_pos++] = *p;
            }
            p++;
        }
    }
    
    // 处理最后一个单元格（如果行没有以 | 结尾，但还有内容）
    if (cell_pos > 0 && *cell_count < max_cells && !ends_with_pipe) {
        cell[cell_pos] = '\0';
        char *start = cell;
        while (*start == ' ' || *start == '\t') start++;
        int len = strlen(start);
        while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\t')) {
            start[--len] = '\0';
        }
        if (len > 0) {
            memmove(cell, start, len + 1);
            (*cell_count)++;
        }
    }
}

// 计算单个表格的列宽（从表格开始到结束）
void calculate_single_table_widths(const char *table_start, const char *table_end, StyleType style, int *col_widths, int *num_cols, int max_cols) {
    if (table_start == NULL || col_widths == NULL || num_cols == NULL || max_cols <= 0) {
        if (num_cols) *num_cols = 0;
        return;
    }
    
    // 初始化列宽数组
    for (int i = 0; i < max_cols; i++) {
        col_widths[i] = 0;
    }
    *num_cols = 0;
    
    // 计算表格内容长度
    size_t table_len = (table_end != NULL && table_end > table_start) ? 
                       (size_t)(table_end - table_start) : strlen(table_start);
    
    // 复制表格内容
    char *table_content = (char*)malloc(table_len + 1);
    if (table_content == NULL) {
        return;
    }
    strncpy(table_content, table_start, table_len);
    table_content[table_len] = '\0';
    
    // 手动按行分割，避免strtok状态冲突
    char *p = table_content;
    char line_buffer[4096];
    
    while (*p != '\0') {
        // 提取一行
        size_t line_pos = 0;
        while (*p != '\0' && *p != '\n' && line_pos < sizeof(line_buffer) - 1) {
            line_buffer[line_pos++] = *p++;
        }
        line_buffer[line_pos] = '\0';
        
        // 跳过换行符
        if (*p == '\n') p++;
        
        // 只处理表格行（不包括分隔行）
        if (is_table_row(line_buffer) && !is_table_separator(line_buffer)) {
            char cells[64][512];
            int cell_count = 0;
            
            parse_table_cells(line_buffer, cells, &cell_count, 64);
            
            // 更新列数（取所有行中的最大列数）
            if (cell_count > *num_cols) {
                *num_cols = cell_count;
            }
            if (*num_cols > max_cols) {
                *num_cols = max_cols;
            }
            
            // 计算每列的显示宽度（只计算实际存在的列）
            for (int i = 0; i < cell_count && i < max_cols; i++) {
                // 使用统一的预处理管线处理单元格内容
                char rendered_cell[1024];
                preprocess_cell(cells[i], rendered_cell, sizeof(rendered_cell), style);
                
                // 计算显示宽度（ANSI代码已被正确跳过）
                int display_width = calculate_display_width(rendered_cell);
                
                // 更新最大宽度
                if (display_width > col_widths[i]) {
                    col_widths[i] = display_width;
                }
            }
        }
    }
    
    // 确保所有列都有合理的宽度（即使某些行没有该列）
    for (int i = 0; i < *num_cols && i < max_cols; i++) {
        if (col_widths[i] < 1) {
            col_widths[i] = 1;  // 至少1个字符宽度
        }
    }
    
    free(table_content);
}

// 计算表格列宽（扫描所有表格行，计算每列最大显示宽度）
// 注意：此函数已废弃，保留用于向后兼容，实际应该使用 calculate_single_table_widths
void calculate_table_column_widths(const char *content, StyleType style, int *col_widths, int *num_cols, int max_cols) {
    calculate_single_table_widths(content, NULL, style, col_widths, num_cols, max_cols);
}

// 格式化表格行，使用动态计算的列宽以确保对齐
void format_table_row(const char *line, char *output, int max_len, StyleType style, int terminal_width, const int *col_widths, int num_cols) {
    (void)terminal_width;  // 暂时未使用
    if (line == NULL || output == NULL || max_len <= 0) {
        if (output && max_len > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    const char *table_color = (style == STYLE_LIGHT) ? COLOR_BLACK : COLOR_WHITE;
    const char *separator_color = (style == STYLE_LIGHT) ? 
        "\033[38;5;244m" : "\033[38;5;240m";  // 灰色分隔符
    
    // 如果没有提供列宽，使用默认值
    int default_col_widths[] = {16, 15, 29, 15};
    int default_num_cols = 4;
    
    if (col_widths == NULL || num_cols <= 0) {
        col_widths = default_col_widths;
        num_cols = default_num_cols;
    }
    
    // 分割单元格
    char cells[64][512];
    int cell_count = 0;
    parse_table_cells(line, cells, &cell_count, 64);
    
    if (cell_count == 0) {
        output[0] = '\0';
        return;
    }
    
    // 格式化输出
    int out_pos = 0;
    // 使用实际单元格数和计算出的列数中的较小值
    int max_output_cols = (cell_count < num_cols) ? cell_count : num_cols;
    
    for (int i = 0; i < max_output_cols && out_pos < max_len - 100; i++) {
        if (i > 0) {
            out_pos += snprintf(output + out_pos, max_len - out_pos, 
                               " %s│%s ", separator_color, COLOR_RESET);
        }
        
        // 使用统一的预处理管线处理单元格内容
        char rendered_cell[1024];
        preprocess_cell(cells[i], rendered_cell, sizeof(rendered_cell), style);
        
        // 计算显示宽度（ANSI代码已被正确跳过）
        // 注意：必须使用与列宽计算时完全相同的内容来计算宽度
        int display_width = calculate_display_width(rendered_cell);
        
        // 输出单元格内容（添加table_color，但这不影响宽度计算）
        out_pos += snprintf(output + out_pos, max_len - out_pos, "%s%s%s", 
                           table_color, rendered_cell, COLOR_RESET);
        
        // 添加填充空格（修正的padding计算）
        // padding 只看 display_width，ANSI永远不参与宽度计算
        // 所有列都需要填充，包括最后一列（为了对齐）
        if (i < num_cols && i < 64 && col_widths != NULL) {
            int padding = col_widths[i] - display_width;
            if (padding < 0) padding = 0;
            
            // 确保填充足够的空格以达到列宽
            for (int j = 0; j < padding && out_pos < max_len - 1; j++) {
                output[out_pos++] = ' ';
            }
            output[out_pos] = '\0';
        }
    }
    output[out_pos] = '\0';
}

// 居中文本
void center_text(const char *text, int width, char *output, int max_len) {
    if (text == NULL || output == NULL || max_len <= 0) {
        return;
    }
    
    // 使用新的函数计算文本宽度（考虑中文字符）
    int text_width = calculate_display_width(text);
    
    int padding = (width - text_width) / 2;
    if (padding < 0) padding = 0;
    
    int out_pos = 0;
    for (int i = 0; i < padding && out_pos < max_len - 1; i++) {
        output[out_pos++] = ' ';
    }
    
    strncpy(output + out_pos, text, max_len - out_pos - 1);
    out_pos += strlen(text);
    output[out_pos] = '\0';
}

// 处理代码块语言标识
void format_code_block_marker(const char *line, char *output, int max_len) {
    if (line == NULL || output == NULL || max_len <= 0) {
        return;
    }
    
    // 提取语言标识
    if (strncmp(line, "```", 3) == 0) {
        const char *lang = line + 3;
        while (*lang == ' ' || *lang == '\t') lang++;
        
        if (*lang != '\0' && *lang != '\n') {
            char lang_name[64];
            int i = 0;
            while (*lang != '\0' && *lang != '\n' && i < 63) {
                lang_name[i++] = *lang++;
            }
            lang_name[i] = '\0';
            
            snprintf(output, max_len, "%s```%s%s", COLOR_GREEN, lang_name, COLOR_RESET);
        } else {
            snprintf(output, max_len, "%s```%s", COLOR_GREEN, COLOR_RESET);
        }
    } else {
        strncpy(output, line, max_len - 1);
        output[max_len - 1] = '\0';
    }
}

