#include "Video_and_Photo/FFmpegUtils.hpp"
#include <iostream>

namespace FFmpegUtils {

// --------------------------------------------------
// 构造函数：使用智能指针管理 FFmpeg 资源，避免手动释放引发内存泄漏
// 初始化成员变量：
//   formatContext - 管理 AVFormatContext，析构时调用 avformat_close_input
//   codecContext  - 管理 AVCodecContext，析构时调用 avcodec_free_context
//   frame         - 管理 AVFrame，析构时调用 av_frame_free
//   packet        - 管理 AVPacket，析构时调用 av_packet_free
//   swsContext    - 默认为 nullptr，后续用于像素格式转换
//   videoStream   - AVStream*，后续指向所选视频流
//   videoStreamIndex - 视频流索引，初始化为 -1 表示未找到
//   fps           - 帧率，初始化为 0.0
//   duration      - 视频时长（秒），初始化为 0.0
//   initialized   - 是否已成功初始化，初始化为 false
// --------------------------------------------------
FFmpegContext::FFmpegContext()
    : formatContext(nullptr, [](AVFormatContext* f) {
          if (f) avformat_close_input(&f); // 自动关闭并释放 AVFormatContext
      })
    , codecContext(nullptr, [](AVCodecContext* c) {
          if (c) avcodec_free_context(&c); // 自动释放 AVCodecContext
      })
    , frame(nullptr, [](AVFrame* f) {
          if (f) av_frame_free(&f); // 自动释放 AVFrame
      })
    , packet(nullptr, [](AVPacket* p) {
          if (p) av_packet_free(&p); // 自动释放 AVPacket
      })
    , swsContext(nullptr)       // 图像转换上下文，稍后初始化
    , videoStream(nullptr)      // 指向 AVStream，后续获取视频流
    , videoStreamIndex(-1)      // 视频流索引， -1 表示暂未找到
    , fps(0.0)                  // 帧率，后续从流信息获取
    , duration(0.0)             // 视频时长（秒），后续从流信息获取
    , initialized(false)        // 是否已成功调用 open()，初始为 false
{}

// --------------------------------------------------
// 析构函数：在对象生命周期结束时释放所有 FFmpeg 资源
// 通过智能指针自动调用各资源的析构操作
// --------------------------------------------------
FFmpegContext::~FFmpegContext() {
    close();
}

// --------------------------------------------------
// 打开视频文件并初始化解码资源
// 返回：若成功打开并初始化返回 true，否则返回 false
//
// 步骤：
//  1. 调用 avformat_open_input 打开视频文件，获取 AVFormatContext
//  2. 调用 avformat_find_stream_info 解析流信息
//  3. 调用 av_find_best_stream 找到最佳的视频流索引
//  4. 从 AVStream 中获取解码器 ID，并调用 avcodec_find_decoder 查找 AVCodec
//  5. 分配 AVCodecContext 并调用 avcodec_parameters_to_context 复制参数
//  6. 打开解码器 avcodec_open2
//  7. 分配 AVFrame 与 AVPacket，用于后续解码
//  8. 从流信息中提取帧率 (r_frame_rate) 与时长 (time_base * duration)
//  9. 标记 initialized 为 true
// --------------------------------------------------
bool FFmpegContext::open(const std::string& path) {
    AVFormatContext* fmt_ctx = nullptr;

    // 1. 打开输入文件
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "❌ 无法打开输入文件: " << path << std::endl;
        return false;
    }
    // 使用智能指针管理 AVFormatContext，确保资源释放
    formatContext.reset(fmt_ctx);

    // 2. 查找流信息
    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) {
        std::cerr << "❌ 无法找到流信息" << std::endl;
        return false;
    }

    // 3. 查找最优视频流
    videoStreamIndex = av_find_best_stream(formatContext.get(),
                                           AVMEDIA_TYPE_VIDEO,
                                           -1, -1, nullptr, 0);
    if (videoStreamIndex < 0) {
        std::cerr << "❌ 无法找到视频流" << std::endl;
        return false;
    }
    // 获取对应 AVStream 指针
    videoStream = formatContext->streams[videoStreamIndex];

    // 4. 查找视频解码器
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "❌ 无法找到解码器" << std::endl;
        return false;
    }

    // 5. 分配解码器上下文
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "❌ 无法分配解码器上下文" << std::endl;
        return false;
    }
    codecContext.reset(codec_ctx);

    // 6. 将流参数复制到 codecContext 中
    if (avcodec_parameters_to_context(codecContext.get(), videoStream->codecpar) < 0) {
        std::cerr << "❌ 无法复制解码器参数" << std::endl;
        return false;
    }

    // 7. 打开解码器
    if (avcodec_open2(codecContext.get(), codec, nullptr) < 0) {
        std::cerr << "❌ 无法打开解码器" << std::endl;
        return false;
    }

    // 8. 分配 AVFrame 与 AVPacket
    frame.reset(av_frame_alloc());
    packet.reset(av_packet_alloc());
    if (!frame || !packet) {
        std::cerr << "❌ 无法分配帧或包内存" << std::endl;
        return false;
    }

    // 9. 从流信息计算帧率和时长
    fps = av_q2d(videoStream->r_frame_rate);                                           // 帧率 = r_frame_rate
    duration = av_q2d(videoStream->time_base) * videoStream->duration;                 // 时长 = time_base * duration

    initialized = true; // 标记初始化成功
    return true;
}

// --------------------------------------------------
// 关闭并释放所有 FFmpeg 资源
// 步骤：
//  - 如果 swsContext 不为空，调用 sws_freeContext 释放
//  - 重置智能指针（AVFrame, AVPacket, AVCodecContext, AVFormatContext）
//  - 清空视频流指针与相关信息，重置初始化标志
// --------------------------------------------------
void FFmpegContext::close() {
    // 释放像素格式转换上下文
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    // 重置智能指针，依次释放各类资源
    frame.reset();
    packet.reset();
    codecContext.reset();
    formatContext.reset();

    // 清除视频流指针与相关信息
    videoStream = nullptr;
    videoStreamIndex = -1;
    fps = 0.0;
    duration = 0.0;
    initialized = false;
}

// --------------------------------------------------
// 解码一个视频帧并转换为 RGB24 格式
// 参数：
//   frameData - 输出参数，存储解码后 RGB24 像素数据
//   width     - 输出参数，帧宽度（像素）
//   height    - 输出参数，帧高度（像素）
// 返回：
//   如果成功解码并转换返回 true，否则返回 false
// 处理流程：
//  1. 循环读取 AVPacket，直到找到属于视频流的包
//  2. 将包发送给解码器 (avcodec_send_packet)
//  3. 调用 avcodec_receive_frame 从解码器获取 AVFrame
//  4. 若 swsContext 尚未初始化，则根据原帧尺寸与像素格式创建 swsContext
//  5. 使用 sws_scale 将源像素格式转换为 RGB24，填充至 frameData
// --------------------------------------------------
bool FFmpegContext::decodeFrame(std::vector<uint8_t>& frameData,
                                int& width,
                                int& height) 
{
    if (!initialized) return false;

    // 1. 循环读取包，直到找到视频流对应的包
    while (av_read_frame(formatContext.get(), packet.get()) >= 0) {
        // 仅处理视频流包
        if (packet->stream_index == videoStreamIndex) {
            // 2. 发送包到解码器
            if (avcodec_send_packet(codecContext.get(), packet.get()) < 0) {
                return false;
            }

            // 3. 从解码器接收解码后的帧
            int ret = avcodec_receive_frame(codecContext.get(), frame.get());
            if (ret == AVERROR(EAGAIN)) {
                // 解码器需要更多数据才能输出帧，继续读取
                continue;
            }
            if (ret < 0) {
                return false;
            }

            // 获取解码后的帧宽度和高度
            width = frame->width;
            height = frame->height;

            // 4. 如果尚未创建 swsContext，则初始化它
            if (!swsContext) {
                swsContext = sws_getContext(
                    width, height, codecContext->pix_fmt, // 源像素格式
                    width, height, AV_PIX_FMT_RGB24,      // 目标像素格式
                    SWS_BILINEAR, nullptr, nullptr, nullptr  // 双线性插值
                );
                if (!swsContext) {
                    return false;
                }
            }

            // 5. 将 AVFrame 中的数据转换为 RGB24，存储到 frameData
            frameData.resize(width * height * 3); // 3 字节 (R,G,B) * 像素数
            uint8_t* dst_data[4]   = { frameData.data(), nullptr, nullptr, nullptr };
            int dst_linesize[4]    = { width * 3, 0, 0, 0 };

            sws_scale(swsContext,
                      frame->data, frame->linesize, 0, height,
                      dst_data, dst_linesize);

            return true;
        }
    }
    // 未找到更多视频帧或已到达文件结尾
    return false;
}

// --------------------------------------------------
// 获取下一帧函数：与 decodeFrame 类似，但区分文件尾状态
// 参数：
//   frameData    - 输出参数，存储解码后 RGB24 像素数据
//   width        - 输出参数，帧宽度（像素）
//   height       - 输出参数，帧高度（像素）
//   isEndOfFile  - 输出参数，若到达输入文件尾设置为 true
// 返回：
//   若成功解码一帧返回 true，若到达文件尾返回 false 并将 isEndOfFile = true，
//   其他错误则直接返回 false
// --------------------------------------------------
bool FFmpegContext::getNextFrame(std::vector<uint8_t>& frameData,
                                 int& width,
                                 int& height,
                                 bool& isEndOfFile) 
{
    if (!initialized) return false;
    isEndOfFile = false;

    // 从输入中读取一个包
    int ret = av_read_frame(formatContext.get(), packet.get());
    if (ret < 0) {
        // 读取失败（常见于文件结尾），标记为 EOF
        isEndOfFile = true;
        return false;
    }

    // 若该包属于视频流，则尝试解码
    if (packet->stream_index == videoStreamIndex) {
        if (avcodec_send_packet(codecContext.get(), packet.get()) >= 0) {
            ret = avcodec_receive_frame(codecContext.get(), frame.get());
            if (ret >= 0) {
                // 获取解码后帧的尺寸
                width = frame->width;
                height = frame->height;

                // 若尚未创建 swsContext，则初始化
                if (!swsContext) {
                    swsContext = sws_getContext(
                        width, height, codecContext->pix_fmt,
                        width, height, AV_PIX_FMT_RGB24,
                        SWS_BILINEAR, nullptr, nullptr, nullptr
                    );
                }

                // 如果转换上下文存在，则转换像素数据到 RGB24
                if (swsContext) {
                    frameData.resize(width * height * 3);
                    uint8_t* dst_data[4]   = { frameData.data(), nullptr, nullptr, nullptr };
                    int dst_linesize[4]    = { width * 3, 0, 0, 0 };

                    sws_scale(swsContext,
                              frame->data, frame->linesize, 0, height,
                              dst_data, dst_linesize);
                    return true;
                }
            }
        }
    }
    // 若非视频流包或解码失败，则返回 false
    return false;
}

// --------------------------------------------------
// 获取帧率 (FPS)
// 返回：
//   存储在成员 fps 中的值，若未初始化则为 0.0
// --------------------------------------------------
double FFmpegContext::getFPS() const {
    return fps;
}

// --------------------------------------------------
// 获取视频时长（秒）
// 返回：
//   存储在成员 duration 中的值，若未初始化则为 0.0
// --------------------------------------------------
double FFmpegContext::getDuration() const {
    return duration;
}

} // namespace FFmpegUtils
