#ifndef FLOW_H
#define FLOW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 4096
#define MAX_FILENAME_LENGTH 512

// 样式类型
typedef enum {
    STYLE_AUTO,    // 自动检测
    STYLE_DARK,    // 深色主题
    STYLE_LIGHT    // 浅色主题
} StyleType;

// Markdown 元素类型
typedef enum {
    MD_NORMAL,      // 普通文本
    MD_HEADING1,     // # 标题
    MD_HEADING2,     // ## 标题
    MD_HEADING3,     // ### 标题
    MD_HEADING4,     // #### 标题
    MD_HEADING5,     // ##### 标题
    MD_HEADING6,     // ###### 标题
    MD_BOLD,         // **粗体**
    MD_ITALIC,       // *斜体*
    MD_CODE_INLINE,  // `代码`
    MD_CODE_BLOCK,   // ```代码块```
    MD_LIST_ITEM,    // - 列表项
    MD_LIST_ORDERED, // 1. 有序列表
    MD_QUOTE,        // > 引用
    MD_LINK,         // [链接](url)
    MD_HR,           // --- 水平线
    MD_EMPTY         // 空行
} MarkdownType;

// 选项结构
typedef struct {
    bool show_help;
    bool show_version;
    bool use_pager;
    bool show_line_numbers;  // 显示行号
    bool show_status_bar;    // 显示状态栏
    int width;           // 输出宽度，0表示自动
    StyleType style;     // 样式类型
    char filename[MAX_FILENAME_LENGTH];
    bool from_stdin;     // 从标准输入读取
} FlowOptions;

// 初始化选项
void init_options(FlowOptions *opts);

// 解析命令行参数
int parse_arguments(int argc, char *argv[], FlowOptions *opts);

// 显示帮助信息
void print_help(const char *program_name);

// 显示版本信息
void print_version(void);

// 注意：read_file_content 和 read_stdin_content 已移至 flow_content.c
// 使用 read_and_process_content() 作为统一入口

// URL 相关函数
bool is_url(const char *path);
char* read_markdown_from_url(const char *path);

// 工具函数
char* remove_frontmatter(const char *content);
bool is_markdown_file(const char *filename);
char* wrap_code_block(const char *content, const char *language);
const char* get_file_extension(const char *filename);

// 解析 Markdown 行
MarkdownType parse_markdown_line(const char *line, int *level);

// 提取文本辅助函数
void extract_heading_text(const char *line, char *output, int max_len);
void extract_list_text(const char *line, char *output, int max_len);
void extract_quote_text(const char *line, char *output, int max_len);

// 解析行内格式（粗体、斜体、代码、链接）
void parse_inline_formatting(const char *text, char *output, int max_len, StyleType style);

// 格式化辅助函数
void strip_html_tags(const char *input, char *output, int max_len);
void format_image_link(const char *text, char *output, int max_len);

// HTML 解析函数（在 flow_html.c 中实现）
void render_html_content(const char *html, char *output, int max_len, StyleType style);
bool is_html_tag_line(const char *line);
bool is_html_centered(const char *text);
bool is_table_row(const char *line);
bool is_table_separator(const char *line);
void format_table_row(const char *line, char *output, int max_len, StyleType style, int terminal_width, const int *col_widths, int num_cols);
void calculate_table_column_widths(const char *content, StyleType style, int *col_widths, int *num_cols, int max_cols);
void calculate_single_table_widths(const char *table_start, const char *table_end, StyleType style, int *col_widths, int *num_cols, int max_cols);
void center_text(const char *text, int width, char *output, int max_len);
void format_code_block_marker(const char *line, char *output, int max_len);

// 渲染 Markdown 内容
void render_markdown(const char *content, FlowOptions *opts);

// 渲染单行
void render_line(const char *line, MarkdownType type, int level, StyleType style, int line_number, bool show_line_num, const int *table_col_widths, int table_num_cols);

// 显示状态栏
void render_status_bar(const char *filename, int current_line, int total_lines, StyleType style);

// 获取样式颜色
const char* get_style_color(const char *element, StyleType style);

// 检测终端背景色（深色/浅色）
StyleType detect_terminal_style(void);

// 使用分页器显示（使用渲染后的内容）
void display_with_pager(const char *content, FlowOptions *opts);

#endif // FLOW_H

