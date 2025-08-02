#include "Video_and_Photo/ImageViewer.hpp"
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace ImageViewer {

// --------------------------------------------------
// ImageDecoder::decodeImage
// 功能：使用 FFmpeg 解码图像文件，将其转换为原始 RGB 像素数据
// 参数：
//   path      - 图像文件路径
//   pixelData - 输出参数，用于存储解码后连续的 RGB24 像素数据
//   width     - 输出参数，图像宽度（像素）
//   height    - 输出参数，图像高度（像素）
// 返回：
//   解码成功返回 true，否则返回 false
// 说明：内部创建一个 FFmpegContext，打开文件并解码首帧
// --------------------------------------------------
bool ImageDecoder::decodeImage(const std::string& path,
                               std::vector<uint8_t>& pixelData,
                               int& width, int& height) {
    // 创建 FFmpegContext，用于打开文件和解码
    FFmpegUtils::FFmpegContext ctx;
    // 尝试打开指定路径的图像文件
    if (!ctx.open(path)) {
        std::cerr << "❌ 无法打开图片文件" << std::endl;
        return false;
    }

    // 使用 decodeFrame 将首帧解码为 RGB24 数据
    bool success = ctx.decodeFrame(pixelData, width, height);
    return success;
}

// --------------------------------------------------
// ImageDecoder::calculateDisplayDimensions
// 功能：根据原始图像尺寸和终端格子大小，计算适合终端显示的像素宽高
// 参数：
//   originalWidth  - 图像原始宽度（像素）
//   originalHeight - 图像原始高度（像素）
//   terminalCols   - 终端列数（可显示字符宽度）
//   terminalRows   - 终端行数（可显示字符高度）
//   block_width    - 每个字符“块”对应的像素宽度
//   block_height   - 每个字符“块”对应的像素高度
//   displayWidth   - 输出参数，计算得到的显示宽度（像素）
//   displayHeight  - 输出参数，计算得到的显示高度（像素）
// 说明：将字符格子尺寸乘以列/行数得到实际像素可用空间，内部调用 CommonMedia::calculateDisplayDimensions
// --------------------------------------------------
void ImageDecoder::calculateDisplayDimensions(
    int originalWidth, int originalHeight,
    int terminalCols, int terminalRows,
    int block_width, int block_height,
    int& displayWidth, int& displayHeight) {

    // 调用通用函数，传入终端像素最大可用宽高
    CommonMedia::calculateDisplayDimensions(
        originalWidth, originalHeight,
        terminalCols * block_width,
        terminalRows * block_height,
        displayWidth, displayHeight);
}

// --------------------------------------------------
// ImageDecoder::convertToColorAsciiArt
// 功能：将解码后的 RGB 像素数据转换为彩色 ASCII 艺术表示
// 参数：
//   pixels        - 原始 RGB24 像素数据，连续排列，每像素 3 字节 (R,G,B)
//   originalWidth - 原始图像宽度（像素）
//   originalHeight- 原始图像高度（像素）
//   displayWidth  - 目标显示宽度（像素）
//   displayHeight - 目标显示高度（像素）
//   config        - DisplayConfig 配置：对比度、亮度、伽马校正、高分辨率标志等
// 返回：
//   二维向量，每行包含字符（vector<string>）和对应的颜色（vector<PixelColor>）
// 说明：
//   1. 根据 displayWidth/Height 做采样，按行列遍历目标像素点
//   2. 对采样到的像素做颜色增强（Contrast/Brightness/Gamma）
//   3. 计算归一化亮度，并选择亮度最接近的 Unicode 块字符
//   4. 将该字符的 UTF-8 字符串和对应颜色保存到结果列表中
// --------------------------------------------------
std::vector<std::pair<std::vector<std::string>, std::vector<CommonMedia::PixelColor>>>
ImageDecoder::convertToColorAsciiArt(
    const std::vector<uint8_t>& pixels,
    int originalWidth, int originalHeight,
    int displayWidth, int displayHeight,
    const CommonMedia::DisplayConfig& config) {

    // 最终结果：每行的字符和每行对应的颜色像素
    std::vector<std::pair<std::vector<std::string>, std::vector<CommonMedia::PixelColor>>> result;

    // 获取预定义的 Unicode 字符集及其“灰度值”比例
    const auto& charSet = CommonMedia::getExtendedUnicodeCharSet();

    // 定义一个采样 lambda：根据浮点坐标返回原图中对应的像素颜色
    auto get_pixel = [&](float x, float y) -> CommonMedia::PixelColor {
        // clamp 坐标到图像范围内
        int srcX = std::clamp(static_cast<int>(x), 0, originalWidth - 1);
        int srcY = std::clamp(static_cast<int>(y), 0, originalHeight - 1);
        int idx = (srcY * originalWidth + srcX) * 3;
        return {
            pixels[idx],       // R
            pixels[idx + 1],   // G
            pixels[idx + 2]    // B
        };
    };

    // 根据配置决定单个“块”所占的像素行列，若高分辨率模式则使用单像素块，否则两像素块
    int blockHeight = config.useHighResolution ? 1 : 2;
    int blockWidth  = config.useHighResolution ? 1 : 2;

    // 每循环一次处理一行
    for (int y = 0; y < displayHeight; y += blockHeight) {
        std::vector<std::string> rowChars;                      // 当前行的字符
        std::vector<CommonMedia::PixelColor> rowColors;         // 当前行对应颜色

        // 每循环一次处理一列
        for (int x = 0; x < displayWidth; x += blockWidth) {
            // 计算在原图的浮点采样坐标
            float sampleX = x * static_cast<float>(originalWidth) / displayWidth;
            float sampleY = y * static_cast<float>(originalHeight) / displayHeight;
            // 获取对应像素颜色
            auto pixel = get_pixel(sampleX, sampleY);

            // 对该像素做颜色增强（对比度、亮度、伽马校正）
            pixel = CommonMedia::enhanceColor(pixel,
                                             config.contrast,
                                             config.brightness,
                                             config.gamma);

            // 计算归一化亮度（0.0 - 1.0）
            float brightness = CommonMedia::calculateBrightness(pixel);

            // 在 charSet 中寻找亮度最接近的字符
            auto bestChar = charSet[0];
            float minDiff = std::abs(brightness - charSet[0].second);
            for (size_t i = 1; i < charSet.size(); ++i) {
                float diff = std::abs(brightness - charSet[i].second);
                if (diff < minDiff) {
                    minDiff = diff;
                    bestChar = charSet[i];
                }
            }

            // 将选中的 Unicode 码点转换为 UTF-8 字符串
            rowChars.push_back(CommonMedia::codepointToUtf8(bestChar.first));
            // 将当前像素颜色保存
            rowColors.push_back(pixel);
        }

        // 将当前行的字符和颜色打包，加入结果
        result.emplace_back(std::move(rowChars), std::move(rowColors));
    }

    return result;
}

// --------------------------------------------------
// ImageViewerUI::showImagePreview
// 功能：在 FTXUI 界面中预览图像，将其渲染为彩色 ASCII 艺术
// 参数：
//   imagePath - 图像文件路径
//   screen    - FTXUI 交互式屏幕引用，用于显示和事件循环
// 说明：
//   1. 初始化 State，包括配置、原始尺寸、终端尺寸等
//   2. 调用 ImageDecoder 解码图片，获取原始像素数据及宽高
//   3. 计算终端可用列数与行数
//   4. 基于默认放缩配置，计算初始显示尺寸并生成 ASCII 艺术
//   5. 构建 FTXUI Renderer 显示组件，包含图像内容与状态提示
//   6. 捕获 +/- 缩放、方向键滚动、ESC 退出等事件，实时更新显示
// --------------------------------------------------
void ImageViewerUI::showImagePreview(const std::string& imagePath,
                                     ftxui::ScreenInteractive& screen) {
    State state;
    std::vector<uint8_t> pixelData;

    // ---------------- 初始化显示配置 ----------------
    state.config = {
        1.2f,  // scaleRatio：初始缩放比例 120%
        0.85f, // gamma：伽马校正
        1.2f,  // contrast：对比度
        1.1f,  // brightness：亮度
        true,  // useHighResolution：是否使用高分辨率（单像素块）
        true,  // preserveAspectRatio：保持纵横比（目前未在代码中使用）
        false  // fitToScreen：是否完全填满屏幕（这里优先保持原始尺寸）
    };

    // ---------------- 解码图片 ----------------
    if (!ImageDecoder::decodeImage(imagePath,
                                   pixelData,
                                   state.originalWidth,
                                   state.originalHeight)) {
        // 解码失败则直接返回
        return;
    }

    // ---------------- 获取终端尺寸 ----------------
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    // 预留一些行空间用于标题、分隔线和状态栏
    state.terminalCols = w.ws_col - 2;
    state.terminalRows = w.ws_row - 7;

    // 单个字符块在像素中的宽高
    const int block_width = 2;
    const int block_height = 4;

    // ---------------- 计算初始显示尺寸 ----------------
    ImageDecoder::calculateDisplayDimensions(
        state.originalWidth, state.originalHeight,
        state.terminalCols, state.terminalRows,
        block_width, block_height,
        state.displayWidth, state.displayHeight
    );

    // ---------------- 生成完整 ASCII 艺术 ----------------
    auto fullAsciiArt = ImageDecoder::convertToColorAsciiArt(
        pixelData,
        state.originalWidth, state.originalHeight,
        state.displayWidth, state.displayHeight,
        state.config
    );

    // ---------------- FTXUI 渲染组件 ----------------
    auto component = ftxui::Renderer([&] {
        // 使用 CommonMedia::renderSubImage 渲染子区域
        auto imageElement = CommonMedia::renderSubImage(
            fullAsciiArt,
            state.offset_x, state.offset_y,
            state.terminalCols, state.terminalRows
        );

        // 构造状态信息字符串：缩放、偏移、原始分辨率
        std::string status = "缩放: " + std::to_string(static_cast<int>(state.scaleRatio * 100)) + "% ";
        status += "位置: " + std::to_string(state.offset_x) + "," + std::to_string(state.offset_y);
        status += " 分辨率: " + std::to_string(state.originalWidth) + "x" +
                  std::to_string(state.originalHeight);

        // 返回一个垂直布局，包括文件名、分隔线、图像、分隔线、状态提示、控制说明
        return ftxui::vbox({
            // 文件名标题，使用绿色加粗显示
            ftxui::text("🖼️ " + std::filesystem::path(imagePath).filename().string()) |
                ftxui::bold | ftxui::color(ftxui::Color::Green),
            ftxui::separator(),
            // 中心对齐图像内容
            imageElement | ftxui::center,
            ftxui::separator(),
            // 状态行：缩放、偏移、分辨率
            ftxui::text(status) | ftxui::color(ftxui::Color::GrayDark),
            // 控制说明：+/- 缩放，方向键滚动，ESC 退出
            ftxui::text("控制: +/-缩放, ↑↓←→滚动, ESC退出") |
                ftxui::color(ftxui::Color::GrayLight)
        }) | ftxui::border;
    });

    // ---------------- 事件捕获与处理 ----------------
    auto main_component = ftxui::CatchEvent(component, [&](ftxui::Event event) {
        // ESC 键退出图像预览
        if (event == ftxui::Event::Escape) {
            screen.Exit();
            return true;
        }

        bool needRefresh = false;

        // 处理放大：+ 键或 = 键
        if (event == ftxui::Event::Character('+') ||
            event == ftxui::Event::Character('=')) {
            state.scaleRatio = std::min(4.0f, state.scaleRatio * 1.2f);
            needRefresh = true;
        }
        // 处理缩小：- 键或 _ 键
        else if (event == ftxui::Event::Character('-') ||
                 event == ftxui::Event::Character('_')) {
            state.scaleRatio = std::max(0.1f, state.scaleRatio / 1.2f);
            needRefresh = true;
        }

        // 处理垂直滚动：↑ 向上滚动
        if (event == ftxui::Event::ArrowUp) {
            state.offset_y = std::max(0, state.offset_y - 1);
            return true;
        }
        // ↓ 向下滚动
        else if (event == ftxui::Event::ArrowDown) {
            state.offset_y = std::min(
                static_cast<int>(fullAsciiArt.size()) - state.terminalRows,
                state.offset_y + 1
            );
            return true;
        }
        // ← 向左滚动
        else if (event == ftxui::Event::ArrowLeft) {
            state.offset_x = std::max(0, state.offset_x - 1);
            return true;
        }
        // → 向右滚动
        else if (event == ftxui::Event::ArrowRight) {
            if (!fullAsciiArt.empty()) {
                state.offset_x = std::min(
                    static_cast<int>(fullAsciiArt[0].first.size()) - state.terminalCols,
                    state.offset_x + 1
                );
            }
            return true;
        }

        // 如果需要重新缩放与重新生成 ASCII 艺术
        if (needRefresh) {
            // 重置滚动偏移
            state.offset_x = 0;
            state.offset_y = 0;

            // 重新计算显示尺寸，应用新的缩放比例
            ImageDecoder::calculateDisplayDimensions(
                state.originalWidth, state.originalHeight,
                state.terminalCols, state.terminalRows,
                static_cast<int>(block_width * state.scaleRatio),
                static_cast<int>(block_height * state.scaleRatio),
                state.displayWidth, state.displayHeight
            );

            // 根据新的 displayWidth/Height 重新生成 ASCII 艺术
            fullAsciiArt = ImageDecoder::convertToColorAsciiArt(
                pixelData,
                state.originalWidth, state.originalHeight,
                state.displayWidth, state.displayHeight,
                state.config
            );
            return true;
        }

        return false;
    });

    // 进入 FTXUI 事件循环，开始显示并响应事件
    screen.Loop(main_component);
}

} // namespace ImageViewer
