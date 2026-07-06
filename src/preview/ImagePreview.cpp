#include "../../include/preview/ImagePreview.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <thread>
#include <vector>


#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

#include "../../third_party/stb_image_resize2.h"

#ifdef FTB_ENABLE_LIBCHAFA
#include <chafa.h>
#endif

namespace FTB {

std::mutex ImagePreview::s_cache_mutex;
std::list<ImageCacheEntry> ImagePreview::s_lru_list;
using ImageLRUIter = std::list<ImageCacheEntry>::iterator;
std::unordered_map<std::string, ImageLRUIter> ImagePreview::s_cache_map;

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

std::vector<ImageLine> ImagePreview::RenderWithStbImage(
    const std::string& path,
    int max_width,
    int max_height
) {
    int img_w, img_h, channels;
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);

    if (!data) {
        return {};
    }

    double target_ratio = 2.0 * img_w / img_h;
    int render_w, render_h;
    if (static_cast<double>(max_width) / max_height > target_ratio) {
        render_h = max_height;
        render_w = std::max(1, static_cast<int>(max_height * target_ratio));
    } else {
        render_w = max_width;
        render_h = std::max(1, static_cast<int>(max_width / target_ratio));
    }

    int pixel_h = render_h * 2;
    std::vector<unsigned char> scaled(render_w * pixel_h * 3);

    stbir_resize_uint8_srgb(
        data, img_w, img_h, 0,
        scaled.data(), render_w, pixel_h, 0,
        STBIR_RGB);

    stbi_image_free(data);

    std::vector<ImageLine> lines;
    lines.reserve(render_h);

    for (int y = 0; y < render_h; ++y) {
        ImageLine line;
        line.pixels.reserve(render_w);
        line.half_block_bottom.reserve(render_w);

        int top_row = y * 2;
        int bot_row = y * 2 + 1;

        for (int x = 0; x < render_w; ++x) {
            int top_idx = (top_row * render_w + x) * 3;
            int bot_idx = (bot_row * render_w + x) * 3;

            line.pixels.push_back({
                scaled[top_idx + 0],
                scaled[top_idx + 1],
                scaled[top_idx + 2],
            });
            line.half_block_bottom.push_back({
                scaled[bot_idx + 0],
                scaled[bot_idx + 1],
                scaled[bot_idx + 2],
            });
        }

        lines.push_back(std::move(line));
    }

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

std::vector<ImageLine> ImagePreview::RenderToPixels(
    const std::string& path,
    int max_width,
    int max_height
) {
#ifdef FTB_ENABLE_LIBCHAFA
    return RenderWithChafa(path, max_width, max_height);
#else
    return RenderWithStbImage(path, max_width, max_height);
#endif
}

bool ImagePreview::GetCached(const std::string& path, ImageCacheEntry& cache) {
    std::lock_guard<std::mutex> lock(s_cache_mutex);
    auto it = s_cache_map.find(path);
    if (it != s_cache_map.end() && it->second->loaded) {
        cache = *it->second;
        s_lru_list.splice(s_lru_list.begin(), s_lru_list, it->second);
        return true;
    }
    return false;
}

void ImagePreview::LoadAsync(const std::string& path, int max_width, int max_height) {
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_cache_map.find(path);
        if (it != s_cache_map.end() && it->second->loaded) {
            if (it->second->failed) {
                return;
            }
            s_lru_list.splice(s_lru_list.begin(), s_lru_list, it->second);
            return;
        }
        if (it != s_cache_map.end() && it->second->failed) {
            return;
        }
        if (s_cache_map.size() >= static_cast<size_t>(kImageCacheMaxEntries)) {
            auto oldest = std::prev(s_lru_list.end());
            s_cache_map.erase(oldest->key);
            s_lru_list.erase(oldest);
        }
        s_lru_list.emplace_front();
        s_lru_list.front().key = path;
        s_lru_list.front().loaded = false;
        s_cache_map[path] = s_lru_list.begin();
    }

    std::thread([path, max_width, max_height]() {
        auto lines = RenderToPixels(path, max_width, max_height);
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_cache_map.find(path);
        if (it == s_cache_map.end()) {
            return;
        }
        it->second->image_lines = std::move(lines);
        it->second->loaded = true;
        it->second->is_image = !it->second->image_lines.empty();
        it->second->failed = it->second->image_lines.empty();
        if (!it->second->image_lines.empty()) {
            it->second->width = static_cast<int>(it->second->image_lines[0].pixels.size());
            it->second->height = static_cast<int>(it->second->image_lines.size());
        }
        s_lru_list.splice(s_lru_list.begin(), s_lru_list, it->second);
    }).detach();
}

}  // namespace FTB
