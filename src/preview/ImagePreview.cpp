#include "../../include/preview/ImagePreview.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <thread>

#include "../../include/utils/PerfLogger.hpp"

// stb_image 实现
#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

#ifdef FTB_ENABLE_LIBCHAFA
#include <chafa.h>
#endif

namespace FTB {

// ---- 静态成员 ----
std::mutex ImagePreview::s_cache_mutex;
ImageCache ImagePreview::s_cache;

// ---- 检查是否为图片文件 ----
bool ImagePreview::IsImageFile(const std::string& path) {
    namespace fs = std::filesystem;
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::vector<std::string> image_exts = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga", ".psd",
        ".pic", ".pnm", ".hdr", ".webp"
    };

    for (const auto& e : image_exts) {
        if (ext == e) return true;
    }
    return false;
}

// ---- 使用 stb_image 渲染为像素数据 ----
std::vector<ImageLine> ImagePreview::RenderWithStbImage(
    const std::string& path,
    int max_width,
    int max_height
) {
    PERF_SCOPE("preview");
    int img_w, img_h, channels;
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);

    if (!data) {
        return {};
    }

    // Character cells are ~2:1 (taller than wide), so for correct visual
    // proportions we need: render_w / (render_h * 2) = img_w / img_h
    double target_ratio = 2.0 * img_w / img_h;
    int render_w, render_h;
    if (static_cast<double>(max_width) / max_height > target_ratio) {
        render_h = max_height;
        render_w = std::max(1, static_cast<int>(max_height * target_ratio));
    } else {
        render_w = max_width;
        render_h = std::max(1, static_cast<int>(max_width / target_ratio));
    }

    double scale_x = static_cast<double>(render_w) / img_w;
    double scale_y = static_cast<double>(render_h) / img_h;

    std::vector<ImageLine> lines;
    lines.reserve(render_h);

    for (int y = 0; y < render_h; ++y) {
        ImageLine line;
        line.pixels.reserve(render_w);

        for (int x = 0; x < render_w; ++x) {
            int src_x = std::min(static_cast<int>(x / scale_x), img_w - 1);
            int src_y = std::min(static_cast<int>(y / scale_y), img_h - 1);
            int idx = (src_y * img_w + src_x) * 3;
            line.pixels.push_back({data[idx], data[idx + 1], data[idx + 2]});
        }

        lines.push_back(std::move(line));
    }

    stbi_image_free(data);
    return lines;
}

#ifdef FTB_ENABLE_LIBCHAFA
std::vector<ImageLine> ImagePreview::RenderWithChafa(
    const std::string& path,
    int max_width,
    int max_height
) {
    int img_w, img_h, channels;
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);
    if (!data) return {};

    // Character cells are ~2:1 (taller than wide), so for correct visual
    // proportions we need: render_w / (render_h * 2) = img_w / img_h
    double target_ratio = 2.0 * img_w / img_h;
    int render_w, render_h;
    if (static_cast<double>(max_width) / max_height > target_ratio) {
        render_h = max_height;
        render_w = std::max(1, static_cast<int>(max_height * target_ratio));
    } else {
        render_w = max_width;
        render_h = std::max(1, static_cast<int>(max_width / target_ratio));
    }

    ChafaSymbolMap* sym_map = chafa_symbol_map_new();
    chafa_symbol_map_add_by_tags(sym_map, CHAFA_SYMBOL_TAG_BLOCK);
    chafa_symbol_map_add_by_tags(sym_map, CHAFA_SYMBOL_TAG_SPACE);

    ChafaCanvasConfig* config = chafa_canvas_config_new();
    chafa_canvas_config_set_geometry(config, render_w, render_h);
    chafa_canvas_config_set_symbol_map(config, sym_map);

    ChafaCanvas* canvas = chafa_canvas_new(config);
    chafa_canvas_draw_all_pixels(canvas,
        CHAFA_PIXEL_RGB8, data, img_w, img_h, img_w * 3);

    auto unpack = [](gint c) -> Pixel {
        if (c < 0) return {0, 0, 0};
        return {
            static_cast<uint8_t>((c >> 16) & 0xFF),
            static_cast<uint8_t>((c >> 8) & 0xFF),
            static_cast<uint8_t>(c & 0xFF)
        };
    };

    std::vector<ImageLine> lines;
    lines.reserve(render_h);

    for (int y = 0; y < render_h; ++y) {
        ImageLine line;
        line.pixels.reserve(render_w);

        for (int x = 0; x < render_w; ++x) {
            gunichar ch = chafa_canvas_get_char_at(canvas, x, y);
            gint fg, bg;
            chafa_canvas_get_raw_colors_at(canvas, x, y, &fg, &bg);

            Pixel p;
            if (ch == 0x0020) {
                p = unpack(bg);
            } else {
                p = unpack(fg >= 0 ? fg : bg);
            }
            line.pixels.push_back(p);
        }

        lines.push_back(std::move(line));
    }

    chafa_canvas_unref(canvas);
    chafa_canvas_config_unref(config);
    chafa_symbol_map_unref(sym_map);
    stbi_image_free(data);

    return lines;
}
#endif

// ---- 渲染入口 ----
std::vector<ImageLine> ImagePreview::RenderToPixels(
    const std::string& path,
    int max_width,
    int max_height
) {
    PERF_SCOPE("preview");
#ifdef FTB_ENABLE_LIBCHAFA
    return RenderWithChafa(path, max_width, max_height);
#else
    return RenderWithStbImage(path, max_width, max_height);
#endif
}

// ---- 获取缓存 ----
bool ImagePreview::GetCached(const std::string& path, ImageCache& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    if (s_cache.key == path && s_cache.loaded) {
        cache = s_cache;
        return true;
    }
    return false;
}

// ---- 异步加载 ----
void ImagePreview::LoadAsync(const std::string& path, int max_width, int max_height) {
    PERF_LOG("preview", "ImagePreview::LoadAsync: " + path);
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        if (s_cache.key == path && s_cache.loaded) return;
        s_cache.key = path;
        s_cache.loaded = false;
    }

    std::thread([path, max_width, max_height]() {
        PERF_LOG("preview", "ImagePreview thread start: " + path);
        auto lines = RenderToPixels(path, max_width, max_height);
        PERF_LOG("preview", "ImagePreview thread done: " + path);
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_cache.image_lines = std::move(lines);
        s_cache.loaded = true;
        s_cache.is_image = !s_cache.image_lines.empty();
        if (!s_cache.image_lines.empty()) {
            s_cache.width = static_cast<int>(s_cache.image_lines[0].pixels.size());
            s_cache.height = static_cast<int>(s_cache.image_lines.size());
        }
    }).detach();
}

}  // namespace FTB
