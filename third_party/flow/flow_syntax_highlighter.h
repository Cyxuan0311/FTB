#ifndef FLOW_SYNTAX_HIGHLIGHTER_H
#define FLOW_SYNTAX_HIGHLIGHTER_H

#include "flow.h"
#include <stdbool.h>

// 语法高亮类型
typedef enum {
    SYNTAX_NONE,        // 无高亮
    SYNTAX_C,           // C 语言
    SYNTAX_CPP,         // C++
    SYNTAX_PYTHON,      // Python
    SYNTAX_JAVASCRIPT,  // JavaScript
    SYNTAX_TYPESCRIPT,  // TypeScript
    SYNTAX_BASH,        // Bash/Shell
    SYNTAX_GO,          // Go
    SYNTAX_RUST,        // Rust
    SYNTAX_JAVA,        // Java
    SYNTAX_JSON,        // JSON
    SYNTAX_XML,         // XML/HTML
    SYNTAX_MARKDOWN,    // Markdown
    SYNTAX_SQL,         // SQL
    SYNTAX_YAML,        // YAML
    SYNTAX_MAKEFILE,    // Makefile
    SYNTAX_DOCKERFILE,  // Dockerfile
    SYNTAX_CMAKE        // CMake
} SyntaxType;

// 语法元素类型
typedef enum {
    TOKEN_KEYWORD,      // 关键字
    TOKEN_STRING,       // 字符串
    TOKEN_COMMENT,      // 注释
    TOKEN_NUMBER,       // 数字
    TOKEN_FUNCTION,     // 函数名
    TOKEN_TYPE,         // 类型名
    TOKEN_PREPROCESSOR, // 预处理器指令
    TOKEN_OPERATOR,     // 运算符
    TOKEN_PUNCTUATION,  // 标点符号
    TOKEN_NORMAL        // 普通文本
} TokenType;

// 从语言名称获取语法类型
SyntaxType detect_syntax_type(const char *language);

// 对代码行进行语法高亮
void highlight_code_line(const char *line, char *output, size_t max_len, 
                        SyntaxType syntax, StyleType style);

// 高亮代码块内容
void highlight_code_block(const char *code, char *output, size_t max_len,
                         SyntaxType syntax, StyleType style);

#endif // FLOW_SYNTAX_HIGHLIGHTER_H

