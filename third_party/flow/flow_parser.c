#include "flow.h"
#include "../include/common.h"
#include <ctype.h>
#include <string.h>

// 解析 Markdown 行
MarkdownType parse_markdown_line(const char *line, int *level) {
    if (line == NULL || strlen(line) == 0) {
        return MD_EMPTY;
    }
    
    // 计算缩进层级（每2个空格或1个tab算一级）
    const char *p = line;
    int indent_count = 0;
    while (*p == ' ' || *p == '\t') {
        if (*p == '\t') {
            indent_count += 2;  // tab 算2个空格
        } else {
            indent_count++;
        }
        p++;
    }
    int indent_level = indent_count / 2;  // 每2个空格一级
    
    const char *trimmed = p;
    
    // 检查水平线
    if (strncmp(trimmed, "---", 3) == 0 || strncmp(trimmed, "***", 3) == 0) {
        bool all_same = true;
        char first_char = trimmed[0];
        for (int i = 1; trimmed[i] != '\0' && trimmed[i] != '\n'; i++) {
            if (trimmed[i] != first_char && trimmed[i] != ' ' && trimmed[i] != '\t') {
                all_same = false;
                break;
            }
        }
        if (all_same && strlen(trimmed) >= 3) {
            return MD_HR;
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
                case 1: return MD_HEADING1;
                case 2: return MD_HEADING2;
                case 3: return MD_HEADING3;
                case 4: return MD_HEADING4;
                case 5: return MD_HEADING5;
                case 6: return MD_HEADING6;
            }
        }
    }
    
    // 检查引用 (>)
    if (trimmed[0] == '>') {
        return MD_QUOTE;
    }
    
    // 检查列表项 (-, *, +)
    if ((trimmed[0] == '-' || trimmed[0] == '*' || trimmed[0] == '+') && 
        (trimmed[1] == ' ' || trimmed[1] == '\t')) {
        if (level) *level = indent_level;  // 传递缩进层级
        return MD_LIST_ITEM;
    }
    
    // 检查有序列表 (1. 2. 等)
    if (isdigit(trimmed[0])) {
        int num = 0;
        int i = 0;
        while (isdigit(trimmed[i])) {
            num = num * 10 + (trimmed[i] - '0');
            i++;
        }
        if (trimmed[i] == '.' && (trimmed[i+1] == ' ' || trimmed[i+1] == '\t')) {
            if (level) *level = indent_level;  // 传递缩进层级
            return MD_LIST_ORDERED;
        }
    }
    
    // 检查代码块标记 (```)
    if (strncmp(trimmed, "```", 3) == 0) {
        return MD_CODE_BLOCK;
    }
    
    return MD_NORMAL;
}

// 提取标题文本（去除 # 标记）
void extract_heading_text(const char *line, char *output, int max_len) {
    const char *trimmed = line;
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
    
    strncpy(output, trimmed + count, max_len - 1);
    output[max_len - 1] = '\0';
    
    // 去除尾部空格
    int len = strlen(output);
    while (len > 0 && (output[len-1] == ' ' || output[len-1] == '\t' || output[len-1] == '\n')) {
        output[--len] = '\0';
    }
}

// 提取列表项文本
void extract_list_text(const char *line, char *output, int max_len) {
    const char *trimmed = line;
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    // 跳过列表标记
    if (trimmed[0] == '-' || trimmed[0] == '*' || trimmed[0] == '+') {
        trimmed += 2; // 跳过 "- " 或 "* " 或 "+ "
    } else if (isdigit(trimmed[0])) {
        while (isdigit(trimmed[0])) {
            trimmed++;
        }
        if (trimmed[0] == '.') {
            trimmed += 2; // 跳过 ". "
        }
    }
    
    strncpy(output, trimmed, max_len - 1);
    output[max_len - 1] = '\0';
    
    // 去除尾部换行
    int len = strlen(output);
    if (len > 0 && output[len-1] == '\n') {
        output[--len] = '\0';
    }
}

// 提取引用文本
void extract_quote_text(const char *line, char *output, int max_len) {
    const char *trimmed = line;
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    
    if (trimmed[0] == '>') {
        trimmed++;
        if (trimmed[0] == ' ') {
            trimmed++;
        }
    }
    
    strncpy(output, trimmed, max_len - 1);
    output[max_len - 1] = '\0';
    
    // 去除尾部换行
    int len = strlen(output);
    if (len > 0 && output[len-1] == '\n') {
        output[--len] = '\0';
    }
}

// 行内格式标记结构
typedef struct {
    int start;
    int end;
    MarkdownType type;
} InlineFormat;

// 解析行内格式（粗体、斜体、代码、链接）
// 返回格式化的字符串，使用 ANSI 颜色代码
void parse_inline_formatting(const char *text, char *output, int max_len, StyleType style) {
    if (text == NULL || output == NULL || max_len <= 0) {
        return;
    }
    
    int len = strlen(text);
    int out_pos = 0;
    int i = 0;
    
    const char *bold_color = (style == STYLE_LIGHT) ? COLOR_BLACK BOLD : COLOR_WHITE BOLD;
    const char *italic_color = (style == STYLE_LIGHT) ? COLOR_BLACK DIM : COLOR_WHITE DIM;
    // 内联代码颜色（与 get_markdown_color 保持一致，包含背景色）
    const char *code_color = (style == STYLE_LIGHT) ? 
        "\033[38;5;88m\033[48;5;253m" : "\033[38;5;221m\033[48;5;236m";  // 深红/黄色 + 背景
    
    while (i < len && out_pos < max_len - 100) {  // 预留空间给 ANSI 代码
        // 检查嵌套图片链接 [![alt](img-url)](link-url)（必须优先处理）
        if (text[i] == '[' && i + 1 < len && text[i + 1] == '!' && i + 2 < len && text[i + 2] == '[') {
            int outer_start = i;
            i++;  // 跳过第一个 [
            
            // 现在处理 ![alt](img-url)
            if (text[i] == '!' && i + 1 < len && text[i + 1] == '[') {
                i += 2;  // 跳过 ![
                int alt_start = i;
                while (i < len && text[i] != ']') {
                    i++;
                }
                if (i < len && text[i] == ']' && i + 1 < len && text[i + 1] == '(') {
                    int alt_end = i;
                    i += 2;  // 跳过 ](
                    int img_url_start = i;
                    while (i < len && text[i] != ')') {
                        i++;
                    }
                    if (i < len && text[i] == ')') {
                        int img_url_end = i;
                        i++;  // 跳过 )
                        
                        // 检查外层链接
                        if (i < len && text[i] == ']' && i + 1 < len && text[i + 1] == '(') {
                            i += 2;  // 跳过 ](
                            int link_url_start = i;
                            while (i < len && text[i] != ')') {
                                i++;
                            }
                            if (i < len && text[i] == ')') {
                                int link_url_end = i;
                                i++;  // 跳过 )
                                
                                // 格式化为 "Image: alt → img-url" + 换行 + "link-url"（带颜色）
                                const char *label_color = (style == STYLE_LIGHT) ? 
                                    "\033[38;5;101m" : "\033[38;5;244m";  // 灰色
                                const char *link_color = (style == STYLE_LIGHT) ? 
                                    "\033[38;5;25m\033[4m" : "\033[38;5;81m\033[4m";  // 蓝色/青色 + 下划线
                                
                                out_pos += snprintf(output + out_pos, max_len - out_pos, "%sImage: %s", label_color, COLOR_RESET);
                                int alt_len = alt_end - alt_start;
                                if (alt_len > 0 && alt_len < 200) {
                                    memcpy(output + out_pos, text + alt_start, alt_len);
                                    out_pos += alt_len;
                                    output[out_pos] = '\0';
                                }
                                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s → %s", label_color, link_color);
                                int img_url_len = img_url_end - img_url_start;
                                if (img_url_len > 0 && img_url_len < 500) {
                                    memcpy(output + out_pos, text + img_url_start, img_url_len);
                                    out_pos += img_url_len;
                                    output[out_pos] = '\0';
                                }
                                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s\n%s", COLOR_RESET, link_color);
                                int link_url_len = link_url_end - link_url_start;
                                if (link_url_len > 0 && link_url_len < 500) {
                                    memcpy(output + out_pos, text + link_url_start, link_url_len);
                                    out_pos += link_url_len;
                                    output[out_pos] = '\0';
                                }
                                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                                continue;
                            }
                        }
                    }
                }
            }
            // 如果不是完整的嵌套链接，回退
            i = outer_start;
        }
        
        // 检查普通图片链接 ![alt](url)
        if (text[i] == '!' && i + 1 < len && text[i + 1] == '[') {
            int link_start = i;
            i += 2;  // 跳过 ![
            int text_start = i;
            while (i < len && text[i] != ']') {
                i++;
            }
            if (i < len && text[i] == ']' && i + 1 < len && text[i + 1] == '(') {
                int text_end = i;
                i += 2;  // 跳过 ](
                int url_start = i;
                while (i < len && text[i] != ')') {
                    i++;
                }
                if (i < len && text[i] == ')') {
                    int url_end = i;
                    i++;  // 跳过 )
                    
                    // 普通图片链接，格式化为 "Image: alt → url"（带颜色）
                    const char *label_color = (style == STYLE_LIGHT) ? 
                        "\033[38;5;101m" : "\033[38;5;244m";  // 灰色
                    const char *link_color = (style == STYLE_LIGHT) ? 
                        "\033[38;5;25m\033[4m" : "\033[38;5;81m\033[4m";  // 蓝色/青色 + 下划线
                    
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%sImage: %s", label_color, COLOR_RESET);
                    int text_len = text_end - text_start;
                    if (text_len > 0 && text_len < 200) {
                        memcpy(output + out_pos, text + text_start, text_len);
                        out_pos += text_len;
                        output[out_pos] = '\0';
                    }
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s → %s", label_color, link_color);
                    int url_len = url_end - url_start;
                    if (url_len > 0 && url_len < 500) {
                        memcpy(output + out_pos, text + url_start, url_len);
                        out_pos += url_len;
                        output[out_pos] = '\0';
                    }
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s\n", COLOR_RESET);  // 换行
                    continue;
                }
            }
            // 如果不是完整链接，回退
            i = link_start;
        }
        
        // 检查代码行内格式 `code`
        if (text[i] == '`') {
            int start = i;
            i++;
            while (i < len && text[i] != '`') {
                i++;
            }
            if (i < len) {
                // 找到匹配的 `
                int code_len = i - start - 1;
                if (code_len > 0 && code_len < 100) {
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", code_color);
                    strncpy(output + out_pos, text + start + 1, code_len);
                    out_pos += code_len;
                    output[out_pos] = '\0';
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                    i++;
                    continue;
                }
            }
        }
        
        // 检查普通链接 [text](url)
        if (text[i] == '[') {
            int link_start = i;
            i++;
            int text_start = i;
            while (i < len && text[i] != ']') {
                i++;
            }
            if (i < len && text[i] == ']' && i + 1 < len && text[i + 1] == '(') {
                int text_end = i;
                i += 2;  // 跳过 ](
                int url_start = i;
                while (i < len && text[i] != ')') {
                    i++;
                }
                if (i < len && text[i] == ')') {
                    int url_end = i;
                    i++;  // 跳过 )
                    
                    // 普通链接，显示链接文本（带颜色和下划线）
                    const char *link_color = (style == STYLE_LIGHT) ? 
                        "\033[38;5;25m\033[4m" : "\033[38;5;81m\033[4m";  // 蓝色/青色 + 下划线
                    
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", link_color);
                    int text_len = text_end - text_start;
                    if (text_len > 0 && text_len < 200) {
                        memcpy(output + out_pos, text + text_start, text_len);
                        out_pos += text_len;
                        output[out_pos] = '\0';
                    }
                    out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                    
                    // 添加换行和 URL（仅当 URL 是文件路径或完整 URL 时）
                    int url_len = url_end - url_start;
                    if (url_len > 0 && url_len < 500) {
                        // 检查是否是文件路径或完整 URL
                        char url_buffer[512];
                        memcpy(url_buffer, text + url_start, url_len);
                        url_buffer[url_len] = '\0';
                        
                        // 如果 URL 以 / 或 http 开头，或包含 .md/.html 等，显示 URL（灰色）
                        if (url_buffer[0] == '/' || url_buffer[0] == '.' || 
                            strstr(url_buffer, "http://") || strstr(url_buffer, "https://") ||
                            strstr(url_buffer, ".md") || strstr(url_buffer, ".html")) {
                            const char *url_color = (style == STYLE_LIGHT) ? 
                                "\033[38;5;101m" : "\033[38;5;244m";  // 灰色
                            out_pos += snprintf(output + out_pos, max_len - out_pos, "\n%s", url_color);
                            memcpy(output + out_pos, text + url_start, url_len);
                            out_pos += url_len;
                            output[out_pos] = '\0';
                            out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                        }
                    }
                    continue;
                }
            }
            // 如果不是完整链接，回退
            i = link_start;
        }
        
        // 检查加粗斜体 ***text*** 或 ___text___（优先处理）
        if ((text[i] == '*' && i + 2 < len && text[i + 1] == '*' && text[i + 2] == '*') ||
            (text[i] == '_' && i + 2 < len && text[i + 1] == '_' && text[i + 2] == '_')) {
            char marker = text[i];
            int start = i;
            i += 3;
            while (i + 2 < len && !(text[i] == marker && text[i + 1] == marker && text[i + 2] == marker)) {
                i++;
            }
            if (i + 2 < len && text[i] == marker && text[i + 1] == marker && text[i + 2] == marker) {
                // 找到匹配的结束标记，应用加粗+斜体
                const char *bold_italic_color = (style == STYLE_LIGHT) ? 
                    COLOR_BLACK BOLD "\033[3m" : COLOR_WHITE BOLD "\033[3m";
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", bold_italic_color);
                int text_len = i - start - 3;
                if (text_len > 0 && text_len < 200) {
                    strncpy(output + out_pos, text + start + 3, text_len);
                    out_pos += text_len;
                    output[out_pos] = '\0';
                }
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                i += 3;
                continue;
            }
            // 如果不是完整加粗斜体，回退
            i = start;
        }
        
        // 检查粗体 **text** 或 __text__
        if ((text[i] == '*' && i + 1 < len && text[i + 1] == '*') ||
            (text[i] == '_' && i + 1 < len && text[i + 1] == '_')) {
            char marker = text[i];
            int start = i;
            i += 2;
            while (i < len && !(text[i] == marker && i + 1 < len && text[i + 1] == marker)) {
                i++;
            }
            if (i < len && i + 1 < len) {
                // 找到匹配的结束标记
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", bold_color);
                int text_len = i - start - 2;
                if (text_len > 0 && text_len < 200) {
                    strncpy(output + out_pos, text + start + 2, text_len);
                    out_pos += text_len;
                    output[out_pos] = '\0';
                }
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                i += 2;
                continue;
            }
            // 如果不是完整粗体，回退
            i = start;
        }
        
        // 检查斜体 *text* 或 _text_（单个字符，不是双字符）
        if ((text[i] == '*' && (i == 0 || text[i-1] != '*') && (i + 1 >= len || text[i+1] != '*')) ||
            (text[i] == '_' && (i == 0 || text[i-1] != '_') && (i + 1 >= len || text[i+1] != '_'))) {
            char marker = text[i];
            int start = i;
            i++;
            while (i < len && text[i] != marker) {
                // 确保不是双字符标记的一部分
                if (text[i] == marker && i + 1 < len && text[i + 1] == marker) {
                    break;
                }
                i++;
            }
            if (i < len && text[i] == marker) {
                // 找到匹配的结束标记
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", italic_color);
                int text_len = i - start - 1;
                if (text_len > 0 && text_len < 200) {
                    strncpy(output + out_pos, text + start + 1, text_len);
                    out_pos += text_len;
                    output[out_pos] = '\0';
                }
                out_pos += snprintf(output + out_pos, max_len - out_pos, "%s", COLOR_RESET);
                i++;
                continue;
            }
            // 如果不是完整斜体，回退
            i = start;
        }
        
        // 普通字符
        output[out_pos++] = text[i++];
        output[out_pos] = '\0';
    }
    
    output[out_pos] = '\0';
}

