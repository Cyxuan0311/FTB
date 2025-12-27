#include "flow.h"
#include "../include/common.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#define MAX_URL_LENGTH 2048
#define GITHUB_RAW_BASE "https://raw.githubusercontent.com"
#define GITLAB_RAW_BASE "https://gitlab.com"

// URL 数据结构
typedef struct {
    char url[MAX_URL_LENGTH];
    bool is_github;
    bool is_gitlab;
    char owner[256];
    char repo[256];
    char path[512];
    char branch[128];      // 分支或标签（如 main, master, v1.0.0）
    char file_path[512];  // 文件路径（如 README.md, docs/guide.md）
} URLInfo;

// 内存缓冲区结构（用于 curl 回调）
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} MemoryBuffer;

// 检查是否为 URL
bool is_url(const char *path) {
    if (path == NULL) return false;
    
    // 检查是否包含协议
    if (strstr(path, "://") != NULL) {
        return true;
    }
    
    // 检查是否为 GitHub/GitLab 格式
    if (strstr(path, "github.com") != NULL || 
        strstr(path, "gitlab.com") != NULL ||
        strstr(path, "github://") != NULL ||
        strstr(path, "gitlab://") != NULL) {
        return true;
    }
    
    // 检查是否为简化的 user/repo 格式（至少包含一个 /）
    if (strchr(path, '/') != NULL) {
        // 检查是否像 URL 路径（不以 . 开头，不是相对路径）
        if (path[0] != '.' && path[0] != '/') {
            // 可能是简化的 GitHub 格式
            return true;
        }
    }
    
    return false;
}

// 解析 GitHub URL
static bool parse_github_url(const char *path, URLInfo *info) {
    memset(info, 0, sizeof(URLInfo));
    
    const char *github_prefix = "github.com/";
    const char *github_proto = "github://";
    const char *github_https = "https://github.com/";
    const char *github_http = "http://github.com/";
    
    const char *repo_path = NULL;
    
    // 处理不同的 GitHub URL 格式
    if (strncmp(path, github_proto, strlen(github_proto)) == 0) {
        repo_path = path + strlen(github_proto);
    } else if (strncmp(path, github_https, strlen(github_https)) == 0) {
        repo_path = path + strlen(github_https);
    } else if (strncmp(path, github_http, strlen(github_http)) == 0) {
        repo_path = path + strlen(github_http);
    } else if (strstr(path, github_prefix) != NULL) {
        repo_path = strstr(path, github_prefix) + strlen(github_prefix);
    } else {
        // 可能是简化的 user/repo 格式
        repo_path = path;
    }
    
    if (repo_path == NULL) return false;
    
    // 复制路径用于解析
    char repo_str[1024];
    strncpy(repo_str, repo_path, sizeof(repo_str) - 1);
    repo_str[sizeof(repo_str) - 1] = '\0';
    
    // 提取 owner 和 repo（第一个 / 之前的部分）
    char *first_slash = strchr(repo_str, '/');
    if (first_slash == NULL) return false;
    
    *first_slash = '\0';
    size_t owner_len = strlen(repo_str);
    if (owner_len >= sizeof(info->owner)) {
        owner_len = sizeof(info->owner) - 1;
    }
    memcpy(info->owner, repo_str, owner_len);
    info->owner[owner_len] = '\0';
    
    // 提取 repo（第二个 / 之前的部分）
    char *second_slash = strchr(first_slash + 1, '/');
    if (second_slash == NULL) {
        // 只有 user/repo，没有路径
        size_t repo_len = strlen(first_slash + 1);
        if (repo_len >= sizeof(info->repo)) {
            repo_len = sizeof(info->repo) - 1;
        }
        memcpy(info->repo, first_slash + 1, repo_len);
        info->repo[repo_len] = '\0';
        
        // 移除 .git 后缀
        size_t repo_name_len = strlen(info->repo);
        if (repo_name_len > 4 && strcmp(info->repo + repo_name_len - 4, ".git") == 0) {
            info->repo[repo_name_len - 4] = '\0';
        }
        
        info->is_github = true;
        // 使用 GitHub API 查找 README
        snprintf(info->url, sizeof(info->url), 
                 "https://api.github.com/repos/%s/%s/readme", 
                 info->owner, info->repo);
        return true;
    }
    
    // 有路径，提取 repo
    *second_slash = '\0';
    size_t repo_len = strlen(first_slash + 1);
    if (repo_len >= sizeof(info->repo)) {
        repo_len = sizeof(info->repo) - 1;
    }
    memcpy(info->repo, first_slash + 1, repo_len);
    info->repo[repo_len] = '\0';
    
    // 移除 .git 后缀
    size_t repo_name_len = strlen(info->repo);
    if (repo_name_len > 4 && strcmp(info->repo + repo_name_len - 4, ".git") == 0) {
        info->repo[repo_name_len - 4] = '\0';
    }
    
    // 检查是否有 /blob/ 或 /tree/ 路径
    char *blob = strstr(second_slash + 1, "/blob/");
    char *tree = strstr(second_slash + 1, "/tree/");
    
    if (blob != NULL) {
        // 提取分支和文件路径
        char *branch_start = blob + 6; // 跳过 "/blob/"
        char *file_start = strchr(branch_start, '/');
        
        if (file_start != NULL) {
            // 提取分支
            size_t branch_len = file_start - branch_start;
            if (branch_len >= sizeof(info->branch)) {
                branch_len = sizeof(info->branch) - 1;
            }
            memcpy(info->branch, branch_start, branch_len);
            info->branch[branch_len] = '\0';
            
            // 提取文件路径
            file_start++; // 跳过 '/'
            size_t file_len = strlen(file_start);
            if (file_len >= sizeof(info->file_path)) {
                file_len = sizeof(info->file_path) - 1;
            }
            memcpy(info->file_path, file_start, file_len);
            info->file_path[file_len] = '\0';
            
            // 构建 raw URL
            snprintf(info->url, sizeof(info->url),
                     "https://raw.githubusercontent.com/%s/%s/%s/%s",
                     info->owner, info->repo, info->branch, info->file_path);
            info->is_github = true;
            return true;
        }
    } else if (tree != NULL) {
        // /tree/ 路径，不支持直接读取目录，返回 false
        return false;
    }
    
    // 没有 /blob/ 或 /tree/，可能是其他格式，尝试使用 API
    info->is_github = true;
    snprintf(info->url, sizeof(info->url), 
             "https://api.github.com/repos/%s/%s/readme", 
             info->owner, info->repo);
    
    return true;
}

// 解析 GitLab URL
static bool parse_gitlab_url(const char *path, URLInfo *info) {
    memset(info, 0, sizeof(URLInfo));
    
    const char *gitlab_prefix = "gitlab.com/";
    const char *gitlab_proto = "gitlab://";
    const char *gitlab_https = "https://gitlab.com/";
    
    const char *repo_path = NULL;
    
    // 处理不同的 GitLab URL 格式
    if (strncmp(path, gitlab_proto, strlen(gitlab_proto)) == 0) {
        repo_path = path + strlen(gitlab_proto);
    } else if (strncmp(path, gitlab_https, strlen(gitlab_https)) == 0) {
        repo_path = path + strlen(gitlab_https);
    } else if (strstr(path, gitlab_prefix) != NULL) {
        repo_path = strstr(path, gitlab_prefix) + strlen(gitlab_prefix);
    } else {
        return false;
    }
    
    if (repo_path == NULL) return false;
    
    // 解析路径（GitLab 可能有多级路径）
    char repo_str[512];
    strncpy(repo_str, repo_path, sizeof(repo_str) - 1);
    repo_str[sizeof(repo_str) - 1] = '\0';
    
    // 移除可能的路径部分
    char *blob = strstr(repo_str, "/-/blob/");
    if (blob != NULL) {
        *blob = '\0';
    }
    
    // 提取最后一个 / 之前的部分作为 repo
    char *last_slash = strrchr(repo_str, '/');
    if (last_slash == NULL) return false;
    
    *last_slash = '\0';
    size_t owner_len = strlen(repo_str);
    if (owner_len >= sizeof(info->owner)) {
        owner_len = sizeof(info->owner) - 1;
    }
    memcpy(info->owner, repo_str, owner_len);
    info->owner[owner_len] = '\0';
    
    size_t repo_len = strlen(last_slash + 1);
    if (repo_len >= sizeof(info->repo)) {
        repo_len = sizeof(info->repo) - 1;
    }
    memcpy(info->repo, last_slash + 1, repo_len);
    info->repo[repo_len] = '\0';
    
    // 移除 .git 后缀
    size_t repo_name_len = strlen(info->repo);
    if (repo_name_len > 4 && strcmp(info->repo + repo_name_len - 4, ".git") == 0) {
        info->repo[repo_name_len - 4] = '\0';
    }
    
    info->is_gitlab = true;
    
    // 使用 GitLab API 查找 README（类似 glow 的实现）
    // API: https://gitlab.com/api/v4/projects/owner%2Frepo
    char project_path[512];
    snprintf(project_path, sizeof(project_path), "%s%%2F%s", info->owner, info->repo);
    snprintf(info->url, sizeof(info->url), 
             "https://gitlab.com/api/v4/projects/%s", 
             project_path);
    
    return true;
}

// 解析通用 URL
static bool parse_generic_url(const char *path, URLInfo *info) {
    memset(info, 0, sizeof(URLInfo));
    
    // 如果没有协议，添加 https://
    if (strstr(path, "://") == NULL) {
        snprintf(info->url, sizeof(info->url), "https://%s", path);
    } else {
        strncpy(info->url, path, sizeof(info->url) - 1);
        info->url[sizeof(info->url) - 1] = '\0';
    }
    
    return true;
}

// 解析 URL
bool parse_url(const char *path, URLInfo *info) {
    if (path == NULL || info == NULL) {
        return false;
    }
    
    // 尝试解析为 GitHub
    if (parse_github_url(path, info)) {
        return true;
    }
    
    // 尝试解析为 GitLab
    if (parse_gitlab_url(path, info)) {
        return true;
    }
    
    // 解析为通用 URL
    return parse_generic_url(path, info);
}

#ifdef HAVE_LIBCURL
// curl 写入回调函数
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    MemoryBuffer *mem = (MemoryBuffer *)userp;
    size_t realsize = size * nmemb;
    
    // 扩展缓冲区
    if (mem->size + realsize + 1 > mem->capacity) {
        size_t new_capacity = mem->capacity * 2;
        if (new_capacity < mem->size + realsize + 1) {
            new_capacity = mem->size + realsize + 1;
        }
        
        char *new_data = (char *)realloc(mem->data, new_capacity);
        if (new_data == NULL) {
            return 0;  // 内存分配失败
        }
        
        mem->data = new_data;
        mem->capacity = new_capacity;
    }
    
    // 复制数据
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    
    return realsize;
}

// 从 URL 读取内容（使用 libcurl）
char* read_url_content(const char *url) {
    CURL *curl;
    CURLcode res;
    MemoryBuffer mem;
    
    // 初始化内存缓冲区
    mem.data = (char *)malloc(4096);
    if (mem.data == NULL) {
        return NULL;
    }
    mem.capacity = 4096;
    mem.size = 0;
    mem.data[0] = '\0';
    
    curl = curl_easy_init();
    if (!curl) {
        free(mem.data);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "flow/1.0");
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "%s错误: 无法从 URL 读取内容: %s%s\n", 
                COLOR_RED, curl_easy_strerror(res), COLOR_RESET);
        free(mem.data);
        curl_easy_cleanup(curl);
        return NULL;
    }
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (http_code != 200) {
        fprintf(stderr, "%s错误: HTTP 状态码 %ld%s\n", 
                COLOR_RED, http_code, COLOR_RESET);
        free(mem.data);
        return NULL;
    }
    
    return mem.data;
}
#else
// 从 URL 读取内容（不使用 libcurl，返回错误）
char* read_url_content(const char *url) {
    (void)url;
    fprintf(stderr, "%s错误: 此版本未编译 libcurl 支持，无法从 URL 读取%s\n", 
            COLOR_RED, COLOR_RESET);
    fprintf(stderr, "请使用文件路径或标准输入\n");
    return NULL;
}
#endif

// 解析 GitHub API JSON 响应，提取 download_url
static char* parse_github_api_response(const char *json_response) {
    // 查找 "download_url" 字段
    const char *download_url_key = "\"download_url\"";
    const char *key_pos = strstr(json_response, download_url_key);
    if (key_pos == NULL) {
        return NULL;
    }
    
    // 查找值（在冒号后面）
    const char *value_start = strchr(key_pos, ':');
    if (value_start == NULL) {
        return NULL;
    }
    value_start++;  // 跳过 ':'
    
    // 跳过空格和引号
    while (*value_start == ' ' || *value_start == '"') {
        value_start++;
    }
    
    // 查找引号结束
    const char *value_end = strchr(value_start, '"');
    if (value_end == NULL) {
        return NULL;
    }
    
    // 提取 URL
    int url_len = value_end - value_start;
    char *url = (char *)malloc(url_len + 1);
    if (url == NULL) {
        return NULL;
    }
    strncpy(url, value_start, url_len);
    url[url_len] = '\0';
    
    return url;
}

// 解析 GitLab API JSON 响应，提取 readme_url
static char* parse_gitlab_api_response(const char *json_response) {
    // 查找 "readme_url" 字段
    const char *readme_url_key = "\"readme_url\"";
    const char *key_pos = strstr(json_response, readme_url_key);
    if (key_pos == NULL) {
        return NULL;
    }
    
    // 查找值（在冒号后面）
    const char *value_start = strchr(key_pos, ':');
    if (value_start == NULL) {
        return NULL;
    }
    value_start++;  // 跳过 ':'
    
    // 跳过空格和引号
    while (*value_start == ' ' || *value_start == '"') {
        value_start++;
    }
    
    // 查找引号结束
    const char *value_end = strchr(value_start, '"');
    if (value_end == NULL) {
        return NULL;
    }
    
    // 提取 URL
    int url_len = value_end - value_start;
    char *url = (char *)malloc(url_len + 1);
    if (url == NULL) {
        return NULL;
    }
    memcpy(url, value_start, url_len);
    url[url_len] = '\0';
    
    // 将 "blob" 替换为 "raw"（GitLab 需要）
    char *blob_pos = strstr(url, "/blob/");
    if (blob_pos != NULL) {
        memmove(blob_pos + 1, blob_pos + 5, strlen(blob_pos + 5) + 1);
        memcpy(blob_pos, "/raw", 4);
    }
    
    return url;
}

// 从 URL 路径读取 Markdown 内容
char* read_markdown_from_url(const char *path) {
    URLInfo info;
    
    if (!parse_url(path, &info)) {
        fprintf(stderr, "%s错误: 无法解析 URL: %s%s\n", 
                COLOR_RED, path, COLOR_RESET);
        return NULL;
    }
    
    // 如果是 GitHub 或 GitLab，处理方式不同
    if (info.is_github) {
        // 如果已经有直接的文件路径（通过 /blob/ 解析），直接读取
        if (strlen(info.file_path) > 0 && strlen(info.branch) > 0) {
            printf("%s正在从 GitHub 读取: %s/%s/%s/%s%s\n", 
                   COLOR_CYAN, info.owner, info.repo, info.branch, info.file_path, COLOR_RESET);
            return read_url_content(info.url);
        }
        
        // 否则使用 API 查找 README
        printf("%s正在从 GitHub 读取: %s/%s%s\n", 
               COLOR_CYAN, info.owner, info.repo, COLOR_RESET);
        
        char *api_response = read_url_content(info.url);
        if (api_response == NULL) {
            fprintf(stderr, "%s提示: 无法从 GitHub API 获取 README，可能仓库不存在或需要认证%s\n", 
                    COLOR_YELLOW, COLOR_RESET);
            return NULL;
        }
        
        char *download_url = parse_github_api_response(api_response);
        free(api_response);
        
        if (download_url == NULL) {
            fprintf(stderr, "%s错误: 无法从 GitHub API 响应中提取下载 URL%s\n", 
                    COLOR_RED, COLOR_RESET);
            fprintf(stderr, "%s提示: 请尝试使用完整路径，如: github.com/user/repo/blob/main/README.md%s\n",
                    COLOR_YELLOW, COLOR_RESET);
            return NULL;
        }
        
        char *content = read_url_content(download_url);
        free(download_url);
        return content;
    } else if (info.is_gitlab) {
        char *api_response = read_url_content(info.url);
        if (api_response == NULL) {
            return NULL;
        }
        
        char *readme_url = parse_gitlab_api_response(api_response);
        free(api_response);
        
        if (readme_url == NULL) {
            fprintf(stderr, "%s错误: 无法从 GitLab API 响应中提取 README URL%s\n", 
                    COLOR_RED, COLOR_RESET);
            return NULL;
        }
        
        char *content = read_url_content(readme_url);
        free(readme_url);
        return content;
    } else {
        // 通用 URL，直接读取
        return read_url_content(info.url);
    }
}

