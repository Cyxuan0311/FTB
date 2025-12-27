#ifndef FLOW_CONTENT_H
#define FLOW_CONTENT_H

#include "flow.h"

// 内容来源类型
typedef enum {
    CONTENT_SOURCE_FILE,    // 本地文件
    CONTENT_SOURCE_URL,     // URL
    CONTENT_SOURCE_STDIN    // 标准输入
} ContentSource;

// 内容信息结构
typedef struct {
    char *content;          // 内容数据
    ContentSource source;   // 内容来源
    const char *filename;   // 文件名或 URL（用于显示）
    bool is_code_file;      // 是否为代码文件
    const char *file_ext;   // 文件扩展名
} ContentInfo;

// 读取并处理内容（统一入口）
// 根据 FlowOptions 自动判断来源（文件/URL/stdin）
// 返回 ContentInfo，调用者需要释放 content 字段
ContentInfo* read_and_process_content(FlowOptions *opts);

// 释放 ContentInfo
void free_content_info(ContentInfo *info);

#endif // FLOW_CONTENT_H

