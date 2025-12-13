#include "Video_and_Photo/CommonMedia.hpp"
#include <cmath>
#include <algorithm>

namespace CommonMedia {

// --------------------------------------------------
// 将 Unicode 码点转换为 UTF-8 编码的字符串
// 参数：
//   codepoint - Unicode 码点值（0 到 0x10FFFF）
// 返回：
//   对应的 UTF-8 编码字符序列 (std::string)
// 说明：根据 UTF-8 编码规则，将单个码点拆分为 1～4 个字节
// --------------------------------------------------
std::string codepointToUtf8(int codepoint) {
    std::string result;
    if (codepoint < 0x80) {
        // 单字节编码：0xxxxxxx
        result.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint < 0x800) {
        // 双字节编码：110xxxxx 10xxxxxx
        result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint < 0x10000) {
        // 三字节编码：1110xxxx 10xxxxxx 10xxxxxx
        result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else if (codepoint < 0x110000) {
        // 四字节编码：11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return result;
}

// --------------------------------------------------
// 计算像素颜色的亮度（灰度值归一化）
// 参数：
//   color - PixelColor 结构，包含 r、g、b 分量 (0-255)
// 返回：
//   标准化到 [0,1] 范围内的亮度值
// 说明：参考 dsa 项目的 RGB 到灰度转换算法
//       使用标准亮度系数：0.299∙R + 0.587∙G + 0.114∙B（与 dsa 一致）
//       这比 Rec. 709 更适合终端显示
// --------------------------------------------------
float calculateBrightness(const PixelColor& color) {
    // 使用与 dsa 相同的 RGB 到灰度转换系数
    // 这提供了更好的视觉对比度
    return (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
}

// --------------------------------------------------
// 对单个像素进行对比度、亮度和伽马校正增强
// 参数：
//   color     - PixelColor 结构，包含 r、g、b 分量 (0-255)
//   contrast  - 对比度系数 (>0, 1.0 表示不变，>1.0 提高对比度，<1.0 降低对比度)
//   brightness - 亮度系数 (>0, 1.0 表示不变，大于 1.0 提升亮度)
//   gamma     - 伽马校正指数 (>0, 1.0 表示不校正，<1.0 提亮暗部, >1.0 压暗暗部)
// 返回：
//   增强后的 PixelColor 结构，r, g, b 分量重新计算后被限制在 0-255
// 说明：对每个分量先归一化到 [0,1]，进行伽马校正、对比度调整，然后乘以亮度，
//      最后映射回 [0,255] 并使用 std::clamp 限制范围。
// --------------------------------------------------
PixelColor enhanceColor(const PixelColor& color, float contrast, float brightness, float gamma) {
    // 内部 lambda：处理单通道分量
    auto enhance = [](uint8_t value, float contrast, float brightness, float gamma) -> uint8_t {
        // 归一化到 [0,1]
        float normalized = value / 255.0f;
        // 伽马校正
        normalized = std::pow(normalized, gamma);
        // 对比度调整：((val - 0.5) * contrast + 0.5)
        normalized = (normalized - 0.5f) * contrast + 0.5f;
        // 亮度调整
        normalized = normalized * brightness;
        // 映射回 0-255，并确保不越界
        return static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
    };

    // 对 r/g/b 分别处理
    return {
        enhance(color.r, contrast, brightness, gamma),
        enhance(color.g, contrast, brightness, gamma),
        enhance(color.b, contrast, brightness, gamma)
    };
}

// --------------------------------------------------
// 获取扩展 Unicode 字符集及对应"占用亮度"比例
// 返回：
//   包含多个 (Unicode 码点, 亮度比例) 对的 vector，用于 ASCII 艺术或字符画映射
// 说明：完全使用 dsa 项目的字符集：█▓▒░ (4个字符)
//       按亮度从暗到亮排列：█ (最暗) -> ▓ -> ▒ -> ░ (最亮)
//       对应 dsa 的 UNICODE_CHARS = "█▓▒░"
// --------------------------------------------------
std::vector<std::pair<int, float>> getExtendedUnicodeCharSet() {
    // 完全使用 dsa 项目的字符集，只使用 4 个字符
    // 按 dsa 的 get_unicode_char 函数逻辑：
    // index = (gray_value * 3) / 255，索引 0-3 对应 ░▒▓█
    // 但 dsa 中 index=0 是最亮(░)，index=3 是最暗(█)
    // 我们按亮度值从暗到亮排列，对应亮度比例
    return {
        {0x2588, 1.0f},   // █ FULL BLOCK (最暗，对应 dsa index=3)
        {0x2593, 0.75f},  // ▓ DARK SHADE (对应 dsa index=2)
        {0x2592, 0.5f},   // ▒ MEDIUM SHADE (对应 dsa index=1)
        {0x2591, 0.25f}   // ░ LIGHT SHADE (最亮，对应 dsa index=0)
    };
}

// --------------------------------------------------
// 渲染子区域 ASCII 艺术图像
// 参数：
//   fullAsciiArt  - 二维数组表示完整 ASCII 艺术画面：
//                   每个元素是 (符号行 vector, 对应行的像素颜色 vector)
//   offset_x      - 水平方向偏移量（子区域起点 x）
//   offset_y      - 垂直方向偏移量（子区域起点 y）
//   viewportWidth - 子区域宽度（字符数）
//   viewportHeight- 子区域高度（行数）
// 返回：
//   一个 FTXUI Element，包含在指定子区域范围内的行和字符，
//   并为每个字符上色
// 说明：
//   - 若 fullAsciiArt 为空，则显示“加载中...”
//   - 计算子区域在全画面中的结束行和结束列时，需限于原始尺寸
//   - 将每行字符与对应颜色组合为 hbox，再合成 vbox 返回
// --------------------------------------------------
ftxui::Element renderSubImage(
    const std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>& fullAsciiArt,
    int offset_x, int offset_y,
    int viewportWidth, int viewportHeight) 
{
    // 如果还没生成 ASCII 艺术，显示加载提示
    if (fullAsciiArt.empty()) {
        return ftxui::text("加载中...") | ftxui::center;
    }

    std::vector<ftxui::Element> lines;
    // 计算子区域结束行，不能超过原始行数
    int endRow = std::min(offset_y + viewportHeight, static_cast<int>(fullAsciiArt.size()));

    // 遍历子区域内的每一行
    for (int y = offset_y; y < endRow; ++y) {
        const auto& rowSymbols = fullAsciiArt[y].first;   // 当前行的字符符号
        const auto& rowColors  = fullAsciiArt[y].second;  // 当前行对应的像素颜色
        std::vector<ftxui::Element> row;

        // 计算当前行子区域结束列，不能超过原始列数
        int endCol = std::min(offset_x + viewportWidth, static_cast<int>(rowSymbols.size()));
        // 遍历子区域内的每个字符
        for (int x = offset_x; x < endCol; ++x) {
            const auto& color = rowColors[x];
            // 创建一个带颜色的字符元素
            row.push_back(
                ftxui::text(rowSymbols[x]) |
                ftxui::color(ftxui::Color::RGB(color.r, color.g, color.b))
            );
        }

        // 将这一行的字符水平排列
        lines.push_back(ftxui::hbox(std::move(row)));
    }

    // 垂直排列所有行，并允许自动伸缩
    return ftxui::vbox(std::move(lines)) | ftxui::flex;
}

// --------------------------------------------------
// 计算图像在终端中显示的宽度与高度
// 参数：
//   originalWidth       - 图像原始宽度（像素）
//   originalHeight      - 图像原始高度（像素）
//   maxWidth            - 终端允许的最大宽度（字符列数）
//   maxHeight           - 终端允许的最大高度（行数）
//   displayWidth        - 计算得到的显示宽度（输出引用）
//   displayHeight       - 计算得到的显示高度（输出引用）
//   considerTerminalAspect - 是否考虑终端字符的宽高比（TRUE 时假设字符宽高比约 1:2）
//   fitToScreen         - 是否填满整个可见区域（TRUE 会尽量铺满，但保持纵横比）
//                            FALSE 则保持原始尺寸，最大不超过屏幕
// 说明：
//   - 若 considerTerminalAspect 为 true，则图片纵横比需乘以 0.5（字符高度是宽度的约两倍）
//   - 计算 fitToScreen 时，根据 targetAspectRatio 决定以宽度撑满或以高度撑满
//   - 计算非填充模式时，先设 displayWidth/Height = originalWidth/Height，
//     再按比例缩放，直到不超过 maxWidth/maxHeight
//   - 最后将 displayWidth/Height 限制在最小 20x10 和最大 maxWidth/maxHeight 之间
// --------------------------------------------------
void calculateDisplayDimensions(
    int originalWidth, int originalHeight,
    int maxWidth, int maxHeight,
    int& displayWidth, int& displayHeight,
    bool considerTerminalAspect,
    bool fitToScreen) 
{
    // 终端字符单元宽高比：假设字符宽:高 = 1:2，故乘以 0.5
    float terminalAspectRatio = considerTerminalAspect ? 0.5f : 1.0f;
    // 图像本身宽高比
    float imageAspectRatio = static_cast<float>(originalWidth) / originalHeight;
    // 最终目标宽高比 = 图像宽高比 * 终端字符宽高比
    float targetAspectRatio = imageAspectRatio * terminalAspectRatio;

    if (fitToScreen) {
        // 若填充模式：尽量占满可用区域
        if (static_cast<float>(maxWidth) / maxHeight > targetAspectRatio) {
            // 可用区域宽高比大于目标，按高度撑满，宽度按比例计算
            displayHeight = maxHeight;
            displayWidth  = static_cast<int>(maxHeight * targetAspectRatio);
        } else {
            // 可用区域宽高比小于目标，按宽度撑满，按比例计算高度
            displayWidth  = maxWidth;
            displayHeight = static_cast<int>(maxWidth / targetAspectRatio);
        }
    } else {
        // 保持接近原始尺寸，但不超过最大值
        displayWidth  = originalWidth;
        displayHeight = originalHeight;

        float scale = 1.0f;
        // 若宽度超过最大值，则按比例缩放
        if (displayWidth > maxWidth) {
            scale = static_cast<float>(maxWidth) / displayWidth;
        }
        // 若缩放后的高度仍超过最大高度，则更新缩放比例
        if (displayHeight * scale > maxHeight) {
            scale = static_cast<float>(maxHeight) / displayHeight;
        }

        // 应用缩放比例
        displayWidth  = static_cast<int>(displayWidth * scale);
        displayHeight = static_cast<int>(displayHeight * scale);
    }

    // 确保显示尺寸至少为 20x10，不超过可用窗口
    displayWidth  = std::clamp(displayWidth, 20, maxWidth);
    displayHeight = std::clamp(displayHeight, 10, maxHeight);
}

} // namespace CommonMedia
