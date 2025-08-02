#pragma once

#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace VideoPlayer {

    std::string codepointToUtf8(int codepoint);

    struct PixelColor {
        uint8_t r, g, b;
    };
    
    class VideoDecoder {
        AVFormatContext* formatContext;
        AVCodecContext* codecContext;
        AVStream* videoStream;
        SwsContext* swsContext;
        AVFrame* frame;
        AVPacket* packet;
        int videoStreamIndex;
        double fps;
        double duration;
        bool initialized;
    
    public:
        VideoDecoder();
        ~VideoDecoder();
    
        bool open(const std::string& path);
        void close();
        bool getNextFrame(std::vector<uint8_t>& frameData, int& width, int& height, bool& isEndOfFile);
        
        double getFPS() const;
        double getDuration() const;
    };
    
    // 添加显示配置结构体
    struct DisplayConfig {
        float scaleRatio = 1.0f;      // 缩放比例
        float gamma = 0.85f;          // 伽马校正
        float contrast = 1.2f;        // 对比度增强
        float brightness = 1.1f;      // 亮度调整
        bool useHighResolution = true; // 是否使用高分辨率模式
    };

    class VideoPlayerUI {
    public:
         // 添加新的配置成员
        static DisplayConfig displayConfig;
        
        static void calculateDisplayDimensions(int originalWidth, int originalHeight,
            int maxWidth, int maxHeight,
            int& displayWidth, int& displayHeight);
        
        static std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>
        convertToColorAsciiArt(const std::vector<uint8_t>& pixels,
                              int originalWidth, int originalHeight,
                              int displayWidth, int displayHeight,
                              float scaleRatio);
        
        // 新增渲染子图像的方法
        static ftxui::Element renderSubImage(
            const std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>& fullAsciiArt,
            int offset_x, int offset_y,
            int viewportWidth, int viewportHeight);
        
        static void startPlayback(VideoDecoder& decoder,
                                std::atomic<bool>& isPlaying,
                                std::atomic<bool>& shouldExit,
                                std::function<void(const std::vector<uint8_t>&, int, int)> onNewFrame);
        
        static void playVideo(const std::string& videoPath, 
                            ftxui::ScreenInteractive& screen);
    private:
        using AsciiFrame = std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>;
        static float calculateBrightness(const PixelColor& color);
        static std::vector<std::pair<int, float>> getUnicodeCharSet();
        // 添加新的辅助方法
        static std::vector<std::pair<int, float>> getExtendedUnicodeCharSet();
        static PixelColor enhanceColor(const PixelColor& color);
    };
    
}; // namespace VideoPlayer