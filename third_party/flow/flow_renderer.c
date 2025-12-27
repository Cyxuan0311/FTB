#include "flow.h"
#include "flow_syntax_highlighter.h"
#include "flow_html.h"
#include "../include/common.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdbool.h>

// 获取终端宽度
static int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 80;  // 默认宽度
}

// 从代码块标记行提取语言
static const char* extract_code_language(const char *line) {
    if (line == NULL || strncmp(line, "```", 3) != 0) {
        return NULL;
    }
    const char *lang = line + 3;
    while (*lang == ' ' || *lang == '\t') lang++;
    if (*lang == '\0' || *lang == '\n') {
        return NULL;
    }
    return lang;
}

// 检测终端背景色
StyleType detect_terminal_style(void) {
    // 简化实现：检查环境变量或使用默认值
    const char *term_bg = getenv("COLORFGBG");
    if (term_bg != NULL) {
        // COLORFGBG 格式通常是 "15;0" (前景;背景)
        // 如果背景是 0-7，可能是深色；8-15 可能是浅色
        // 这里简化处理，默认返回深色
    }
    return STYLE_DARK;  // 默认深色主题
}

// 获取样式颜色（参考 glow 的颜色方案）
const char* get_style_color(const char *element, StyleType style) {
    // 深色主题颜色（参考 glow dark style）
    static const char* dark_colors[] = {
        [MD_HEADING1] = "\033[38;5;81m\033[1m",      // 青色加粗
        [MD_HEADING2] = "\033[38;5;81m",             // 青色
        [MD_HEADING3] = "\033[38;5;111m\033[1m",     // 蓝色加粗
        [MD_HEADING4] = "\033[38;5;111m",            // 蓝色
        [MD_HEADING5] = "\033[38;5;177m",            // 紫色
        [MD_HEADING6] = "\033[38;5;177m",            // 紫色
        [MD_BOLD] = "\033[37m\033[1m",               // 白色加粗
        [MD_ITALIC] = "\033[37m\033[3m",             // 白色斜体
        [MD_CODE_INLINE] = "\033[38;5;221m",        // 黄色
        [MD_CODE_BLOCK] = "\033[38;5;149m",         // 绿色
        [MD_LIST_ITEM] = "\033[37m",                 // 白色
        [MD_LIST_ORDERED] = "\033[37m",              // 白色
        [MD_QUOTE] = "\033[38;5;221m",               // 黄色
        [MD_LINK] = "\033[38;5;81m\033[4m",         // 青色下划线
        [MD_HR] = "\033[38;5;240m",                  // 灰色
        [MD_NORMAL] = COLOR_RESET,
        [MD_EMPTY] = COLOR_RESET
    };
    
    // 浅色主题颜色（参考 glow light style）
    static const char* light_colors[] = {
        [MD_HEADING1] = "\033[38;5;25m\033[1m",      // 深蓝加粗
        [MD_HEADING2] = "\033[38;5;25m",             // 深蓝
        [MD_HEADING3] = "\033[38;5;90m\033[1m",      // 深紫加粗
        [MD_HEADING4] = "\033[38;5;90m",             // 深紫
        [MD_HEADING5] = "\033[38;5;88m",             // 深红
        [MD_HEADING6] = "\033[38;5;88m",             // 深红
        [MD_BOLD] = "\033[30m\033[1m",               // 黑色加粗
        [MD_ITALIC] = "\033[30m\033[3m",             // 黑色斜体
        [MD_CODE_INLINE] = "\033[38;5;88m",          // 深红
        [MD_CODE_BLOCK] = "\033[38;5;28m",          // 深绿
        [MD_LIST_ITEM] = "\033[30m",                 // 黑色
        [MD_LIST_ORDERED] = "\033[30m",             // 黑色
        [MD_QUOTE] = "\033[38;5;94m",                // 棕色
        [MD_LINK] = "\033[38;5;25m\033[4m",         // 深蓝下划线
        [MD_HR] = "\033[38;5;244m",                  // 灰色
        [MD_NORMAL] = COLOR_RESET,
        [MD_EMPTY] = COLOR_RESET
    };
    
    // 将元素名转换为索引（简化实现）
    int index = MD_NORMAL;
    if (strcmp(element, "heading1") == 0) index = MD_HEADING1;
    else if (strcmp(element, "heading2") == 0) index = MD_HEADING2;
    else if (strcmp(element, "heading3") == 0) index = MD_HEADING3;
    else if (strcmp(element, "heading4") == 0) index = MD_HEADING4;
    else if (strcmp(element, "heading5") == 0) index = MD_HEADING5;
    else if (strcmp(element, "heading6") == 0) index = MD_HEADING6;
    else if (strcmp(element, "bold") == 0) index = MD_BOLD;
    else if (strcmp(element, "italic") == 0) index = MD_ITALIC;
    else if (strcmp(element, "code") == 0) index = MD_CODE_INLINE;
    else if (strcmp(element, "codeblock") == 0) index = MD_CODE_BLOCK;
    else if (strcmp(element, "list") == 0) index = MD_LIST_ITEM;
    else if (strcmp(element, "quote") == 0) index = MD_QUOTE;
    else if (strcmp(element, "link") == 0) index = MD_LINK;
    else if (strcmp(element, "hr") == 0) index = MD_HR;
    
    if (style == STYLE_LIGHT) {
        return light_colors[index];
    }
    return dark_colors[index];
}

// 获取 Markdown 类型的颜色（参考 glow 的颜色方案，优化配色）
static const char* get_markdown_color(MarkdownType type, StyleType style) {
    if (style == STYLE_LIGHT) {
        // 浅色主题（参考 glow light style，优化配色）
        switch (type) {
            case MD_HEADING1: return "\033[38;5;25m\033[1m";      // 深蓝加粗
            case MD_HEADING2: return "\033[38;5;25m";             // 深蓝
            case MD_HEADING3: return "\033[38;5;90m\033[1m";        // 深紫加粗
            case MD_HEADING4: return "\033[38;5;90m";              // 深紫
            case MD_HEADING5: return "\033[38;5;88m\033[1m";       // 深红加粗
            case MD_HEADING6: return "\033[38;5;88m";              // 深红
            case MD_BOLD: return "\033[30m\033[1m";                // 黑色加粗
            case MD_ITALIC: return "\033[30m\033[3m";              // 黑色斜体
            case MD_CODE_INLINE: return "\033[38;5;88m\033[48;5;253m";  // 深红，浅灰背景
            case MD_CODE_BLOCK: return "\033[38;5;28m";           // 深绿
            case MD_LIST_ITEM: return "\033[30m";                  // 黑色
            case MD_LIST_ORDERED: return "\033[30m";              // 黑色
            case MD_QUOTE: return "\033[38;5;94m";                 // 棕色
            case MD_LINK: return "\033[38;5;25m\033[4m";          // 深蓝下划线
            case MD_HR: return "\033[38;5;244m";                   // 灰色
            default: return COLOR_RESET;
        }
    } else {
        // 深色主题（参考 glow dark style，优化配色）
        switch (type) {
            case MD_HEADING1: return "\033[38;5;81m\033[1m";      // 青色加粗
            case MD_HEADING2: return "\033[38;5;81m";              // 青色
            case MD_HEADING3: return "\033[38;5;111m\033[1m";     // 蓝色加粗
            case MD_HEADING4: return "\033[38;5;111m";            // 蓝色
            case MD_HEADING5: return "\033[38;5;177m\033[1m";      // 紫色加粗
            case MD_HEADING6: return "\033[38;5;177m";            // 紫色
            case MD_BOLD: return "\033[37m\033[1m";                // 白色加粗
            case MD_ITALIC: return "\033[37m\033[3m";              // 白色斜体
            case MD_CODE_INLINE: return "\033[38;5;221m\033[48;5;236m";  // 黄色，深灰背景
            case MD_CODE_BLOCK: return "\033[38;5;149m";          // 绿色
            case MD_LIST_ITEM: return "\033[37m";                 // 白色
            case MD_LIST_ORDERED: return "\033[37m";              // 白色
            case MD_QUOTE: return "\033[38;5;221m";               // 黄色
            case MD_LINK: return "\033[38;5;81m\033[4m";         // 青色下划线
            case MD_HR: return "\033[38;5;240m";                   // 灰色
            default: return COLOR_RESET;
        }
    }
}

// 渲染单行（table_col_widths 和 table_num_cols 由调用者查找并传递）
void render_line(const char *line, MarkdownType type, int level, StyleType style, int line_number, bool show_line_num, const int *table_col_widths, int table_num_cols) {
    (void)level;  // 暂时未使用
    
    // 显示行号
    if (show_line_num) {
        const char *line_num_color = (style == STYLE_LIGHT) ? 
            "\033[38;5;101m" : "\033[38;5;244m";  // 灰色
        printf("%s%4d │%s ", line_num_color, line_number, COLOR_RESET);
    }
    
    if (type == MD_EMPTY) {
        printf("\n");
        return;
    }
    
    const char *color = get_markdown_color(type, style);
    
    switch (type) {
        case MD_HEADING1:
        case MD_HEADING2:
        case MD_HEADING3:
        case MD_HEADING4:
        case MD_HEADING5:
        case MD_HEADING6: {
            char heading_text[MAX_LINE_LENGTH];
            char cleaned_text[MAX_LINE_LENGTH];
            char formatted_text[MAX_LINE_LENGTH * 2];
            
            if (type == MD_HEADING1) {
                // H1: 反白显示，只有在 HTML 有居中属性时才居中
                extract_heading_text(line, heading_text, sizeof(heading_text));
                // 如果包含 HTML，使用 HTML 解析器；否则移除标签
                bool has_html = (strchr(heading_text, '<') != NULL && strchr(heading_text, '>') != NULL);
                bool should_center = false;
                
                if (has_html) {
                    // 检查是否有居中属性
                    should_center = is_html_centered(heading_text);
                    render_html_content(heading_text, cleaned_text, sizeof(cleaned_text), style);
                    parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                } else {
                strip_html_tags(heading_text, cleaned_text, sizeof(cleaned_text));
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                }
                
                // 反白颜色（背景色和前景色互换）
                const char *inverse_color = (style == STYLE_LIGHT) ? 
                    "\033[7m\033[1m\033[38;5;25m" : "\033[7m\033[1m\033[38;5;81m";
                
                printf("\n");  // 前面添加空行
                
                // 只有在有居中属性时才居中显示
                if (should_center) {
                int width = get_terminal_width();
                if (show_line_num) width -= 7;
                
                // 计算文本实际显示宽度（去除 ANSI 代码）
                int text_width = 0;
                const char *p = formatted_text;
                bool in_ansi = false;
                while (*p) {
                    if (*p == '\033' && p[1] == '[') {
                        in_ansi = true;
                        p += 2;
                        while (*p && *p != 'm') p++;
                        if (*p == 'm') p++;
                        in_ansi = false;
                        continue;
                    }
                    if (!in_ansi) {
                        // 简单判断中文字符（UTF-8）
                        if ((unsigned char)*p >= 0x80) {
                            text_width += 2;  // 中文字符占2个宽度
                            // 跳过 UTF-8 多字节字符的后续字节
                            if ((unsigned char)*p >= 0xE0) p += 2;
                            else if ((unsigned char)*p >= 0xC0) p += 1;
                        } else {
                            text_width += 1;  // 英文字符占1个宽度
                        }
                    }
                    p++;
                }
                
                // 计算左侧填充空格数量以居中
                int padding = (width - text_width) / 2;
                if (padding < 0) padding = 0;
                
                // 左侧填充
                for (int i = 0; i < padding; i++) {
                    printf(" ");
                }
                }
                
                // 反白文本（只反白文本部分）
                printf("%s %s %s\n", inverse_color, formatted_text, COLOR_RESET);
                printf("\n");  // 后面添加空行
            } else {
                // H2-H6: 保留 # 符号，改进间距
                const char *p = line;
                // 跳过前导空格
                while (*p == ' ' || *p == '\t') p++;
                
                // 提取 # 符号
                char prefix[10] = "";
                int prefix_len = 0;
                while (*p == '#' && prefix_len < 9) {
                    prefix[prefix_len++] = '#';
                    p++;
                }
                prefix[prefix_len] = '\0';
                
                // 跳过 # 后的空格
                while (*p == ' ' || *p == '\t') p++;
                
                // 处理标题文本（支持 HTML）
                if (strchr(p, '<') != NULL && strchr(p, '>') != NULL) {
                    render_html_content(p, cleaned_text, sizeof(cleaned_text), style);
                    parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                } else {
                strip_html_tags(p, cleaned_text, sizeof(cleaned_text));
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                }
                
                // 根据标题级别添加不同的间距（参考 glow）
                if (type == MD_HEADING2) {
                    printf("\n");  // H2 前添加空行
                } else if (type == MD_HEADING3 || type == MD_HEADING4) {
                    // H3/H4 前添加较小间距
                }
                printf("%s%s %s%s\n", color, prefix, formatted_text, COLOR_RESET);
                // 标题后添加空行（除了 H6）
                if (type != MD_HEADING6) {
                    printf("\n");
                }
            }
            break;
        }
        case MD_QUOTE: {
            char quote_text[MAX_LINE_LENGTH];
            char cleaned_text[MAX_LINE_LENGTH];
            char formatted_text[MAX_LINE_LENGTH * 2];
            extract_quote_text(line, quote_text, sizeof(quote_text));
            // 支持 HTML 解析
            if (strchr(quote_text, '<') != NULL && strchr(quote_text, '>') != NULL) {
                render_html_content(quote_text, cleaned_text, sizeof(cleaned_text), style);
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            } else {
            strip_html_tags(quote_text, cleaned_text, sizeof(cleaned_text));
            parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            }
            // 引用块前添加垂直线和缩进（类似 glow，优化样式）
            const char *quote_border = (style == STYLE_LIGHT) ? 
                "\033[38;5;246m" : "\033[38;5;240m";
            const char *quote_bg = (style == STYLE_LIGHT) ? 
                "\033[48;5;253m" : "\033[48;5;236m";  // 浅灰或深灰背景
            // 使用更粗的边框字符，添加背景色
            printf("%s%s│%s %s%s%s\n", quote_border, quote_bg, COLOR_RESET, 
                   color, formatted_text, COLOR_RESET);
            break;
        }
        case MD_LIST_ITEM: {
            char list_text[MAX_LINE_LENGTH];
            char cleaned_text[MAX_LINE_LENGTH];
            char formatted_text[MAX_LINE_LENGTH * 2];
            extract_list_text(line, list_text, sizeof(list_text));
            // 支持 HTML 解析
            if (strchr(list_text, '<') != NULL && strchr(list_text, '>') != NULL) {
                render_html_content(list_text, cleaned_text, sizeof(cleaned_text), style);
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            } else {
            strip_html_tags(list_text, cleaned_text, sizeof(cleaned_text));
            parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            }
            // 根据层级缩进（每级2个空格），优化符号显示
            for (int i = 0; i < level; i++) {
                printf("  ");
            }
            // 使用更美观的列表符号（参考 glow）
            const char *bullet = (level == 0) ? "•" : "◦";
            printf("%s  %s %s%s\n", color, bullet, formatted_text, COLOR_RESET);
            break;
        }
        case MD_LIST_ORDERED: {
            char list_text[MAX_LINE_LENGTH];
            char cleaned_text[MAX_LINE_LENGTH];
            char formatted_text[MAX_LINE_LENGTH * 2];
            extract_list_text(line, list_text, sizeof(list_text));
            // 支持 HTML 解析
            if (strchr(list_text, '<') != NULL && strchr(list_text, '>') != NULL) {
                render_html_content(list_text, cleaned_text, sizeof(cleaned_text), style);
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            } else {
            strip_html_tags(list_text, cleaned_text, sizeof(cleaned_text));
            parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
            }
            // 提取序号
            int num = 0;
            const char *p = line;
            while (*p == ' ' || *p == '\t') p++;
            while (isdigit(*p)) {
                num = num * 10 + (*p - '0');
                p++;
            }
            // 根据层级缩进（每级2个空格）
            for (int i = 0; i < level; i++) {
                printf("  ");
            }
            printf("%s%d. %s%s\n", color, num, formatted_text, COLOR_RESET);
            break;
        }
        case MD_HR: {
            int width = get_terminal_width();
            if (show_line_num) {
                width -= 7;  // 减去行号宽度
            }
            // 水平线前后添加空行（参考 glow）
            printf("\n");
            printf("%s", color);
            // 使用更粗的线条字符
            for (int i = 0; i < width; i++) {
                printf("━");
            }
            printf("%s\n", COLOR_RESET);
            printf("\n");
            break;
        }
        case MD_CODE_BLOCK: {
            // 代码块标记，不显示（类似 glow）
            char formatted_marker[MAX_LINE_LENGTH];
            format_code_block_marker(line, formatted_marker, sizeof(formatted_marker));
            if (formatted_marker[0] != '\0') {
                printf("%s\n", formatted_marker);
            }
            break;
        }
        case MD_NORMAL: {
            // 检查是否为表格分隔行（必须在表格行之前检查）
            if (is_table_separator(line)) {
                // 表格分隔行，使用 Unicode 字符（类似 glow）
                const char *sep_color = (style == STYLE_LIGHT) ? 
                    "\033[38;5;244m" : "\033[38;5;240m";
                
                // 使用动态列宽，如果没有提供则使用默认值
                int default_col_widths[] = {16, 15, 29, 15};
                int default_num_cols = 4;
                const int *col_widths = (table_col_widths != NULL && table_num_cols > 0) ? table_col_widths : default_col_widths;
                int num_cols = (table_col_widths != NULL && table_num_cols > 0) ? table_num_cols : default_num_cols;
                
                // 调试信息
                
                printf("%s", sep_color);
                // 生成分隔线：与 format_table_row 的格式保持一致
                // format_table_row 格式：内容 + 填充空格 + " │ " + 下一列
                // 分隔行格式：─ * col_width + " ┼ " + ─ * col_width + ...
                // 注意：分隔行的 " ┼ " 与表格行的 " │ " 对齐
                // 使用更粗的线条字符（参考 glow）
                for (int i = 0; i < num_cols; i++) {
                    if (i > 0) {
                        printf(" ┼ ");  // 与 format_table_row 中的 " │ " 保持一致（空格+分隔符+空格）
                    }
                    // 绘制该列的宽度（与单元格内容宽度一致），使用更粗的线条
                    for (int j = 0; j < col_widths[i]; j++) {
                        printf("─");
                    }
                }
                printf("%s\n", COLOR_RESET);
            } else if (is_table_row(line)) {
                // 表格行
                char formatted_row[MAX_LINE_LENGTH * 2];
                format_table_row(line, formatted_row, sizeof(formatted_row), style, get_terminal_width() - (show_line_num ? 7 : 0), table_col_widths, table_num_cols);
                printf("%s\n", formatted_row);
            } else {
                // 普通文本，处理 HTML，优化显示
                char cleaned_text[MAX_LINE_LENGTH];
                char formatted_text[MAX_LINE_LENGTH * 2];
                // 支持 HTML 解析
                if (strchr(line, '<') != NULL && strchr(line, '>') != NULL) {
                    render_html_content(line, cleaned_text, sizeof(cleaned_text), style);
                    parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                } else {
                strip_html_tags(line, cleaned_text, sizeof(cleaned_text));
                parse_inline_formatting(cleaned_text, formatted_text, sizeof(formatted_text), style);
                }
                // 普通文本使用默认颜色，确保可读性
                const char *normal_color = (style == STYLE_LIGHT) ? 
                    "\033[30m" : "\033[37m";  // 黑色或白色
                printf("%s%s%s\n", normal_color, formatted_text, COLOR_RESET);
            }
            break;
        }
        default:
            printf("%s%s%s\n", COLOR_RESET, line, COLOR_RESET);
            break;
    }
}

// 显示状态栏（参考 glow 的设计）
void render_status_bar(const char *filename, int current_line, int total_lines, StyleType style) {
    int width = get_terminal_width();
    const char *bg_color = (style == STYLE_LIGHT) ? 
        "\033[48;5;230m" : "\033[48;5;36m";  // 浅灰或深灰背景
    const char *fg_color = (style == STYLE_LIGHT) ? 
        "\033[38;5;101m" : "\033[38;5;244m";  // 灰色文字
    
    // 计算百分比
    float percent = 0.0;
    if (total_lines > 0) {
        percent = ((float)current_line / (float)total_lines) * 100.0;
        if (percent < 0.0) percent = 0.0;
        if (percent > 100.0) percent = 100.0;
    }
    
    // Logo（占用约 3 个字符）
    const char *logo = "💅 ";
    int logo_width = 3;
    
    // 滚动百分比（格式： 100% ）
    char scroll_percent[16];
    snprintf(scroll_percent, sizeof(scroll_percent), " %3.0f%% ", percent);
    int scroll_width = 7;
    
    // 帮助提示
    const char *help_note = " ? Help ";
    int help_width = 8;
    
    // 文件名/备注（截断以适应宽度）
    char note[256];
    if (filename && strlen(filename) > 0) {
        strncpy(note, filename, sizeof(note) - 1);
        note[sizeof(note) - 1] = '\0';
    } else {
        strcpy(note, "stdin");
    }
    
    // 计算可用空间
    int available_width = width - logo_width - scroll_width - help_width - 4; // 4 个分隔符
    if (available_width < 10) available_width = 10;
    
    // 截断文件名以适应宽度
    int note_len = strlen(note);
    if (note_len > available_width) {
        note[available_width - 3] = '\0';
        strcat(note, "...");
    }
    
    // 构建状态栏
    printf("%s%s", bg_color, fg_color);
    printf("%s", logo);
    printf(" %s ", note);
    
    // 填充空格
    int used_width = logo_width + strlen(note) + scroll_width + help_width + 4;
    int padding = width - used_width;
    if (padding > 0) {
        for (int i = 0; i < padding; i++) {
            printf(" ");
        }
    }
    
    // 滚动百分比
    printf("%s", scroll_percent);
    
    // 帮助提示
    printf("%s", help_note);
    
    printf("%s\n", COLOR_RESET);
}

// 渲染 Markdown 内容
void render_markdown(const char *content, FlowOptions *opts) {
    if (content == NULL) {
        return;
    }
    
    StyleType style = opts->style;
    if (style == STYLE_AUTO) {
        style = detect_terminal_style();
    }
    
    // 计算总行数
    int total_lines = 0;
    const char *p = content;
    while (*p) {
        if (*p == '\n') total_lines++;
        p++;
    }
    if (content[strlen(content) - 1] != '\n') {
        total_lines++;  // 最后一行没有换行符
    }
    
    // 预先扫描所有表格，为每个表格计算列宽
    // 使用简单的行号映射来存储表格列宽
    typedef struct {
        int start_line;
        int end_line;
        int col_widths[64];
        int num_cols;
    } TableInfo;
    
    TableInfo tables[100];
    int table_count = 0;
    
    // 第一遍扫描：识别所有表格
    char *scan_copy = strdup(content);
    char *scan_line = strtok(scan_copy, "\n");
    int scan_line_num = 1;
    bool scan_in_code_block = false;
    bool scan_in_table = false;
    int table_start_line = 0;
    char *table_lines[1000];
    int table_line_count = 0;
    
    
    while (scan_line != NULL) {
        if (strncmp(scan_line, "```", 3) == 0) {
            scan_in_code_block = !scan_in_code_block;
            if (scan_in_code_block && scan_in_table) {
                // 代码块开始，结束当前表格
                if (table_count < 100 && table_line_count > 0) {
                    // 计算该表格的列宽
                    char table_buffer[10000] = "";
                    for (int i = 0; i < table_line_count; i++) {
                        strcat(table_buffer, table_lines[i]);
                        strcat(table_buffer, "\n");
                    }
                    calculate_single_table_widths(table_buffer, NULL, style, 
                                                 tables[table_count].col_widths, 
                                                 &tables[table_count].num_cols, 64);
                    tables[table_count].start_line = table_start_line;
                    tables[table_count].end_line = scan_line_num - 1;
                    table_count++;
                }
                scan_in_table = false;
                table_line_count = 0;
            }
        } else if (!scan_in_code_block) {
            bool is_table = is_table_row(scan_line) || is_table_separator(scan_line);
            
            if (is_table) {
                if (!scan_in_table) {
                    // 新表格开始
                    scan_in_table = true;
                    table_start_line = scan_line_num;
                    table_line_count = 0;
                }
                if (table_line_count < 1000) {
                    table_lines[table_line_count++] = scan_line;
                }
            } else if (scan_in_table) {
                // 表格结束：遇到非表格行
                // 检查是否是空行或 HTML 标签（这些不应该结束表格）
                bool is_empty = (strlen(scan_line) == 0 || 
                                (scan_line[0] == '\0') ||
                                (scan_line[0] == ' ' && strlen(scan_line) == strspn(scan_line, " \t")));
                bool is_html_tag = (strncmp(scan_line, "<", 1) == 0 || 
                                   strncmp(scan_line, "</", 2) == 0);
                
                // 检查是否是标题行（以 # 开头）或引用行（以 > 开头）
                bool is_heading = (scan_line[0] == '#');
                bool is_quote = (scan_line[0] == '>');
                
                // 如果是空行或 HTML 标签，继续当前表格
                // 但如果是标题行或引用行，应该结束表格（因为新章节开始）
                if (is_empty || is_html_tag) {
                    // 继续当前表格，不结束
                } else if (is_heading) {
                    // 遇到标题，结束当前表格（标题可能意味着新章节，但标题后可能有新表格）
                    if (table_count < 100 && table_line_count > 0) {
                        char table_buffer[10000] = "";
                        for (int i = 0; i < table_line_count; i++) {
                            strcat(table_buffer, table_lines[i]);
                            strcat(table_buffer, "\n");
                        }
                        calculate_single_table_widths(table_buffer, NULL, style, 
                                                     tables[table_count].col_widths, 
                                                     &tables[table_count].num_cols, 64);
                        tables[table_count].start_line = table_start_line;
                        tables[table_count].end_line = scan_line_num - 1;
                        table_count++;
                    }
                    scan_in_table = false;
                    table_line_count = 0;
                } else if (is_quote) {
                    // 遇到引用行，结束当前表格（引用行通常不在表格内）
                    if (table_count < 100 && table_line_count > 0) {
                        char table_buffer[10000] = "";
                        for (int i = 0; i < table_line_count; i++) {
                            strcat(table_buffer, table_lines[i]);
                            strcat(table_buffer, "\n");
                        }
                        calculate_single_table_widths(table_buffer, NULL, style, 
                                                     tables[table_count].col_widths, 
                                                     &tables[table_count].num_cols, 64);
                        tables[table_count].start_line = table_start_line;
                        tables[table_count].end_line = scan_line_num - 1;
                        table_count++;
                    }
                    scan_in_table = false;
                    table_line_count = 0;
                } else {
                    // 其他非表格行，结束表格
                    if (table_count < 100 && table_line_count > 0) {
                        char table_buffer[10000] = "";
                        for (int i = 0; i < table_line_count; i++) {
                            strcat(table_buffer, table_lines[i]);
                            strcat(table_buffer, "\n");
                        }
                        calculate_single_table_widths(table_buffer, NULL, style, 
                                                     tables[table_count].col_widths, 
                                                     &tables[table_count].num_cols, 64);
                        tables[table_count].start_line = table_start_line;
                        tables[table_count].end_line = scan_line_num - 1;
                        table_count++;
                    }
                    scan_in_table = false;
                    table_line_count = 0;
                }
            }
        }
        
        scan_line_num++;
        scan_line = strtok(NULL, "\n");
    }
    
    // 处理最后一个表格
    if (scan_in_table && table_count < 100 && table_line_count > 0) {
        char table_buffer[10000] = "";
        for (int i = 0; i < table_line_count; i++) {
            strcat(table_buffer, table_lines[i]);
            strcat(table_buffer, "\n");
        }
        calculate_single_table_widths(table_buffer, NULL, style, 
                                     tables[table_count].col_widths, 
                                     &tables[table_count].num_cols, 64);
        tables[table_count].start_line = table_start_line;
        tables[table_count].end_line = scan_line_num - 1;
        table_count++;
    }
    
    
    free(scan_copy);
    
    // 第二遍：渲染内容
    char *content_copy = strdup(content);
    char *line = strtok(content_copy, "\n");
    bool in_code_block = false;
    SyntaxType current_syntax = SYNTAX_NONE;
    int line_number = 1;
    
    while (line != NULL) {
        int level = 0;
        MarkdownType type = parse_markdown_line(line, &level);
        
        // 查找当前行属于哪个表格
        int *current_table_col_widths = NULL;
        int current_table_num_cols = 0;
        for (int i = 0; i < table_count; i++) {
            if (line_number >= tables[i].start_line && line_number <= tables[i].end_line) {
                current_table_col_widths = tables[i].col_widths;
                current_table_num_cols = tables[i].num_cols;
                if (is_table_row(line) || is_table_separator(line)) {
                }
                break;
            }
        }
        
        // 处理代码块
        if (type == MD_CODE_BLOCK) {
            if (!in_code_block) {
                // 代码块开始，提取语言
                const char *lang = extract_code_language(line);
                if (lang != NULL) {
                    char lang_name[64] = {0};
                    int i = 0;
                    while (*lang != '\0' && *lang != '\n' && i < 63) {
                        lang_name[i++] = *lang++;
                    }
                    lang_name[i] = '\0';
                    current_syntax = detect_syntax_type(lang_name);
                } else {
                    current_syntax = SYNTAX_NONE;
                }
                // 代码块开始前添加空行（参考 glow）
                printf("\n");
            } else {
                // 代码块结束
                current_syntax = SYNTAX_NONE;
                // 代码块结束后添加空行（参考 glow）
                printf("\n");
            }
            in_code_block = !in_code_block;
            // 不显示代码块标记（类似 glow）
            line_number++;
            line = strtok(NULL, "\n");
            continue;
        } else if (in_code_block) {
            // 代码块内的内容（带语法高亮和左边框，优化样式）
            char highlighted_line[8192];
            highlight_code_line(line, highlighted_line, sizeof(highlighted_line), 
                              current_syntax, style);
            
            const char *border_color = (style == STYLE_LIGHT) ? 
                "\033[38;5;244m" : "\033[38;5;240m";  // 灰色边框
            const char *code_bg = (style == STYLE_LIGHT) ? 
                "\033[48;5;253m" : "\033[48;5;236m";  // 浅灰或深灰背景
            if (opts->show_line_numbers) {
                const char *line_num_color = (style == STYLE_LIGHT) ? 
                    "\033[38;5;101m" : "\033[38;5;244m";
                printf("%s%4d │%s ", line_num_color, line_number, COLOR_RESET);
            }
            // 添加左边框和背景色（参考 glow）
            printf("%s%s│%s %s%s%s\n", border_color, code_bg, COLOR_RESET, 
                   highlighted_line, COLOR_RESET, COLOR_RESET);
        } else {
            // 对于表格行和分隔行，确保传递正确的列宽信息
            if (is_table_row(line) || is_table_separator(line)) {
                if (current_table_col_widths == NULL || current_table_num_cols == 0) {
                }
            }
            render_line(line, type, level, style, line_number, opts->show_line_numbers, 
                       current_table_col_widths, current_table_num_cols);
        }
        
        line_number++;
        line = strtok(NULL, "\n");
    }
    
    // 显示状态栏
    if (opts->show_status_bar) {
        render_status_bar(opts->filename, line_number - 1, total_lines, style);
    }
    
    free(content_copy);
}

// 渲染 Markdown 内容到字符串缓冲区
static void render_markdown_to_buffer(const char *content, FlowOptions *opts, char **output, size_t *output_size) {
    if (content == NULL || opts == NULL || output == NULL) {
        return;
    }
    
    // 使用临时文件或内存缓冲区来捕获渲染输出
    // 这里使用重定向 stdout 的方式
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        return;
    }
    
    // 保存原始 stdout
    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout == -1) {
        fclose(temp_file);
        return;
    }
    
    // 重定向 stdout 到临时文件
    if (dup2(fileno(temp_file), STDOUT_FILENO) == -1) {
        close(saved_stdout);
        fclose(temp_file);
        return;
    }
    
    // 渲染内容
    render_markdown(content, opts);
    fflush(stdout);
    
    // 恢复 stdout
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    
    // 读取临时文件内容
    fseek(temp_file, 0, SEEK_END);
    long size = ftell(temp_file);
    fseek(temp_file, 0, SEEK_SET);
    
    if (size > 0) {
        *output = (char *)malloc(size + 1);
        if (*output != NULL) {
            size_t read_result = fread(*output, 1, size, temp_file);
            (void)read_result;  // 忽略返回值（已检查 size > 0）
            (*output)[size] = '\0';
            if (output_size != NULL) {
                *output_size = size;
            }
        }
    }
    
    fclose(temp_file);
}

// 使用分页器显示（使用渲染后的内容）
void display_with_pager(const char *content, FlowOptions *opts) {
    if (content == NULL || opts == NULL) {
        return;
    }
    
    // 渲染内容到缓冲区
    char *rendered_content = NULL;
    render_markdown_to_buffer(content, opts, &rendered_content, NULL);
    
    if (rendered_content == NULL) {
        // 如果渲染失败，使用原始内容
        rendered_content = strdup(content);
    }
    
    const char *pager = getenv("PAGER");
    if (pager == NULL) {
        pager = "less -R";  // 使用 less -R 支持 ANSI 颜色
    }
    
    FILE *pipe = popen(pager, "w");
    if (pipe != NULL) {
        fprintf(pipe, "%s", rendered_content);
        pclose(pipe);
    } else {
        // 如果分页器失败，直接输出
        printf("%s", rendered_content);
    }
    
    free(rendered_content);
}

