#ifndef FLOW_HTML_H
#define FLOW_HTML_H

#include "flow.h"
#include <stdbool.h>

// HTML 标签类型
typedef enum {
    HTML_TAG_UNKNOWN,
    HTML_TAG_STRONG,      // <strong>
    HTML_TAG_EM,          // <em>
    HTML_TAG_B,           // <b>
    HTML_TAG_I,           // <i>
    HTML_TAG_U,           // <u>
    HTML_TAG_S,           // <s>
    HTML_TAG_CODE,        // <code>
    HTML_TAG_PRE,         // <pre>
    HTML_TAG_H1,          // <h1>
    HTML_TAG_H2,          // <h2>
    HTML_TAG_H3,          // <h3>
    HTML_TAG_H4,          // <h4>
    HTML_TAG_H5,          // <h5>
    HTML_TAG_H6,          // <h6>
    HTML_TAG_P,           // <p>
    HTML_TAG_BR,          // <br>
    HTML_TAG_HR,          // <hr>
    HTML_TAG_BLOCKQUOTE,  // <blockquote>
    HTML_TAG_UL,           // <ul>
    HTML_TAG_OL,           // <ol>
    HTML_TAG_LI,           // <li>
    HTML_TAG_A,            // <a>
    HTML_TAG_IMG,          // <img>
    HTML_TAG_TABLE,        // <table>
    HTML_TAG_TR,           // <tr>
    HTML_TAG_TD,           // <td>
    HTML_TAG_TH,           // <th>
    HTML_TAG_DIV,          // <div>
    HTML_TAG_SPAN,         // <span>
    HTML_TAG_DETAILS,      // <details>
    HTML_TAG_SUMMARY,      // <summary>
    HTML_TAG_DEL,          // <del>
    HTML_TAG_INS,          // <ins>
    HTML_TAG_MARK          // <mark>
} HtmlTagType;

// HTML 标签信息
typedef struct {
    HtmlTagType type;
    char name[32];        // 标签名
    bool is_closing;      // 是否为闭合标签
    bool is_self_closing; // 是否为自闭合标签（如 <br/>）
    int start_pos;        // 在原文中的起始位置
    int end_pos;          // 在原文中的结束位置
} HtmlTag;

// 解析 HTML 标签
// 返回找到的标签信息，如果没找到返回 false
bool parse_html_tag(const char *text, int start_pos, HtmlTag *tag);

// 获取 HTML 标签类型
HtmlTagType get_html_tag_type(const char *tag_name);

// 检查是否为 HTML 标签
bool is_html_tag_line(const char *line);

// 渲染 HTML 内容（将 HTML 转换为带 ANSI 颜色的文本）
// 支持常见的 HTML 标签，转换为相应的 markdown 样式
void render_html_content(const char *html, char *output, int max_len, StyleType style);

// 处理单个 HTML 标签，返回渲染后的文本
void render_html_tag(const HtmlTag *tag, const char *content, char *output, int max_len, StyleType style);

// 提取 HTML 标签属性值（如 href, src, alt 等）
bool get_html_attribute(const char *tag_text, const char *attr_name, char *value, int max_len);

// 检查 HTML 标签是否有居中属性（align="center" 或 style="text-align: center"）
bool is_html_centered(const char *text);

// 处理 HTML 实体（如 &lt; &gt; &amp; &quot; &#39; 等）
void decode_html_entities(const char *input, char *output, int max_len);

#endif // FLOW_HTML_H

