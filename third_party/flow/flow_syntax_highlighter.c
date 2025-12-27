#include "flow_syntax_highlighter.h"
#include "../include/common.h"
#include <string.h>
#include <ctype.h>
#include <stddef.h>

// 深色主题颜色
#define DARK_KEYWORD      "\033[38;5;111m"      // 蓝色 - 关键字
#define DARK_STRING       "\033[38;5;149m"      // 绿色 - 字符串
#define DARK_COMMENT      "\033[38;5;101m"      // 灰色 - 注释
#define DARK_NUMBER        "\033[38;5;177m"      // 紫色 - 数字
#define DARK_FUNCTION      "\033[38;5;221m"      // 黄色 - 函数
#define DARK_TYPE          "\033[38;5;81m"       // 青色 - 类型
#define DARK_PREPROCESSOR  "\033[38;5;196m"      // 红色 - 预处理器
#define DARK_OPERATOR      "\033[38;5;244m"       // 灰色 - 运算符
#define DARK_NORMAL        "\033[38;5;252m"      // 浅灰 - 普通文本

// 浅色主题颜色
#define LIGHT_KEYWORD      "\033[38;5;25m"       // 深蓝 - 关键字
#define LIGHT_STRING       "\033[38;5;28m"       // 深绿 - 字符串
#define LIGHT_COMMENT      "\033[38;5;101m"      // 灰色 - 注释
#define LIGHT_NUMBER       "\033[38;5;88m"       // 深红 - 数字
#define LIGHT_FUNCTION     "\033[38;5;94m"       // 棕色 - 函数
#define LIGHT_TYPE         "\033[38;5;25m"       // 深蓝 - 类型
#define LIGHT_PREPROCESSOR "\033[38;5;124m"      // 深红 - 预处理器
#define LIGHT_OPERATOR     "\033[38;5;240m"      // 深灰 - 运算符
#define LIGHT_NORMAL       "\033[30m"            // 黑色 - 普通文本

// 获取语法元素的颜色
static const char* get_token_color(TokenType token, StyleType style) {
    if (style == STYLE_LIGHT) {
        switch (token) {
            case TOKEN_KEYWORD: return LIGHT_KEYWORD;
            case TOKEN_STRING: return LIGHT_STRING;
            case TOKEN_COMMENT: return LIGHT_COMMENT;
            case TOKEN_NUMBER: return LIGHT_NUMBER;
            case TOKEN_FUNCTION: return LIGHT_FUNCTION;
            case TOKEN_TYPE: return LIGHT_TYPE;
            case TOKEN_PREPROCESSOR: return LIGHT_PREPROCESSOR;
            case TOKEN_OPERATOR: return LIGHT_OPERATOR;
            default: return LIGHT_NORMAL;
        }
    } else {
        switch (token) {
            case TOKEN_KEYWORD: return DARK_KEYWORD;
            case TOKEN_STRING: return DARK_STRING;
            case TOKEN_COMMENT: return DARK_COMMENT;
            case TOKEN_NUMBER: return DARK_NUMBER;
            case TOKEN_FUNCTION: return DARK_FUNCTION;
            case TOKEN_TYPE: return DARK_TYPE;
            case TOKEN_PREPROCESSOR: return DARK_PREPROCESSOR;
            case TOKEN_OPERATOR: return DARK_OPERATOR;
            default: return DARK_NORMAL;
        }
    }
}

// C/C++ 关键字列表
static const char* c_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
    "inline", "restrict", "_Bool", "_Complex", "_Imaginary",
    // C++ 关键字
    "class", "namespace", "template", "typename", "using", "virtual",
    "public", "private", "protected", "friend", "operator", "this",
    "new", "delete", "try", "catch", "throw", "const_cast", "dynamic_cast",
    "static_cast", "reinterpret_cast", "true", "false", "nullptr",
    NULL
};

// Python 关键字列表
static const char* python_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del",
    "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass",
    "print", "raise", "return", "try", "while", "with", "yield",
    "True", "False", "None", "async", "await", "nonlocal",
    NULL
};

// JavaScript 关键字列表
static const char* js_keywords[] = {
    "break", "case", "catch", "class", "const", "continue", "debugger", "default",
    "delete", "do", "else", "export", "extends", "finally", "for", "function",
    "if", "import", "in", "instanceof", "new", "return", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
    "let", "static", "true", "false", "null", "undefined", "async", "await",
    NULL
};

// Go 关键字列表
static const char* go_keywords[] = {
    "break", "case", "chan", "const", "continue", "default", "defer", "else",
    "fallthrough", "for", "func", "go", "goto", "if", "import", "interface",
    "map", "package", "range", "return", "select", "struct", "switch", "type",
    "var", "true", "false", "nil", "iota",
    NULL
};

// Rust 关键字列表
static const char* rust_keywords[] = {
    "as", "break", "const", "continue", "crate", "else", "enum", "extern",
    "false", "fn", "for", "if", "impl", "in", "let", "loop", "match", "mod",
    "move", "mut", "pub", "ref", "return", "self", "static", "struct",
    "super", "trait", "true", "type", "unsafe", "use", "where", "while",
    "async", "await", "dyn", "abstract", "become", "box", "do", "final",
    "macro", "override", "priv", "typeof", "unsized", "virtual", "yield",
    NULL
};

// Java 关键字列表
static const char* java_keywords[] = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "default", "do", "double", "else", "enum",
    "extends", "final", "finally", "float", "for", "goto", "if", "implements",
    "import", "instanceof", "int", "interface", "long", "native", "new", "package",
    "private", "protected", "public", "return", "short", "static", "strictfp",
    "super", "switch", "synchronized", "this", "throw", "throws", "transient",
    "try", "void", "volatile", "while", "true", "false", "null",
    NULL
};

// Bash 关键字列表
static const char* bash_keywords[] = {
    "if", "then", "else", "elif", "fi", "case", "esac", "for", "select",
    "while", "until", "do", "done", "in", "function", "time", "coproc",
    "!", "[[", "]]", "true", "false", "break", "continue", "return",
    NULL
};

// SQL 关键字列表
static const char* sql_keywords[] = {
    "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "UPDATE", "SET",
    "DELETE", "CREATE", "DROP", "ALTER", "TABLE", "INDEX", "VIEW", "DATABASE",
    "AND", "OR", "NOT", "IN", "LIKE", "BETWEEN", "IS", "NULL", "ORDER", "BY",
    "GROUP", "HAVING", "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER",
    "ON", "AS", "DISTINCT", "COUNT", "SUM", "AVG", "MAX", "MIN", "UNION",
    "ALL", "EXISTS", "CASE", "WHEN", "THEN", "ELSE", "END",
    NULL
};

// 检查是否为关键字
static bool is_keyword(const char *word, const char **keywords) {
    if (keywords == NULL) return false;
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

// 获取语言关键字列表
static const char** get_keywords(SyntaxType syntax) {
    switch (syntax) {
        case SYNTAX_C:
        case SYNTAX_CPP:
            return c_keywords;
        case SYNTAX_PYTHON:
            return python_keywords;
        case SYNTAX_JAVASCRIPT:
        case SYNTAX_TYPESCRIPT:
            return js_keywords;
        case SYNTAX_GO:
            return go_keywords;
        case SYNTAX_RUST:
            return rust_keywords;
        case SYNTAX_JAVA:
            return java_keywords;
        case SYNTAX_BASH:
            return bash_keywords;
        case SYNTAX_SQL:
            return sql_keywords;
        default:
            return NULL;
    }
}

// 检查字符是否为标识符字符
static bool is_identifier_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || c == '_' || c == '$';
}

// 检查是否为数字（包括十六进制、浮点数）
static bool is_number_start(const char *p) {
    if (isdigit(*p)) return true;
    // 检查十六进制 0x, 0X
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) return true;
    // 检查浮点数 .123
    if (*p == '.' && isdigit(p[1])) return true;
    return false;
}

// 解析数字
static const char* parse_number(const char *p, char *output, size_t *out_pos, size_t max_len, StyleType style) {
    const char *color = get_token_color(TOKEN_NUMBER, style);
    size_t color_len = strlen(color);
    
    // 添加颜色开始
    if (*out_pos + color_len < max_len - 1) {
        strcpy(output + *out_pos, color);
        *out_pos += color_len;
    }
    
    // 处理十六进制
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
        while (isxdigit(*p) || *p == 'x' || *p == 'X') {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
        }
    } else {
        // 处理普通数字和浮点数
        while (isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' || 
               *p == '+' || *p == '-' || *p == 'f' || *p == 'F' || 
               *p == 'l' || *p == 'L' || *p == 'u' || *p == 'U') {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
        }
    }
    
    // 添加颜色结束
    size_t reset_len = strlen(COLOR_RESET);
    if (*out_pos + reset_len < max_len - 1) {
        strcpy(output + *out_pos, COLOR_RESET);
        *out_pos += reset_len;
    }
    
    return p;
}

// 解析字符串
static const char* parse_string(const char *p, char *output, size_t *out_pos, size_t max_len, 
                               StyleType style, SyntaxType syntax) {
    (void)syntax;  // 未使用参数，保留用于未来扩展
    char quote = *p;
    const char *color = get_token_color(TOKEN_STRING, style);
    
    // 添加颜色开始
    size_t color_len = strlen(color);
    if (*out_pos + color_len < max_len - 1) {
        strcpy(output + *out_pos, color);
        *out_pos += color_len;
    }
    
    // 添加引号
    if (*out_pos < max_len - 1) {
        output[(*out_pos)++] = *p;
    }
    p++;
    
    bool escaped = false;
    while (*p != '\0') {
        if (escaped) {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
            escaped = false;
        } else if (*p == '\\') {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
            escaped = true;
        } else if (*p == quote) {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
            break;
        } else {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
        }
    }
    
    // 添加颜色结束
    size_t reset_len = strlen(COLOR_RESET);
    if (*out_pos + reset_len < max_len - 1) {
        strcpy(output + *out_pos, COLOR_RESET);
        *out_pos += reset_len;
    }
    
    return p;
}

// 解析注释
static const char* parse_comment(const char *p, char *output, size_t *out_pos, size_t max_len, 
                                StyleType style, SyntaxType syntax) {
    const char *color = get_token_color(TOKEN_COMMENT, style);
    size_t color_len = strlen(color);
    
    // 添加颜色开始
    if (*out_pos + color_len < max_len - 1) {
        strcpy(output + *out_pos, color);
        *out_pos += color_len;
    }
    
    if (syntax == SYNTAX_PYTHON || syntax == SYNTAX_BASH || syntax == SYNTAX_YAML) {
        // Python/Bash 风格注释 #
        while (*p != '\0' && *p != '\n') {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
        }
    } else if (syntax == SYNTAX_SQL) {
        // SQL 风格注释 --
        while (*p != '\0' && *p != '\n') {
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p;
            }
            p++;
        }
    } else {
        // C 风格注释 // 或 /* */
        if (p[1] == '/') {
            // 单行注释
            while (*p != '\0' && *p != '\n') {
                if (*out_pos < max_len - 1) {
                    output[(*out_pos)++] = *p;
                }
                p++;
            }
        } else if (p[1] == '*') {
            // 多行注释
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p; // /
            }
            p++;
            if (*out_pos < max_len - 1) {
                output[(*out_pos)++] = *p; // *
            }
            p++;
            while (*p != '\0') {
                if (*p == '*' && p[1] == '/') {
                    if (*out_pos < max_len - 1) {
                        output[(*out_pos)++] = *p;
                    }
                    p++;
                    if (*out_pos < max_len - 1) {
                        output[(*out_pos)++] = *p;
                    }
                    p++;
                    break;
                }
                if (*out_pos < max_len - 1) {
                    output[(*out_pos)++] = *p;
                }
                p++;
            }
        }
    }
    
    // 添加颜色结束
    size_t reset_len = strlen(COLOR_RESET);
    if (*out_pos + reset_len < max_len - 1) {
        strcpy(output + *out_pos, COLOR_RESET);
        *out_pos += reset_len;
    }
    
    return p;
}

// 解析标识符（可能是关键字、函数名等）
static const char* parse_identifier(const char *p, char *output, size_t *out_pos, size_t max_len,
                                   StyleType style, SyntaxType syntax) {
    char word[256] = {0};
    size_t word_pos = 0;
    
    // 提取标识符
    while (is_identifier_char(*p) && word_pos < sizeof(word) - 1) {
        word[word_pos++] = *p++;
    }
    word[word_pos] = '\0';
    
    const char **keywords = get_keywords(syntax);
    TokenType token_type = TOKEN_NORMAL;
    
    // 检查是否为关键字
    if (is_keyword(word, keywords)) {
        token_type = TOKEN_KEYWORD;
    } else if (syntax == SYNTAX_C || syntax == SYNTAX_CPP) {
        // C/C++ 中检查是否为类型
        const char* types[] = {"int", "char", "float", "double", "void", "long", "short",
                               "signed", "unsigned", "size_t", "ssize_t", "FILE", "NULL",
                               NULL};
        if (is_keyword(word, types)) {
            token_type = TOKEN_TYPE;
        }
    }
    
    const char *color = get_token_color(token_type, style);
    size_t color_len = strlen(color);
    
    // 添加颜色
    if (*out_pos + color_len < max_len - 1) {
        strcpy(output + *out_pos, color);
        *out_pos += color_len;
    }
    
    // 添加标识符
    for (const char *w = word; *w != '\0' && *out_pos < max_len - 1; w++) {
        output[(*out_pos)++] = *w;
    }
    
    // 添加颜色结束
    size_t reset_len = strlen(COLOR_RESET);
    if (*out_pos + reset_len < max_len - 1) {
        strcpy(output + *out_pos, COLOR_RESET);
        *out_pos += reset_len;
    }
    
    return p;
}

// 从语言名称检测语法类型
SyntaxType detect_syntax_type(const char *language) {
    if (language == NULL || *language == '\0') {
        return SYNTAX_NONE;
    }
    
    // 转换为小写进行比较
    char lang_lower[64] = {0};
    for (int i = 0; language[i] != '\0' && i < 63; i++) {
        lang_lower[i] = tolower(language[i]);
    }
    
    if (strcmp(lang_lower, "c") == 0) return SYNTAX_C;
    if (strcmp(lang_lower, "cpp") == 0 || strcmp(lang_lower, "c++") == 0 || 
        strcmp(lang_lower, "cxx") == 0) return SYNTAX_CPP;
    if (strcmp(lang_lower, "py") == 0 || strcmp(lang_lower, "python") == 0) return SYNTAX_PYTHON;
    if (strcmp(lang_lower, "js") == 0 || strcmp(lang_lower, "javascript") == 0) return SYNTAX_JAVASCRIPT;
    if (strcmp(lang_lower, "ts") == 0 || strcmp(lang_lower, "typescript") == 0) return SYNTAX_TYPESCRIPT;
    if (strcmp(lang_lower, "sh") == 0 || strcmp(lang_lower, "bash") == 0 || 
        strcmp(lang_lower, "shell") == 0 || strcmp(lang_lower, "zsh") == 0) return SYNTAX_BASH;
    if (strcmp(lang_lower, "go") == 0 || strcmp(lang_lower, "golang") == 0) return SYNTAX_GO;
    if (strcmp(lang_lower, "rs") == 0 || strcmp(lang_lower, "rust") == 0) return SYNTAX_RUST;
    if (strcmp(lang_lower, "java") == 0) return SYNTAX_JAVA;
    if (strcmp(lang_lower, "json") == 0) return SYNTAX_JSON;
    if (strcmp(lang_lower, "xml") == 0 || strcmp(lang_lower, "html") == 0) return SYNTAX_XML;
    if (strcmp(lang_lower, "md") == 0 || strcmp(lang_lower, "markdown") == 0) return SYNTAX_MARKDOWN;
    if (strcmp(lang_lower, "sql") == 0) return SYNTAX_SQL;
    if (strcmp(lang_lower, "yaml") == 0 || strcmp(lang_lower, "yml") == 0) return SYNTAX_YAML;
    if (strcmp(lang_lower, "makefile") == 0 || strcmp(lang_lower, "make") == 0) return SYNTAX_MAKEFILE;
    if (strcmp(lang_lower, "dockerfile") == 0 || strcmp(lang_lower, "docker") == 0) return SYNTAX_DOCKERFILE;
    if (strcmp(lang_lower, "cmake") == 0) return SYNTAX_CMAKE;
    
    return SYNTAX_NONE;
}

// 对代码行进行语法高亮
void highlight_code_line(const char *line, char *output, size_t max_len,
                        SyntaxType syntax, StyleType style) {
    if (line == NULL || output == NULL || max_len == 0) {
        return;
    }
    
    // 如果没有语法类型，直接复制
    if (syntax == SYNTAX_NONE) {
        strncpy(output, line, max_len - 1);
        output[max_len - 1] = '\0';
        return;
    }
    
    size_t out_pos = 0;
    const char *p = line;
    
    while (*p != '\0' && *p != '\n' && out_pos < max_len - 1) {
        // 跳过空白字符
        if (isspace(*p)) {
            output[out_pos++] = *p++;
            continue;
        }
        
        // 检查预处理器指令（C/C++）
        if ((syntax == SYNTAX_C || syntax == SYNTAX_CPP) && *p == '#') {
            const char *color = get_token_color(TOKEN_PREPROCESSOR, style);
            size_t color_len = strlen(color);
            if (out_pos + color_len < max_len - 1) {
                strcpy(output + out_pos, color);
                out_pos += color_len;
            }
            while (*p != '\0' && *p != '\n' && out_pos < max_len - 1) {
                output[out_pos++] = *p++;
            }
            size_t reset_len = strlen(COLOR_RESET);
            if (out_pos + reset_len < max_len - 1) {
                strcpy(output + out_pos, COLOR_RESET);
                out_pos += reset_len;
            }
            continue;
        }
        
        // 检查字符串
        if (*p == '"' || *p == '\'') {
            p = parse_string(p, output, &out_pos, max_len, style, syntax);
            continue;
        }
        
        // 检查注释
        if (*p == '#' && (syntax == SYNTAX_PYTHON || syntax == SYNTAX_BASH || syntax == SYNTAX_YAML)) {
            p = parse_comment(p, output, &out_pos, max_len, style, syntax);
            continue;
        }
        if (*p == '/' && (syntax == SYNTAX_C || syntax == SYNTAX_CPP || 
                         syntax == SYNTAX_JAVASCRIPT || syntax == SYNTAX_TYPESCRIPT ||
                         syntax == SYNTAX_JAVA || syntax == SYNTAX_GO || syntax == SYNTAX_RUST)) {
            if (p[1] == '/' || p[1] == '*') {
                p = parse_comment(p, output, &out_pos, max_len, style, syntax);
                continue;
            }
        }
        if (p[0] == '-' && p[1] == '-' && syntax == SYNTAX_SQL) {
            p = parse_comment(p, output, &out_pos, max_len, style, syntax);
            continue;
        }
        
        // 检查数字
        if (is_number_start(p)) {
            p = parse_number(p, output, &out_pos, max_len, style);
            continue;
        }
        
        // 检查标识符
        if (is_identifier_char(*p) && !isdigit(*p)) {
            p = parse_identifier(p, output, &out_pos, max_len, style, syntax);
            continue;
        }
        
        // 其他字符（运算符、标点符号等）
        const char *color = get_token_color(TOKEN_OPERATOR, style);
        size_t color_len = strlen(color);
        if (out_pos + color_len < max_len - 1) {
            strcpy(output + out_pos, color);
            out_pos += color_len;
        }
        output[out_pos++] = *p++;
        size_t reset_len = strlen(COLOR_RESET);
        if (out_pos + reset_len < max_len - 1) {
            strcpy(output + out_pos, COLOR_RESET);
            out_pos += reset_len;
        }
    }
    
    output[out_pos] = '\0';
}

// 高亮代码块内容（处理多行）
void highlight_code_block(const char *code, char *output, size_t max_len,
                         SyntaxType syntax, StyleType style) {
    if (code == NULL || output == NULL || max_len == 0) {
        return;
    }
    
    size_t out_pos = 0;
    const char *p = code;
    char line_buffer[4096];
    char highlighted_line[8192];
    
    while (*p != '\0' && out_pos < max_len - 1) {
        // 提取一行
        size_t line_pos = 0;
        while (*p != '\0' && *p != '\n' && line_pos < sizeof(line_buffer) - 1) {
            line_buffer[line_pos++] = *p++;
        }
        line_buffer[line_pos] = '\0';
        
        // 高亮这一行
        highlight_code_line(line_buffer, highlighted_line, sizeof(highlighted_line), syntax, style);
        
        // 添加到输出
        size_t hl_len = strlen(highlighted_line);
        if (out_pos + hl_len < max_len - 1) {
            strcpy(output + out_pos, highlighted_line);
            out_pos += hl_len;
        }
        
        // 添加换行符
        if (*p == '\n') {
            if (out_pos < max_len - 1) {
                output[out_pos++] = '\n';
            }
            p++;
        }
    }
    
    output[out_pos] = '\0';
}

