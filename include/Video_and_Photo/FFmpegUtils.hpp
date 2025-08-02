#ifndef FFMPEG_UTILS_HPP
#define FFMPEG_UTILS_HPP

#include <string>
#include <vector>
#include <memory>

// FFmpeg C库头文件
extern "C" {
#include <libavcodec/avcodec.h>    // 编解码相关
#include <libavformat/avformat.h>  // 格式处理相关
#include <libswscale/swscale.h>    // 图像缩放和像素格式转换
}

namespace FFmpegUtils {

// FFmpeg上下文封装类，用于处理视频文件
class FFmpegContext {
public:
    FFmpegContext();  // 构造函数
    ~FFmpegContext(); // 析构函数

    // 打开视频文件
    bool open(const std::string& path);
    // 关闭视频文件并释放资源
    void close();
    // 解码一帧视频
    bool decodeFrame(std::vector<uint8_t>& frameData, int& width, int& height);
    // 获取下一帧视频数据
    bool getNextFrame(std::vector<uint8_t>& frameData, int& width, int& height, bool& isEndOfFile);
    // 获取视频帧率
    double getFPS() const;
    // 获取视频总时长(秒)
    double getDuration() const;

private:
    // FFmpeg上下文对象，使用智能指针管理生命周期
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> formatContext;  // 格式上下文
    std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)> codecContext;     // 编解码上下文
    std::unique_ptr<AVFrame, void(*)(AVFrame*)> frame;                         // 帧对象
    std::unique_ptr<AVPacket, void(*)(AVPacket*)> packet;                      // 数据包对象
    
    SwsContext* swsContext;       // 图像缩放转换上下文
    AVStream* videoStream;        // 视频流
    int videoStreamIndex;         // 视频流索引
    double fps;                   // 帧率
    double duration;              // 视频总时长(秒)
    bool initialized;             // 是否已初始化标志
};

} // namespace FFmpegUtils

#endif // FFMPEG_UTILS_HPP
