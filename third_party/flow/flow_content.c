#include "flow_content.h"
#include "flow.h"
#include "../include/common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 读取文件内容
static char* read_file_content_local(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "%s错误: 无法打开文件: %s%s\n", COLOR_RED, filename, COLOR_RESET);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存
    char *content = (char*)malloc(size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }
    
    // 读取内容
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    
    fclose(file);
    return content;
}

// 读取标准输入内容
static char* read_stdin_content_local(void) {
    char *content = NULL;
    size_t size = 0;
    size_t capacity = 4096;
    
    content = (char*)malloc(capacity);
    if (content == NULL) {
        return NULL;
    }
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (size + len + 1 > capacity) {
            capacity *= 2;
            char *new_content = (char *)realloc(content, capacity);
            if (new_content == NULL) {
                free(content);
                return NULL;
            }
            content = new_content;
        }
        strcpy(content + size, buffer);
        size += len;
    }
    
    content[size] = '\0';
    return content;
}

// 读取并处理内容（统一入口）
ContentInfo* read_and_process_content(FlowOptions *opts) {
    if (opts == NULL) {
        return NULL;
    }
    
    ContentInfo *info = (ContentInfo *)calloc(1, sizeof(ContentInfo));
    if (info == NULL) {
        return NULL;
    }
    
    char *content = NULL;
    
    // 根据选项判断内容来源
    if (opts->from_stdin) {
        // 从标准输入读取
        info->source = CONTENT_SOURCE_STDIN;
        info->filename = "stdin";
        content = read_stdin_content_local();
    } else if (strlen(opts->filename) > 0) {
        // 检查是否为 URL
        if (is_url(opts->filename)) {
            // 从 URL 读取
            info->source = CONTENT_SOURCE_URL;
            info->filename = opts->filename;
            content = read_markdown_from_url(opts->filename);
            // 从 URL 判断是否为代码文件
            info->is_code_file = !is_markdown_file(opts->filename);
            info->file_ext = get_file_extension(opts->filename);
        } else {
            // 从本地文件读取
            info->source = CONTENT_SOURCE_FILE;
            info->filename = opts->filename;
            content = read_file_content_local(opts->filename);
            info->is_code_file = !is_markdown_file(opts->filename);
            info->file_ext = get_file_extension(opts->filename);
        }
    } else {
        // 没有指定来源，尝试从标准输入读取
        if (isatty(STDIN_FILENO) == 0) {
            info->source = CONTENT_SOURCE_STDIN;
            info->filename = "stdin";
            content = read_stdin_content_local();
        } else {
            // 无法确定来源
            free(info);
            return NULL;
        }
    }
    
    if (content == NULL) {
        free(info);
        return NULL;
    }
    
    // 移除前端内容（frontmatter）
    char *processed_content = remove_frontmatter(content);
    free(content);
    content = processed_content;
    
    if (content == NULL) {
        free(info);
        return NULL;
    }
    
    // 如果是代码文件，包装成代码块
    if (info->is_code_file && content != NULL) {
        char *wrapped = wrap_code_block(content, info->file_ext);
        if (wrapped != NULL) {
            free(content);
            content = wrapped;
        }
    }
    
    info->content = content;
    return info;
}

// 释放 ContentInfo
void free_content_info(ContentInfo *info) {
    if (info == NULL) {
        return;
    }
    
    if (info->content != NULL) {
        free(info->content);
        info->content = NULL;
    }
    
    free(info);
}

