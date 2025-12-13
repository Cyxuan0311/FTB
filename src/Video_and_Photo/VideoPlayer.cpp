#include "Video_and_Photo/VideoPlayer.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace VideoPlayer
{

    DisplayConfig VideoPlayerUI::displayConfig;

    // 实现 UTF-8 转换函数
    std::string codepointToUtf8(int codepoint)
    {
        std::string utf8;
        if (codepoint < 0x80)
        {
            utf8.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint < 0x800)
        {
            utf8.push_back(static_cast<char>((codepoint >> 6) | 0xC0));
            utf8.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
        }
        else if (codepoint < 0x10000)
        {
            utf8.push_back(static_cast<char>((codepoint >> 12) | 0xE0));
            utf8.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
            utf8.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
        }
        return utf8;
    }

    // 实现私有辅助方法
    float VideoPlayerUI::calculateBrightness(const PixelColor& color)
    {
        // 参考 dsa 项目的 RGB 到灰度转换算法
        // 使用标准亮度系数：0.299∙R + 0.587∙G + 0.114∙B（与 dsa 一致）
        // 这比 Rec. 709 更适合终端显示
        return (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
    }

    std::vector<std::pair<int, float>> VideoPlayerUI::getExtendedUnicodeCharSet()
    {
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

    PixelColor VideoPlayerUI::enhanceColor(const PixelColor& color)
    {
        auto enhance = [](uint8_t value, float contrast, float brightness, float gamma) -> uint8_t {
            float normalized = value / 255.0f;
            normalized       = std::pow(normalized, gamma);            // 伽马校正
            normalized       = (normalized - 0.5f) * contrast + 0.5f;  // 对比度调整
            normalized       = normalized * brightness;                // 亮度调整
            return static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
        };

        return {
            enhance(color.r, displayConfig.contrast, displayConfig.brightness, displayConfig.gamma),
            enhance(color.g, displayConfig.contrast, displayConfig.brightness, displayConfig.gamma),
            enhance(color.b, displayConfig.contrast, displayConfig.brightness,
                    displayConfig.gamma)};
    }

    // =============== VideoDecoder 实现 ===============

    VideoDecoder::VideoDecoder()
        : formatContext(nullptr),
          codecContext(nullptr),
          videoStream(nullptr),
          swsContext(nullptr),
          frame(nullptr),
          packet(nullptr),
          videoStreamIndex(-1),
          fps(0.0),
          duration(0.0),
          initialized(false)
    {}

    VideoDecoder::~VideoDecoder()
    {
        close();
    }

    bool VideoDecoder::open(const std::string& path)
    {
        if (avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr) < 0)
        {
            std::cerr << "❌ 无法打开视频文件" << std::endl;
            return false;
        }

        if (avformat_find_stream_info(formatContext, nullptr) < 0)
        {
            avformat_close_input(&formatContext);
            return false;
        }

        videoStreamIndex =
            av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoStreamIndex < 0)
        {
            avformat_close_input(&formatContext);
            return false;
        }

        videoStream          = formatContext->streams[videoStreamIndex];
        const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (!codec)
        {
            avformat_close_input(&formatContext);
            return false;
        }

        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext)
        {
            avformat_close_input(&formatContext);
            return false;
        }

        if (avcodec_parameters_to_context(codecContext, videoStream->codecpar) < 0)
        {
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return false;
        }

        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            avcodec_free_context(&codecContext);
            avformat_close_input(&formatContext);
            return false;
        }

        frame  = av_frame_alloc();
        packet = av_packet_alloc();
        if (!frame || !packet)
        {
            close();
            return false;
        }

        fps      = av_q2d(videoStream->r_frame_rate);
        duration = av_q2d(videoStream->time_base) * videoStream->duration;

        initialized = true;
        return true;
    }

    void VideoDecoder::close()
    {
        if (swsContext)
        {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }
        if (frame)
        {
            av_frame_free(&frame);
        }
        if (packet)
        {
            av_packet_free(&packet);
        }
        if (codecContext)
        {
            avcodec_free_context(&codecContext);
        }
        if (formatContext)
        {
            avformat_close_input(&formatContext);
        }
        initialized = false;
    }

    bool VideoDecoder::getNextFrame(std::vector<uint8_t>& frameData, int& width, int& height,
                                    bool& isEndOfFile)
    {
        if (!initialized)
            return false;

        isEndOfFile = false;
        while (true)
        {
            int ret = av_read_frame(formatContext, packet);
            if (ret < 0)
            {
                isEndOfFile = true;
                return false;
            }

            if (packet->stream_index == videoStreamIndex)
            {
                ret = avcodec_send_packet(codecContext, packet);
                if (ret < 0)
                {
                    av_packet_unref(packet);
                    return false;
                }

                ret = avcodec_receive_frame(codecContext, frame);
                if (ret == AVERROR(EAGAIN))
                {
                    av_packet_unref(packet);
                    continue;
                }
                if (ret < 0)
                {
                    av_packet_unref(packet);
                    return false;
                }

                width  = frame->width;
                height = frame->height;

                if (!swsContext)
                {
                    swsContext =
                        sws_getContext(width, height, codecContext->pix_fmt, width, height,
                                       AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
                    if (!swsContext)
                    {
                        av_packet_unref(packet);
                        return false;
                    }
                }

                frameData.resize(width * height * 3);
                uint8_t* dst_data[4]     = {frameData.data(), nullptr, nullptr, nullptr};
                int      dst_linesize[4] = {width * 3, 0, 0, 0};

                sws_scale(swsContext, frame->data, frame->linesize, 0, height, dst_data,
                          dst_linesize);

                av_packet_unref(packet);
                return true;
            }
            av_packet_unref(packet);
        }
        return false;
    }

    double VideoDecoder::getFPS() const
    {
        return fps;
    }

    double VideoDecoder::getDuration() const
    {
        return duration;
    }

    // =============== VideoPlayerUI 实现 ===============

    void VideoPlayerUI::calculateDisplayDimensions(int originalWidth, int originalHeight,
                                                   int maxWidth, int maxHeight, int& displayWidth,
                                                   int& displayHeight)
    {
        const float TERMINAL_CHAR_ASPECT = 2.2f;
        float       effectiveAspect =
            static_cast<float>(originalWidth) / originalHeight * TERMINAL_CHAR_ASPECT;

        int option1Width  = maxWidth;
        int option1Height = static_cast<int>(maxWidth / effectiveAspect);

        int option2Height = maxHeight;
        int option2Width  = static_cast<int>(maxHeight * effectiveAspect);

        if (option1Width * option1Height > option2Width * option2Height)
        {
            displayWidth  = option1Width;
            displayHeight = option1Height;
        }
        else
        {
            displayWidth  = option2Width;
            displayHeight = option2Height;
        }

        displayWidth  = std::clamp(displayWidth, 20, maxWidth);
        displayHeight = std::clamp(displayHeight, 10, maxHeight);
    }

    // 实现新的色彩 ASCII 艺术转换方法
    std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>
    VideoPlayerUI::convertToColorAsciiArt(const std::vector<uint8_t>& pixels, int originalWidth,
                                          int originalHeight, int displayWidth, int displayHeight,
                                          float scaleRatio)
    {
        (void) scaleRatio;
        // 根据高分辨率模式调整块大小
        const int block_width  = displayConfig.useHighResolution ? 1 : 2;
        const int block_height = displayConfig.useHighResolution ? 1 : 2;

        int virtualWidth  = std::max(40, displayWidth / block_width);
        int virtualHeight = std::max(20, displayHeight / block_height);

        // 使用扩展字符集
        const auto& charSet = getExtendedUnicodeCharSet();

        // 实现双线性插值采样
        auto get_pixel = [&](float x, float y) -> PixelColor {
            float fx = x * originalWidth / virtualWidth;
            float fy = y * originalHeight / virtualHeight;

            int x1 = std::clamp(static_cast<int>(fx), 0, originalWidth - 1);
            int y1 = std::clamp(static_cast<int>(fy), 0, originalHeight - 1);
            int x2 = std::clamp(x1 + 1, 0, originalWidth - 1);
            int y2 = std::clamp(y1 + 1, 0, originalHeight - 1);

            float dx = fx - x1;
            float dy = fy - y1;

            auto sample = [&](int x, int y) -> PixelColor {
                int idx = (y * originalWidth + x) * 3;
                return {pixels[idx], pixels[idx + 1], pixels[idx + 2]};
            };

            auto c1 = sample(x1, y1);
            auto c2 = sample(x2, y1);
            auto c3 = sample(x1, y2);
            auto c4 = sample(x2, y2);

            return {static_cast<uint8_t>((1 - dx) * (1 - dy) * c1.r + dx * (1 - dy) * c2.r +
                                         (1 - dx) * dy * c3.r + dx * dy * c4.r),
                    static_cast<uint8_t>((1 - dx) * (1 - dy) * c1.g + dx * (1 - dy) * c2.g +
                                         (1 - dx) * dy * c3.g + dx * dy * c4.g),
                    static_cast<uint8_t>((1 - dx) * (1 - dy) * c1.b + dx * (1 - dy) * c2.b +
                                         (1 - dx) * dy * c3.b + dx * dy * c4.b)};
        };

        // 计算亮度值并进行对比度增强
        std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>> result;
        result.reserve(virtualHeight);

        const float gamma = 0.85f;  // 伽马校正值

        for (int y = 0; y < virtualHeight; ++y)
        {
            std::vector<std::string> row_chars;
            std::vector<PixelColor>  row_colors;
            row_chars.reserve(virtualWidth);
            row_colors.reserve(virtualWidth);

            for (int x = 0; x < virtualWidth; ++x)
            {
                auto color       = get_pixel(x, y);
                color            = enhanceColor(color);
                float brightness = calculateBrightness(color);
                brightness       = std::pow(brightness, gamma);

                // 完全按照 dsa 项目的字符选择算法
                // dsa 的 get_unicode_char: index = (gray_value * 3) / 255
                // 由于我们的 brightness 是归一化的 (0.0-1.0)，对应 gray_value/255
                // 所以：index = brightness * 3
                // dsa 中：index 0=░(最亮), 1=▒, 2=▓, 3=█(最暗)
                // 我们的字符集按亮度从暗到亮：0=█(最暗), 1=▓, 2=▒, 3=░(最亮)
                // 所以需要反转：我们的索引 = 3 - dsa_index
                int dsaIndex = static_cast<int>(brightness * 3.0f);
                dsaIndex = std::clamp(dsaIndex, 0, 3);
                // 反转索引以匹配我们的字符集顺序（从暗到亮）
                int charIndex = 3 - dsaIndex;
                charIndex = std::clamp(charIndex, 0, 3);
                
                int bestChar = charSet[charIndex].first;

                row_chars.push_back(codepointToUtf8(bestChar));
                row_colors.push_back(color);
            }

            result.emplace_back(std::move(row_chars), std::move(row_colors));
        }

        return result;
    }

    ftxui::Element VideoPlayerUI::renderSubImage(
        const std::vector<std::pair<std::vector<std::string>, std::vector<PixelColor>>>&
            fullAsciiArt,
        int offset_x, int offset_y, int viewportWidth, int viewportHeight)
    {
        if (fullAsciiArt.empty())
        {
            return ftxui::text("加载中...") | ftxui::center | ftxui::color(ftxui::Color::Red3Bis);
        }

        std::vector<ftxui::Element> lines;
        int endRow = std::min(offset_y + viewportHeight, static_cast<int>(fullAsciiArt.size()));

        for (int y = offset_y; y < endRow; ++y)
        {
            const auto&                 rowSymbols = fullAsciiArt[y].first;
            const auto&                 rowColors  = fullAsciiArt[y].second;
            std::vector<ftxui::Element> row;

            int endCol = std::min(offset_x + viewportWidth, static_cast<int>(rowSymbols.size()));
            for (int x = offset_x; x < endCol; ++x)
            {
                const auto& color = rowColors[x];
                row.push_back(ftxui::text(rowSymbols[x]) |
                              ftxui::color(ftxui::Color::RGB(color.r, color.g, color.b)));
            }

            lines.push_back(ftxui::hbox(std::move(row)));
        }

        return ftxui::vbox(std::move(lines)) | ftxui::flex;
    }

    void VideoPlayerUI::startPlayback(
        VideoDecoder& decoder, std::atomic<bool>& isPlaying, std::atomic<bool>& shouldExit,
        std::function<void(const std::vector<uint8_t>&, int, int)> onNewFrame)
    {
        const double targetFPS     = decoder.getFPS();
        const auto   frameDuration = std::chrono::duration<double>(1.0 / targetFPS);

        std::vector<uint8_t> frameData;
        int                  width, height;
        bool                 isEndOfFile;

        while (!shouldExit)
        {
            if (!isPlaying)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            auto frameStart = std::chrono::steady_clock::now();

            if (decoder.getNextFrame(frameData, width, height, isEndOfFile))
            {
                onNewFrame(frameData, width, height);
            }
            else if (isEndOfFile)
            {
                isPlaying = false;
                continue;
            }

            auto frameEnd    = std::chrono::steady_clock::now();
            auto processTime = frameEnd - frameStart;
            auto sleepTime   = frameDuration - processTime;

            if (sleepTime > std::chrono::duration<double>(0))
            {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    // 播放视频文件的主函数
    // 参数:
    //   videoPath - 视频文件路径
    //   screen - FTXUI交互式屏幕引用
    void VideoPlayerUI::playVideo(const std::string& videoPath, ftxui::ScreenInteractive& screen)
    {
        // 1. 初始化视频解码器
        VideoDecoder decoder;
        if (!decoder.open(videoPath))
        {
            std::cerr << "❌ 无法打开视频文件: " << videoPath << std::endl;
            return;
        }

        // 2. 配置显示参数
        displayConfig.scaleRatio        = 1.2f;   // 图像缩放比例
        displayConfig.gamma             = 0.85f;  // 伽马校正值
        displayConfig.contrast          = 1.2f;   // 对比度增强
        displayConfig.brightness        = 1.1f;   // 亮度调整
        displayConfig.useHighResolution = true;   // 高分辨率模式

        // 3. 获取终端窗口尺寸
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        const int UI_RESERVED_SPACE = 3;                             // 为UI控件预留的行数
        int       viewportWidth     = w.ws_col;                      // 视口宽度(字符列数)
        int       viewportHeight    = w.ws_row - UI_RESERVED_SPACE;  // 视口高度

        // 4. 播放控制状态
        std::atomic<bool> isPlaying(true);    // 播放/暂停状态
        std::atomic<bool> shouldExit(false);  // 退出标志

        // 5. 当前帧数据及互斥锁
        AsciiFrame currentFrame;  // 存储当前ASCII艺术帧
        std::mutex frameMutex;    // 保护帧数据的互斥锁

        // 6. 启动播放线程
        std::thread playbackThread([&] {
            startPlayback(decoder, isPlaying, shouldExit,
                          [&](const std::vector<uint8_t>& frameData, int width, int height) {
                              // 计算显示尺寸
                              int displayWidth, displayHeight;
                              calculateDisplayDimensions(width, height, viewportWidth,
                                                         viewportHeight, displayWidth,
                                                         displayHeight);

                              // 转换为ASCII艺术帧
                              auto asciiFrame =
                                  convertToColorAsciiArt(frameData, width, height, displayWidth,
                                                         displayHeight, displayConfig.scaleRatio);

                              // 更新当前帧(线程安全)
                              std::lock_guard<std::mutex> lock(frameMutex);
                              currentFrame = std::move(asciiFrame);
                              screen.PostEvent(ftxui::Event::Custom);  // 触发UI刷新
                          });
        });

        // 7. 构建UI组件
        auto component = ftxui::Renderer([&] {
            std::vector<ftxui::Element> elements;

            // 顶部控制栏
            elements.push_back(
                ftxui::hbox(
                    {// 视频图标和状态
                     ftxui::text("📽️ ▶ ") | ftxui::color(ftxui::Color::Green),
                     // 文件名显示
                     ftxui::text(std::filesystem::path(videoPath).filename().string()) |
                         ftxui::bold,
                     // 播放/暂停状态图标
                     ftxui::text(isPlaying ? "⏸" : "▶") | ftxui::color(ftxui::Color::Yellow),
                     // 退出提示
                     ftxui::text("⎋") | ftxui::color(ftxui::Color::Red)}) |
                ftxui::border);

            // 视频内容区域(线程安全访问)
            {
                std::lock_guard<std::mutex> lock(frameMutex);
                elements.push_back(
                    renderSubImage(currentFrame, 0, 0, viewportWidth, viewportHeight) |
                    ftxui::flex);
            }

            return ftxui::vbox(std::move(elements)) | ftxui::flex;
        });

        // 8. 事件处理
        auto main_component = ftxui::CatchEvent(component, [&](ftxui::Event event) {
            if (event == ftxui::Event::Escape)
            {  // ESC键退出
                shouldExit = true;
                screen.Exit();
                return true;
            }
            if (event == ftxui::Event::Character(' '))
            {  // 空格键暂停/播放
                isPlaying = !isPlaying;
                return true;
            }
            return false;
        });

        // 9. 启动主事件循环
        screen.Loop(main_component);

        // 10. 清理资源
        shouldExit = true;
        if (playbackThread.joinable())
        {
            playbackThread.join();  // 等待播放线程结束
        }
        decoder.close();  // 关闭解码器
    }

}  // namespace VideoPlayer