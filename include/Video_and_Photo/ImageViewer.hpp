#ifndef IMAGEVIEWER_HPP
#define IMAGEVIEWER_HPP

#include "CommonMedia.hpp"
#include "FFmpegUtils.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>

namespace ImageViewer {

// 图像解码与处理类
class ImageDecoder {
public:
    static bool decodeImage(const std::string& path,
                           std::vector<uint8_t>& pixelData,
                           int& width, int& height);

    static void calculateDisplayDimensions(
        int originalWidth, int originalHeight,
        int terminalCols, int terminalRows,
        int block_width, int block_height,
        int& displayWidth, int& displayHeight);

    static std::vector<std::pair<std::vector<std::string>, std::vector<CommonMedia::PixelColor>>>
    convertToColorAsciiArt(const std::vector<uint8_t>& pixels,
                          int originalWidth, int originalHeight,
                          int displayWidth, int displayHeight,
                          const CommonMedia::DisplayConfig& config);
};

// 图像预览 UI 类
class ImageViewerUI {
public:
    static void showImagePreview(const std::string& imagePath,
                                ftxui::ScreenInteractive& screen);

    struct State {
        int offset_x = 0;
        int offset_y = 0;
        float scaleRatio = 1.0f;
        int originalWidth = 0;
        int originalHeight = 0;
        int displayWidth = 0;
        int displayHeight = 0;
        int terminalCols = 0;
        int terminalRows = 0;
        CommonMedia::DisplayConfig config;
    };
};

} // namespace ImageViewer

#endif // IMAGEVIEWER_HPP