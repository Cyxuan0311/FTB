#ifndef COMMON_MEDIA_HPP
#define COMMON_MEDIA_HPP

#include <string>
#include <vector>
#include <ftxui/component/component.hpp>  // FTXUI组件库

namespace CommonMedia {

// RGB像素颜色结构体
struct PixelColor {
    uint8_t r;  // 红色分量
    uint8_t g;  // 绿色分量
    uint8_t b;  // 蓝色分量
};

// 显示配置结构体，包含各种图像处理参数
struct DisplayConfig {
    float scaleRatio = 1.2f;      // 图像缩放比例
    float gamma = 0.85f;          // 伽马校正值
    float contrast = 1.2f;        // 对比度增强系数
    float brightness = 1.1f;      // 亮度调整系数
    bool useHighResolution = true; // 是否使用高分辨率模式
    bool preserveAspectRatio = true; // 是否保持原始宽高比
    bool fitToScreen = false;      // 是否拉伸以适应屏幕
};

// 工具函数声明
std::string codepointToUtf8(int codepoint);  // Unicode码点转UTF-8字符串
float calculateBrightness(const PixelColor& color);  // 计算像素亮度
PixelColor enhanceColor(const PixelColor& color, float contrast, float brightness, float gamma);  // 增强颜色
std::vector<std::pair<int, float>> getExtendedUnicodeCharSet();  // 获取扩展Unicode字符集

// 渲染子图像到FTXUI元素
ftxui::Element renderSubImage(
    const std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>& fullAsciiArt,
    int offset_x, int offset_y,
    int viewportWidth, int viewportHeight);

// 计算显示尺寸，考虑各种约束条件
void calculateDisplayDimensions(
    int originalWidth, int originalHeight,
    int maxWidth, int maxHeight,
    int& displayWidth, int& displayHeight,
    bool considerTerminalAspect = true,
    bool fitToScreen = false);

} // namespace CommonMedia

#endif // COMMON_MEDIA_HPP
