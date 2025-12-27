#include "flow_html.h"
#include "../include/common.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// 获取 HTML 标签类型
HtmlTagType get_html_tag_type(const char *tag_name) {
    if (tag_name == NULL) return HTML_TAG_UNKNOWN;
    
    // 转换为小写进行比较
    char lower[32];
    int i = 0;
    while (tag_name[i] != '\0' && i < 31) {
        lower[i] = tolower(tag_name[i]);
        i++;
    }
    lower[i] = '\0';
    
    // 匹配常见标签
    if (strcmp(lower, "strong") == 0) return HTML_TAG_STRONG;
    if (strcmp(lower, "em") == 0) return HTML_TAG_EM;
    if (strcmp(lower, "b") == 0) return HTML_TAG_B;
    if (strcmp(lower, "i") == 0) return HTML_TAG_I;
    if (strcmp(lower, "u") == 0) return HTML_TAG_U;
    if (strcmp(lower, "s") == 0 || strcmp(lower, "strike") == 0) return HTML_TAG_S;
    if (strcmp(lower, "code") == 0) return HTML_TAG_CODE;
    if (strcmp(lower, "pre") == 0) return HTML_TAG_PRE;
    if (strcmp(lower, "h1") == 0) return HTML_TAG_H1;
    if (strcmp(lower, "h2") == 0) return HTML_TAG_H2;
    if (strcmp(lower, "h3") == 0) return HTML_TAG_H3;
    if (strcmp(lower, "h4") == 0) return HTML_TAG_H4;
    if (strcmp(lower, "h5") == 0) return HTML_TAG_H5;
    if (strcmp(lower, "h6") == 0) return HTML_TAG_H6;
    if (strcmp(lower, "p") == 0) return HTML_TAG_P;
    if (strcmp(lower, "br") == 0) return HTML_TAG_BR;
    if (strcmp(lower, "hr") == 0) return HTML_TAG_HR;
    if (strcmp(lower, "blockquote") == 0) return HTML_TAG_BLOCKQUOTE;
    if (strcmp(lower, "ul") == 0) return HTML_TAG_UL;
    if (strcmp(lower, "ol") == 0) return HTML_TAG_OL;
    if (strcmp(lower, "li") == 0) return HTML_TAG_LI;
    if (strcmp(lower, "a") == 0) return HTML_TAG_A;
    if (strcmp(lower, "img") == 0) return HTML_TAG_IMG;
    if (strcmp(lower, "table") == 0) return HTML_TAG_TABLE;
    if (strcmp(lower, "tr") == 0) return HTML_TAG_TR;
    if (strcmp(lower, "td") == 0) return HTML_TAG_TD;
    if (strcmp(lower, "th") == 0) return HTML_TAG_TH;
    if (strcmp(lower, "div") == 0) return HTML_TAG_DIV;
    if (strcmp(lower, "span") == 0) return HTML_TAG_SPAN;
    if (strcmp(lower, "details") == 0) return HTML_TAG_DETAILS;
    if (strcmp(lower, "summary") == 0) return HTML_TAG_SUMMARY;
    if (strcmp(lower, "del") == 0) return HTML_TAG_DEL;
    if (strcmp(lower, "ins") == 0) return HTML_TAG_INS;
    if (strcmp(lower, "mark") == 0) return HTML_TAG_MARK;
    
    return HTML_TAG_UNKNOWN;
}

// 解析 HTML 标签
bool parse_html_tag(const char *text, int start_pos, HtmlTag *tag) {
    if (text == NULL || tag == NULL || start_pos < 0) {
        return false;
    }
    
    int len = strlen(text);
    if (start_pos >= len) {
        return false;
    }
    
    // 查找 '<' 字符
    int i = start_pos;
    while (i < len && text[i] != '<') {
        i++;
    }
    
    if (i >= len) {
        return false;
    }
    
    tag->start_pos = i;
    i++; // 跳过 '<'
    
    // 检查是否为闭合标签 </tag>
    if (i < len && text[i] == '/') {
        tag->is_closing = true;
        i++;
    } else {
        tag->is_closing = false;
    }
    
    // 提取标签名
    int name_len = 0;
    while (i < len && text[i] != '>' && text[i] != ' ' && text[i] != '\t' && 
           text[i] != '/' && name_len < 31) {
        if (isalnum(text[i]) || text[i] == '-') {
            tag->name[name_len++] = tolower(text[i]);
            i++;
        } else {
            break;
        }
    }
    tag->name[name_len] = '\0';
    
    if (name_len == 0) {
        return false;
    }
    
    // 检查是否为自闭合标签 <tag/>
    tag->is_self_closing = false;
    while (i < len && text[i] != '>') {
        if (text[i] == '/' && i + 1 < len && text[i + 1] == '>') {
            tag->is_self_closing = true;
            break;
        }
        i++;
    }
    
    // 查找 '>'
    while (i < len && text[i] != '>') {
        i++;
    }
    
    if (i >= len) {
        return false;
    }
    
    tag->end_pos = i + 1; // 包含 '>'
    tag->type = get_html_tag_type(tag->name);
    
    return true;
}

// 检查是否为 HTML 标签行
bool is_html_tag_line(const char *line) {
    if (line == NULL) return false;
    
    const char *p = line;
    // 跳过前导空格
    while (*p == ' ' || *p == '\t') p++;
    
    // 检查是否以 < 开头
    return (*p == '<');
}

// 提取 HTML 标签属性值
bool get_html_attribute(const char *tag_text, const char *attr_name, char *value, int max_len) {
    if (tag_text == NULL || attr_name == NULL || value == NULL || max_len <= 0) {
        return false;
    }
    
    char attr_lower[64];
    int i = 0;
    while (attr_name[i] != '\0' && i < 63) {
        attr_lower[i] = tolower(attr_name[i]);
        i++;
    }
    attr_lower[i] = '\0';
    
    // 在标签文本中查找属性
    const char *p = tag_text;
    while (*p != '\0') {
        // 跳过空格和等号
        while (*p == ' ' || *p == '\t' || *p == '=') p++;
        
        if (*p == '\0') break;
        
        // 提取属性名
        char current_attr[64] = {0};
        int attr_pos = 0;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '=' && *p != '>' && attr_pos < 63) {
            current_attr[attr_pos++] = tolower(*p);
            p++;
        }
        current_attr[attr_pos] = '\0';
        
        // 检查是否匹配
        if (strcmp(current_attr, attr_lower) == 0) {
            // 跳过等号和引号
            while (*p == ' ' || *p == '\t' || *p == '=') p++;
            
            char quote = '\0';
            if (*p == '"' || *p == '\'') {
                quote = *p;
                p++;
            }
            
            // 提取属性值
            int value_pos = 0;
            while (*p != '\0' && *p != quote && *p != ' ' && *p != '\t' && *p != '>' && value_pos < max_len - 1) {
                if (quote == '\0' && (*p == ' ' || *p == '\t' || *p == '>')) {
                    break;
                }
                value[value_pos++] = *p;
                p++;
            }
            value[value_pos] = '\0';
            
            if (quote != '\0' && *p == quote) {
                p++;
            }
            
            return value_pos > 0;
        }
        
        // 继续查找下一个属性
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '>') p++;
    }
    
    return false;
}

// 检查 HTML 标签是否有居中属性
bool is_html_centered(const char *text) {
    if (text == NULL) return false;
    
    // 查找第一个 HTML 标签
    const char *p = text;
    while (*p != '\0' && *p != '<') p++;
    
    if (*p != '<') return false;
    
    // 提取标签文本（到 > 为止）
    const char *tag_start = p;
    p++;
    while (*p != '\0' && *p != '>') p++;
    
    if (*p != '>') return false;
    
    int tag_len = p - tag_start + 1;
    if (tag_len > 512) tag_len = 512;
    
    char tag_text[512];
    strncpy(tag_text, tag_start, tag_len);
    tag_text[tag_len] = '\0';
    
    // 检查 align="center" 或 align='center'
    char align_value[64];
    if (get_html_attribute(tag_text, "align", align_value, sizeof(align_value))) {
        // 转换为小写比较
        for (int i = 0; align_value[i] != '\0'; i++) {
            align_value[i] = tolower(align_value[i]);
        }
        if (strcmp(align_value, "center") == 0) {
            return true;
        }
    }
    
    // 检查 style="text-align: center" 或 style='text-align: center'
    char style_value[256];
    if (get_html_attribute(tag_text, "style", style_value, sizeof(style_value))) {
        // 转换为小写查找
        for (int i = 0; style_value[i] != '\0'; i++) {
            style_value[i] = tolower(style_value[i]);
        }
        if (strstr(style_value, "text-align") != NULL && 
            strstr(style_value, "center") != NULL) {
            return true;
        }
    }
    
    return false;
}

// 处理 HTML 实体
void decode_html_entities(const char *input, char *output, int max_len) {
    if (input == NULL || output == NULL || max_len <= 0) {
        return;
    }
    
    int len = strlen(input);
    int out_pos = 0;
    int i = 0;
    
    while (i < len && out_pos < max_len - 1) {
        if (input[i] == '&') {
            // 处理 HTML 实体
            if (i + 3 < len && strncmp(input + i, "&lt;", 4) == 0) {
                output[out_pos++] = '<';
                i += 4;
            } else if (i + 3 < len && strncmp(input + i, "&gt;", 4) == 0) {
                output[out_pos++] = '>';
                i += 4;
            } else if (i + 4 < len && strncmp(input + i, "&amp;", 5) == 0) {
                output[out_pos++] = '&';
                i += 5;
            } else if (i + 5 < len && strncmp(input + i, "&quot;", 6) == 0) {
                output[out_pos++] = '"';
                i += 6;
            } else if (i + 5 < len && strncmp(input + i, "&#39;", 5) == 0) {
                output[out_pos++] = '\'';
                i += 5;
            } else if (i + 5 < len && strncmp(input + i, "&apos;", 6) == 0) {
                output[out_pos++] = '\'';
                i += 6;
            } else if (i + 5 < len && strncmp(input + i, "&nbsp;", 6) == 0) {
                output[out_pos++] = ' ';
                i += 6;
            } else {
                // 未识别的实体，保留原样
                output[out_pos++] = input[i++];
            }
        } else {
            output[out_pos++] = input[i++];
        }
    }
    
    output[out_pos] = '\0';
}

// 处理单个 HTML 标签，返回渲染后的文本
void render_html_tag(const HtmlTag *tag, const char *content, char *output, int max_len, StyleType style) {
    if (tag == NULL || output == NULL || max_len <= 0) {
        if (output && max_len > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    // 解码 HTML 实体
    char decoded_content[MAX_LINE_LENGTH * 2];
    if (content != NULL) {
        decode_html_entities(content, decoded_content, sizeof(decoded_content));
    } else {
        decoded_content[0] = '\0';
    }
    
    const char *color = "";
    const char *reset = COLOR_RESET;
    
    switch (tag->type) {
        case HTML_TAG_STRONG:
        case HTML_TAG_B:
            color = (style == STYLE_LIGHT) ? "\033[30m\033[1m" : "\033[37m\033[1m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_EM:
        case HTML_TAG_I:
            color = (style == STYLE_LIGHT) ? "\033[30m\033[3m" : "\033[37m\033[3m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_U:
            color = (style == STYLE_LIGHT) ? "\033[30m\033[4m" : "\033[37m\033[4m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_S:
        case HTML_TAG_DEL:
            color = (style == STYLE_LIGHT) ? "\033[30m\033[9m" : "\033[37m\033[9m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_CODE:
            color = (style == STYLE_LIGHT) ? 
                "\033[38;5;88m\033[48;5;253m" : "\033[38;5;221m\033[48;5;236m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H1:
            color = (style == STYLE_LIGHT) ? "\033[38;5;25m\033[1m" : "\033[38;5;81m\033[1m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H2:
            color = (style == STYLE_LIGHT) ? "\033[38;5;25m" : "\033[38;5;81m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H3:
            color = (style == STYLE_LIGHT) ? "\033[38;5;90m\033[1m" : "\033[38;5;111m\033[1m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H4:
            color = (style == STYLE_LIGHT) ? "\033[38;5;90m" : "\033[38;5;111m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H5:
            color = (style == STYLE_LIGHT) ? "\033[38;5;88m\033[1m" : "\033[38;5;177m\033[1m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_H6:
            color = (style == STYLE_LIGHT) ? "\033[38;5;88m" : "\033[38;5;177m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_A: {
            // 链接标签，显示内容（带下划线）
            color = (style == STYLE_LIGHT) ? "\033[38;5;25m\033[4m" : "\033[38;5;81m\033[4m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
        }
            
        case HTML_TAG_IMG: {
            // 图片标签，显示内容（在 render_html_content 中会提取属性）
            const char *label_color = (style == STYLE_LIGHT) ? 
                "\033[38;5;101m" : "\033[38;5;244m";
            
            if (strlen(decoded_content) > 0) {
                snprintf(output, max_len, "%sImage: %s%s%s", 
                        label_color, reset, decoded_content, reset);
            } else {
                snprintf(output, max_len, "%sImage%s", label_color, reset);
            }
            break;
        }
            
        case HTML_TAG_MARK:
        case HTML_TAG_INS:
            color = (style == STYLE_LIGHT) ? "\033[38;5;28m" : "\033[38;5;149m";
            snprintf(output, max_len, "%s%s%s", color, decoded_content, reset);
            break;
            
        case HTML_TAG_BR:
            snprintf(output, max_len, "\n");
            break;
            
        case HTML_TAG_HR:
            // 水平线在渲染器中单独处理
            if (max_len > 0) {
                output[0] = '\0';
            }
            break;
            
        default:
            // 其他标签，直接输出内容
            snprintf(output, max_len, "%s", decoded_content);
            break;
    }
}

// 渲染 HTML 内容（将 HTML 转换为带 ANSI 颜色的文本）
void render_html_content(const char *html, char *output, int max_len, StyleType style) {
    if (html == NULL || output == NULL || max_len <= 0) {
        if (output && max_len > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    int len = strlen(html);
    int out_pos = 0;
    int i = 0;
    
    while (i < len && out_pos < max_len - 100) {
        // 查找 HTML 标签
        HtmlTag tag;
        if (parse_html_tag(html, i, &tag)) {
            // 处理标签
            if (tag.is_closing) {
                // 闭合标签，直接跳过
                i = tag.end_pos;
                continue;
            }
            
            // 自闭合标签或空标签（如 <br/>, <hr/>, <img/>）
            if (tag.is_self_closing || 
                tag.type == HTML_TAG_BR || 
                tag.type == HTML_TAG_HR ||
                tag.type == HTML_TAG_IMG) {
                char tag_output[MAX_LINE_LENGTH * 2];
                // 对于 img 标签，尝试提取属性
                if (tag.type == HTML_TAG_IMG) {
                    char alt[256] = {0};
                    char src[512] = {0};
                    char tag_text[512];
                    int tag_text_len = tag.end_pos - tag.start_pos;
                    if (tag_text_len < 512) {
                        strncpy(tag_text, html + tag.start_pos, tag_text_len);
                        tag_text[tag_text_len] = '\0';
                        get_html_attribute(tag_text, "alt", alt, sizeof(alt));
                        get_html_attribute(tag_text, "src", src, sizeof(src));
                    }
                    const char *label_color = (style == STYLE_LIGHT) ? 
                        "\033[38;5;101m" : "\033[38;5;244m";
                    if (strlen(alt) > 0) {
                        snprintf(tag_output, sizeof(tag_output), 
                                "%sImage: %s%s%s", label_color, alt, COLOR_RESET, COLOR_RESET);
                    } else if (strlen(src) > 0) {
                        snprintf(tag_output, sizeof(tag_output), 
                                "%sImage: %s%s%s", label_color, src, COLOR_RESET, COLOR_RESET);
                    } else {
                        snprintf(tag_output, sizeof(tag_output), "%sImage%s", label_color, COLOR_RESET);
                    }
                } else {
                    render_html_tag(&tag, NULL, tag_output, sizeof(tag_output), style);
                }
                int tag_len = strlen(tag_output);
                if (out_pos + tag_len < max_len - 1) {
                    strcpy(output + out_pos, tag_output);
                    out_pos += tag_len;
                }
                i = tag.end_pos;
                continue;
            }
            
            // 查找对应的闭合标签
            int content_start = tag.end_pos;
            int content_end = content_start;
            int depth = 1;
            int search_pos = content_start;
            
            // 查找匹配的闭合标签
            while (depth > 0 && search_pos < len) {
                HtmlTag next_tag;
                if (parse_html_tag(html, search_pos, &next_tag)) {
                    if (strcmp(next_tag.name, tag.name) == 0) {
                        if (next_tag.is_closing) {
                            depth--;
                            if (depth == 0) {
                                content_end = next_tag.start_pos;
                                break;
                            }
                        } else {
                            depth++;
                        }
                    }
                    search_pos = next_tag.end_pos;
                } else {
                    search_pos++;
                }
            }
            
            // 提取标签内容
            int content_len = content_end - content_start;
            if (content_len > 0 && content_len < MAX_LINE_LENGTH) {
                char content[MAX_LINE_LENGTH];
                strncpy(content, html + content_start, content_len);
                content[content_len] = '\0';
                
                // 递归处理嵌套的 HTML
                char rendered_content[MAX_LINE_LENGTH * 2];
                render_html_content(content, rendered_content, sizeof(rendered_content), style);
                
                // 渲染标签
                char tag_output[MAX_LINE_LENGTH * 2];
                render_html_tag(&tag, rendered_content, tag_output, sizeof(tag_output), style);
                
                // 追加到输出
                int tag_len = strlen(tag_output);
                if (out_pos + tag_len < max_len - 1) {
                    strcpy(output + out_pos, tag_output);
                    out_pos += tag_len;
                }
                
                // 跳过到闭合标签之后
                if (depth == 0) {
                    HtmlTag closing_tag;
                    if (parse_html_tag(html, content_end, &closing_tag)) {
                        i = closing_tag.end_pos;
                    } else {
                        i = content_end;
                    }
                } else {
                    // 未找到闭合标签，只处理到标签结束
                    i = tag.end_pos;
                }
            } else {
                // 空标签，直接跳过
                i = tag.end_pos;
            }
        } else {
            // 普通文本，直接复制
            if (out_pos < max_len - 1) {
                output[out_pos++] = html[i++];
            } else {
                break;
            }
        }
    }
    
    output[out_pos] = '\0';
}

