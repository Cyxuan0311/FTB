#include "flow.h"
#include "../include/common.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// 不区分大小写的字符串比较
static int strcasecmp_local(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) {
            return diff;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

// 移除前端内容（YAML frontmatter）
// 参考 glow 的 RemoveFrontmatter 函数
char* remove_frontmatter(const char *content) {
    if (content == NULL) {
        return NULL;
    }
    
    // 检查是否以 --- 开头
    if (strncmp(content, "---", 3) != 0) {
        // 没有 frontmatter，返回原内容
        return strdup(content);
    }
    
    // 查找第二个 ---
    const char *p = content + 3;
    // 跳过第一个换行
    while (*p == '\r' || *p == '\n') p++;
    
    // 查找第二个 ---
    const char *second_dash = strstr(p, "---");
    if (second_dash == NULL) {
        // 没有找到第二个 ---，返回原内容
        return strdup(content);
    }
    
    // 跳过第二个 --- 和后面的换行
    const char *content_start = second_dash + 3;
    while (*content_start == '\r' || *content_start == '\n' || *content_start == ' ') {
        content_start++;
    }
    
    return strdup(content_start);
}

// 检查是否为 Markdown 文件
bool is_markdown_file(const char *filename) {
    if (filename == NULL) {
        return true;  // 默认假设是 Markdown
    }
    
    const char *extensions[] = {
        ".md", ".mdown", ".mkdn", ".mkd", ".markdown", NULL
    };
    
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return true;  // 没有扩展名，假设是 Markdown
    }
    
    for (int i = 0; extensions[i] != NULL; i++) {
        if (strcasecmp_local(ext, extensions[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// 包装代码块（用于非 Markdown 文件）
char* wrap_code_block(const char *content, const char *language) {
    if (content == NULL) {
        return NULL;
    }
    
    size_t content_len = strlen(content);
    size_t lang_len = language ? strlen(language) : 0;
    size_t total_len = 3 + lang_len + 1 + content_len + 3 + 1;  // ```lang\ncontent```
    
    char *wrapped = (char *)malloc(total_len);
    if (wrapped == NULL) {
        return NULL;
    }
    
    if (language && lang_len > 0) {
        snprintf(wrapped, total_len, "```%s\n%s```", language, content);
    } else {
        snprintf(wrapped, total_len, "```\n%s```", content);
    }
    
    return wrapped;
}

// 提取文件扩展名
const char* get_file_extension(const char *filename) {
    if (filename == NULL) {
        return "";
    }
    
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return "";
    }
    
    return ext;
}

