#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace FTB {

static constexpr int kImageCacheMaxEntries = 6;

struct Pixel {
    uint8_t r, g, b;
};

struct ImageLine {
    std::vector<Pixel> pixels;
    std::vector<Pixel> half_block_bottom;
};

struct ImageCacheEntry {
    std::string key;
    std::vector<ImageLine> image_lines;
    int width = 0;
    int height = 0;
    bool loaded = false;
    bool is_image = false;
    bool failed = false;
};

class ImagePreview {
public:
    static bool IsImageFile(const std::string& path);

    static std::vector<ImageLine> RenderToPixels(
        const std::string& path,
        int max_width,
        int max_height
    );

    static bool GetCached(const std::string& path, ImageCacheEntry& cache);

    static void LoadAsync(const std::string& path, int max_width, int max_height);

private:
    static std::vector<ImageLine> RenderWithStbImage(
        const std::string& path,
        int max_width,
        int max_height
    );

#ifdef FTB_ENABLE_LIBCHAFA
    static std::vector<ImageLine> RenderWithChafa(
        const std::string& path,
        int max_width,
        int max_height
    );
#endif

    static std::mutex s_cache_mutex;
    static std::list<ImageCacheEntry> s_lru_list;
    static std::unordered_map<std::string, decltype(s_lru_list)::iterator> s_cache_map;
};

using ImageCache = ImageCacheEntry;

}  // namespace FTB
