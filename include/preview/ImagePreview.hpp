#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace FTB {

// 单个像素的 RGB 值
struct Pixel {
    uint8_t r, g, b;
};

// 一行图片渲染数据: 每个字符位置对应一个像素 (使用 █ 整块字符)
struct ImageLine {
    std::vector<Pixel> pixels;   // 每个字符位置的像素颜色
};

// ---- 图片预览缓存 ----
struct ImageCache {
    std::string key;                        // 文件路径作为缓存键
    std::vector<ImageLine> image_lines;     // 渲染后的像素行数据
    int width = 0;                          // 渲染宽度(字符数)
    int height = 0;                         // 渲染高度(字符行数)
    bool loaded = false;
    bool is_image = false;                  // 是否为可识别的图片
};

// ---- 图片预览渲染器 ----
class ImagePreview {
public:
    // 检查文件是否为支持的图片格式
    static bool IsImageFile(const std::string& path);

    // 渲染图片为像素数据 (使用 Unicode ▀ 块字符)
    static std::vector<ImageLine> RenderToPixels(
        const std::string& path,
        int max_width,
        int max_height
    );

    // 获取缓存的图片预览
    static bool GetCached(const std::string& path, ImageCache& cache);

    // 异步加载图片预览
    static void LoadAsync(const std::string& path, int max_width, int max_height);

private:
    // 使用 stb_image 加载并渲染为像素数据
    static std::vector<ImageLine> RenderWithStbImage(
        const std::string& path,
        int max_width,
        int max_height
    );

#ifdef FTB_ENABLE_LIBCHAFA
    // 使用 libchafa 渲染
    static std::vector<ImageLine> RenderWithChafa(
        const std::string& path,
        int max_width,
        int max_height
    );
#endif

    static std::mutex s_cache_mutex;
    static ImageCache s_cache;
};

}  // namespace FTB
