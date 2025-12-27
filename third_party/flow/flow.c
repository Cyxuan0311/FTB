#include "flow.h"
#include "flow_content.h"
#include "../include/common.h"
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

// 初始化选项
void init_options(FlowOptions *opts) {
    memset(opts, 0, sizeof(FlowOptions));
    opts->width = 0;  // 0 表示自动
    opts->style = STYLE_AUTO;
    opts->use_pager = false;
    opts->from_stdin = false;
    opts->show_line_numbers = false;  // 默认不显示行号
    opts->show_status_bar = false;     // 默认不显示状态栏
}

// 读取文件内容
char* read_file_content(const char *filename) {
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
char* read_stdin_content(void) {
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
            char *new_content = (char*)realloc(content, capacity);
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

// 显示帮助信息
void print_help(const char *program_name) {
    printf("用法: %s [选项] [文件]\n", program_name);
    printf("在终端中预览 Markdown 文件\n\n");
    printf("选项:\n");
    printf("  -s, --style STYLE      样式 (auto/dark/light, 默认: auto)\n");
    printf("  -w, --width WIDTH      输出宽度 (默认: 自动)\n");
    printf("  -p, --pager           使用分页器显示\n");
    printf("  -n, --line-numbers     显示行号\n");
    printf("  --no-status-bar        不显示状态栏\n");
    printf("  -h, --help             显示此帮助信息\n");
    printf("  -v, --version          显示版本信息\n\n");
    printf("参数:\n");
    printf("  文件/URL              要预览的 Markdown 文件或 URL\n");
    printf("  -                      从标准输入读取\n\n");
    printf("URL 支持:\n");
    printf("  GitHub:\n");
    printf("    - github.com/user/repo                    # 自动查找 README\n");
    printf("    - github://user/repo                       # 使用 github:// 协议\n");
    printf("    - github.com/user/repo/blob/main/file.md   # 指定文件和分支\n");
    printf("    - https://github.com/user/repo             # 完整 URL\n");
    printf("  GitLab: gitlab.com/user/repo 或 gitlab://user/repo\n");
    printf("  HTTP:   https://example.com/file.md\n\n");
    printf("示例:\n");
    printf("  %s README.md                                    # 预览本地文件\n", program_name);
    printf("  %s github.com/user/repo                         # 预览 GitHub README\n", program_name);
    printf("  %s github://charmbracelet/glow                  # 使用 github:// 协议\n", program_name);
    printf("  %s github.com/user/repo/blob/main/docs/guide.md # 指定文件路径\n", program_name);
    printf("  %s -s dark file.md               # 使用深色主题\n", program_name);
    printf("  cat file.md | %s -               # 从管道读取\n", program_name);
    printf("  %s -p README.md                  # 使用分页器显示\n", program_name);
}

// 显示版本信息
void print_version(void) {
    printf("flow - Markdown 预览工具 v1.0\n");
    //printf("参考项目: https://github.com/charmbracelet/glow\n");
}

// 解析样式类型
StyleType parse_style(const char *style_str) {
    if (strcmp(style_str, "auto") == 0) return STYLE_AUTO;
    if (strcmp(style_str, "dark") == 0) return STYLE_DARK;
    if (strcmp(style_str, "light") == 0) return STYLE_LIGHT;
    return STYLE_AUTO;
}

// 解析命令行参数
int parse_arguments(int argc, char *argv[], FlowOptions *opts) {
    int opt;
    static struct option long_options[] = {
        {"style", required_argument, 0, 's'},
        {"width", required_argument, 0, 'w'},
        {"pager", no_argument, 0, 'p'},
        {"line-numbers", no_argument, 0, 'n'},
        {"no-status-bar", no_argument, 0, 1},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "s:w:pnhv", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                opts->style = parse_style(optarg);
                break;
            case 'w':
                opts->width = atoi(optarg);
                if (opts->width < 0) opts->width = 0;
                break;
            case 'p':
                opts->use_pager = true;
                break;
            case 'n':
                opts->show_line_numbers = true;
                break;
            case 1:  // --no-status-bar
                opts->show_status_bar = false;
                break;
            case 'h':
                opts->show_help = true;
                break;
            case 'v':
                opts->show_version = true;
                break;
            case '?':
                return 1;
            default:
                break;
        }
    }
    
    // 处理位置参数
    if (optind < argc) {
        if (strcmp(argv[optind], "-") == 0) {
            opts->from_stdin = true;
        } else {
            strncpy(opts->filename, argv[optind], MAX_FILENAME_LENGTH - 1);
            opts->filename[MAX_FILENAME_LENGTH - 1] = '\0';
        }
    } else {
        // 没有参数，尝试从标准输入读取
        if (isatty(STDIN_FILENO) == 0) {
            opts->from_stdin = true;
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    FlowOptions opts;
    
    init_options(&opts);
    
    if (parse_arguments(argc, argv, &opts) != 0) {
        print_help(argv[0]);
        return 1;
    }
    
    if (opts.show_help) {
        print_help(argv[0]);
        return 0;
    }
    
    if (opts.show_version) {
        print_version();
        return 0;
    }
    
    // 读取并处理内容（统一入口，支持本地文件、URL、标准输入）
    ContentInfo *content_info = read_and_process_content(&opts);
    if (content_info == NULL) {
        fprintf(stderr, "%s错误: 无法读取内容%s\n", COLOR_RED, COLOR_RESET);
        if (!opts.from_stdin && strlen(opts.filename) == 0) {
            print_help(argv[0]);
        }
        return 1;
    }
    
    // 渲染内容（所有模式都使用相同的渲染逻辑）
    if (opts.use_pager) {
        // 分页器模式：先渲染，然后传给分页器
        display_with_pager(content_info->content, &opts);
    } else {
        // 直接渲染模式
        render_markdown(content_info->content, &opts);
    }
    
    // 释放内容
    free_content_info(content_info);
    return 0;
}

