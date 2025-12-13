/**
 * dsa - 终端图片查看器
 * 在终端中显示JPEG/PNG图片的ASCII艺术版本
 * 
 * 使用方法: ./dsa image.jpg [width]
 * 
 * 作者: Linux Command Pro Team
 * 版本: 1.0.0
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// Unicode方块字符集，按亮度从暗到亮排列，提供更好的视觉效果
static const char UNICODE_CHARS[] = "█▓▒░";

// 颜色代码
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// 默认宽度
#define DEFAULT_WIDTH 120

// 默认启用颜色
#define DEFAULT_COLOR 1

// 颜色模式
#define COLOR_MODE_8BIT  0  // 8/16色模式
#define COLOR_MODE_24BIT 1  // 24位真彩色模式

// 检测终端是否支持24位真彩色
int detect_truecolor_support() {
    const char *term = getenv("TERM");
    const char *colorterm = getenv("COLORTERM");
    
    // 检查 COLORTERM 环境变量
    if (colorterm) {
        if (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit")) {
            return 1;
        }
    }
    
    // 检查常见的支持真彩色的终端
    if (term) {
        const char *truecolor_terms[] = {
            "xterm-256color", "screen-256color", "tmux-256color",
            "rxvt-unicode-256color", "alacritty", "kitty", "wezterm",
            "vscode", "gnome-terminal", "konsole", "terminator"
        };
        
        for (int i = 0; i < sizeof(truecolor_terms) / sizeof(truecolor_terms[0]); i++) {
            if (strstr(term, truecolor_terms[i])) {
                return 1;
            }
        }
    }
    
    // 尝试通过查询终端能力来检测（更可靠的方法）
    // 发送查询序列并检查响应
    if (isatty(STDOUT_FILENO)) {
        // 大多数现代终端都支持，默认返回1
        // 如果终端不支持，会显示错误的颜色，但不会崩溃
        return 1;
    }
    
    return 0;
}

// 帮助信息
void print_help(const char *program_name) {
    printf("🐧 dsa - 终端图片查看器\n");
    printf("========================\n\n");
    printf("使用方法: %s <图片文件> [宽度]\n\n", program_name);
    printf("参数:\n");
    printf("  图片文件    要显示的图片文件路径 (支持JPG, PNG格式)\n");
    printf("  宽度        可选，ASCII图片的宽度 (默认: %d)\n\n", DEFAULT_WIDTH);
    printf("选项:\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("  -v, --version  显示版本信息\n");
    printf("  -c, --color    启用颜色显示 (默认)\n");
    printf("  -n, --no-color 禁用颜色显示\n");
    printf("  -w, --width    指定宽度\n\n");
    printf("示例:\n");
    printf("  %s image.jpg\n", program_name);
    printf("  %s image.png 120\n", program_name);
    printf("  %s -c image.jpg\n", program_name);
    printf("  %s -n image.jpg\n", program_name);
    printf("  %s --width 100 image.png\n", program_name);
}

// 版本信息
void print_version() {
    printf("dsa version 1.0.0\n");
    printf("Copyright (c) 2025 Linux Command Pro Team\n");
    printf("MIT License\n");
}

// 将RGB值转换为灰度值
unsigned char rgb_to_gray(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
}

// 获取Unicode字符
char* get_unicode_char(unsigned char gray_value) {
    static char result[8]; // 支持多字节Unicode字符
    int index = (gray_value * 3) / 255; // 4个字符，索引0-3
    
    // 确保索引在有效范围内
    if (index > 3) index = 3;
    
    // 根据索引返回对应的Unicode字符
    switch(index) {
        case 0: strcpy(result, "░"); break; // 最亮
        case 1: strcpy(result, "▒"); break;
        case 2: strcpy(result, "▓"); break;
        case 3: strcpy(result, "█"); break; // 最暗
        default: strcpy(result, "░"); break;
    }
    return result;
}

// 获取24位真彩色代码
void get_truecolor_code(unsigned char r, unsigned char g, unsigned char b, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "\033[38;2;%d;%d;%dm", r, g, b);
}

// 将RGB转换为256色模式（8位颜色）
int rgb_to_256color(unsigned char r, unsigned char g, unsigned char b) {
    // 使用标准256色映射算法
    // 前16色是系统颜色，跳过
    // 216色是6x6x6的RGB立方体 (16-231)
    // 最后24色是灰度 (232-255)
    
    // 如果颜色接近灰度，使用灰度色阶
    int max_val = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    int min_val = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    
    if (max_val - min_val < 32) {
        // 使用灰度色阶 (232-255)
        int gray = (r + g + b) / 3;
        return 232 + (gray * 23) / 255;
    }
    
    // 使用RGB立方体 (16-231)
    // 每个分量映射到0-5
    int r6 = (r * 5) / 255;
    int g6 = (g * 5) / 255;
    int b6 = (b * 5) / 255;
    
    return 16 + r6 * 36 + g6 * 6 + b6;
}

// 获取颜色代码 - 支持24位真彩色和256色模式
void get_color_code(unsigned char r, unsigned char g, unsigned char b, 
                    int color_mode, char *buffer, size_t buffer_size) {
    if (color_mode == COLOR_MODE_24BIT) {
        // 使用24位真彩色
        get_truecolor_code(r, g, b, buffer, buffer_size);
    } else {
        // 使用256色模式
        int color_code = rgb_to_256color(r, g, b);
        snprintf(buffer, buffer_size, "\033[38;5;%dm", color_code);
    }
}

// 获取颜色代码（旧版本兼容，用于8/16色模式）
const char* get_color_code_8bit(unsigned char r, unsigned char g, unsigned char b) {
    static char buffer[32];
    
    // 计算亮度和饱和度
    int brightness = (r + g + b) / 3;
    int max_val = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    int min_val = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    int saturation = max_val - min_val;
    
    // 改进的颜色映射算法
    // 使用更精确的阈值和更丰富的颜色判断
    
    // 如果饱和度很低，使用灰度
    if (saturation < 25) {
        if (brightness > 220) return "\033[97m";      // 很亮白
        else if (brightness > 180) return "\033[37m"; // 亮白
        else if (brightness > 140) return "\033[90m"; // 中亮灰
        else if (brightness > 100) return "\033[90m"; // 中灰
        else if (brightness > 60) return "\033[90m";  // 暗灰
        else if (brightness > 30) return "\033[30m";  // 很暗灰
        else return "\033[30m";                       // 黑色
    }
    
    // 计算各颜色分量的相对强度
    int r_ratio = (r * 100) / (max_val + 1);
    int g_ratio = (g * 100) / (max_val + 1);
    int b_ratio = (b * 100) / (max_val + 1);
    
    // 根据主色调和亮度确定颜色
    if (r > g + 30 && r > b + 30) {
        // 红色系（红色明显占优）
        if (brightness > 200) return "\033[91m";      // 亮红
        else if (brightness > 140) return "\033[31m"; // 红
        else if (brightness > 80) return "\033[31m";  // 中红
        else return "\033[31m";                       // 暗红
    } else if (g > r + 30 && g > b + 30) {
        // 绿色系（绿色明显占优）
        if (brightness > 200) return "\033[92m";      // 亮绿
        else if (brightness > 140) return "\033[32m"; // 绿
        else if (brightness > 80) return "\033[32m";  // 中绿
        else return "\033[32m";                       // 暗绿
    } else if (b > r + 30 && b > g + 30) {
        // 蓝色系（蓝色明显占优）
        if (brightness > 200) return "\033[94m";      // 亮蓝
        else if (brightness > 140) return "\033[34m"; // 蓝
        else if (brightness > 80) return "\033[34m";  // 中蓝
        else return "\033[34m";                       // 暗蓝
    } else if (r > 180 && g > 180 && b < 120) {
        // 黄色系（红+绿，蓝少）
        if (brightness > 200) return "\033[93m";      // 亮黄
        else return "\033[33m";                       // 黄
    } else if (r > 180 && g < 120 && b > 180) {
        // 洋红色系（红+蓝，绿少）
        if (brightness > 200) return "\033[95m";      // 亮洋红
        else return "\033[35m";                       // 洋红
    } else if (r < 120 && g > 180 && b > 180) {
        // 青色系（绿+蓝，红少）
        if (brightness > 200) return "\033[96m";      // 亮青
        else return "\033[36m";                       // 青
    } else if (r > 140 && g > 140 && b > 140) {
        // 白色系（所有颜色都较高）
        if (brightness > 220) return "\033[97m";      // 很亮白
        else if (brightness > 180) return "\033[37m"; // 亮白
        else return "\033[37m";                       // 白
    }
    
    // 混合色：根据主要颜色分量选择
    if (r_ratio > g_ratio && r_ratio > b_ratio) {
        // 偏红
        if (brightness > 150) return "\033[91m";
        else return "\033[31m";
    } else if (g_ratio > r_ratio && g_ratio > b_ratio) {
        // 偏绿
        if (brightness > 150) return "\033[92m";
        else return "\033[32m";
    } else if (b_ratio > r_ratio && b_ratio > g_ratio) {
        // 偏蓝
        if (brightness > 150) return "\033[94m";
        else return "\033[34m";
    }
    
    // 默认返回基于亮度的颜色
    if (brightness > 200) return "\033[97m";
    else if (brightness > 150) return "\033[37m";
    else if (brightness > 100) return "\033[90m";
    else if (brightness > 50) return "\033[90m";
    else return "\033[30m";
}

// 显示图片
int display_image(const char *filename, int width, int use_color) {
    int x, y, n;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
    
    if (!data) {
        fprintf(stderr, "❌ 错误: 无法加载图片 '%s'\n", filename);
        fprintf(stderr, "   请检查文件是否存在且格式正确 (支持JPG, PNG)\n");
        return 1;
    }
    
    // 检测颜色模式
    int color_mode = COLOR_MODE_8BIT;
    const char *color_mode_str = "8/16色";
    if (use_color) {
        if (detect_truecolor_support()) {
            color_mode = COLOR_MODE_24BIT;
            color_mode_str = "24位真彩色";
        } else {
            color_mode = COLOR_MODE_8BIT;
            color_mode_str = "256色";
        }
    }
    
    printf("🖼️  图片信息: %dx%d, %d通道\n", x, y, n);
    printf("📏 显示宽度: %d 字符\n", width);
    printf("🎨 颜色模式: %s", use_color ? color_mode_str : "禁用");
    if (use_color && color_mode == COLOR_MODE_24BIT) {
        printf(" ✨");
    }
    printf("\n\n");
    
    // 计算缩放比例 - 提高分辨率
    float scale = (float)width / x;
    int new_height = (int)(y * scale * 0.6); // 字符高度约为宽度的0.6倍，提高分辨率
    
    if (new_height <= 0) new_height = 1;
    
    printf("📐 缩放后尺寸: %dx%d\n\n", width, new_height);
    
    // 生成ASCII艺术 - 使用改进的采样算法
    for (int i = 0; i < new_height; i++) {
        for (int j = 0; j < width; j++) {
            // 计算原始图片中的对应位置
            int orig_x = (int)(j / scale);
            int orig_y = (int)(i / scale / 0.6);
            
            if (orig_x >= x) orig_x = x - 1;
            if (orig_y >= y) orig_y = y - 1;
            
            // 使用区域采样提高质量
            int sample_size = 2; // 采样区域大小
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;
            
            for (int dy = -sample_size/2; dy <= sample_size/2; dy++) {
                for (int dx = -sample_size/2; dx <= sample_size/2; dx++) {
                    int sample_x = orig_x + dx;
                    int sample_y = orig_y + dy;
                    
                    if (sample_x >= 0 && sample_x < x && sample_y >= 0 && sample_y < y) {
                        int pixel_index = (sample_y * x + sample_x) * n;
                        r_sum += data[pixel_index];
                        g_sum += data[pixel_index + 1];
                        b_sum += data[pixel_index + 2];
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                unsigned char r = r_sum / count;
                unsigned char g = g_sum / count;
                unsigned char b = b_sum / count;
                
                // 转换为灰度
                unsigned char gray = rgb_to_gray(r, g, b);
                
                // 获取Unicode字符
                char* unicode_char = get_unicode_char(gray);
                
                // 输出字符
                if (use_color) {
                    char color_buffer[64];
                    // 使用24位真彩色或256色模式
                    get_color_code(r, g, b, color_mode, color_buffer, sizeof(color_buffer));
                    printf("%s%s%s", color_buffer, unicode_char, RESET);
                } else {
                    printf("%s", unicode_char);
                }
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    
    printf("\n✨ 图片显示完成!\n");
    
    // 释放内存
    stbi_image_free(data);
    return 0;
}

int main(int argc, char *argv[]) {
    int width = DEFAULT_WIDTH;
    int use_color = DEFAULT_COLOR; // 默认启用颜色
    char *filename = NULL;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) {
            use_color = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) {
                width = atoi(argv[++i]);
                if (width <= 0) {
                    fprintf(stderr, "❌ 错误: 宽度必须大于0\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "❌ 错误: --width 需要指定数值\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            if (!filename) {
                filename = argv[i];
            } else if (width == DEFAULT_WIDTH) {
                // 如果已经设置了文件名，且宽度还是默认值，则第二个参数是宽度
                width = atoi(argv[i]);
                if (width <= 0) {
                    fprintf(stderr, "❌ 错误: 宽度必须大于0\n");
                    return 1;
                }
            }
        } else {
            fprintf(stderr, "❌ 错误: 未知选项 '%s'\n", argv[i]);
            fprintf(stderr, "使用 '%s --help' 查看帮助信息\n", argv[0]);
            return 1;
        }
    }
    
    // 检查是否指定了图片文件
    if (!filename) {
        fprintf(stderr, "❌ 错误: 请指定图片文件\n");
        fprintf(stderr, "使用 '%s --help' 查看帮助信息\n", argv[0]);
        return 1;
    }
    
    // 检查文件是否存在
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "❌ 错误: 文件 '%s' 不存在或无法访问\n", filename);
        return 1;
    }
    fclose(file);
    
    // 显示图片
    return display_image(filename, width, use_color);
}
