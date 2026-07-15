#include "../../include/utils/WebpDecoder.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

extern "C" {
#include "webp/decode.h"
}

namespace FTB {

WebpDecodeResult WebpDecoder::Decode(const std::string& path) {
    WebpDecodeResult result;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return result;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return result;

    result = Decode(buffer.data(), buffer.size());
    return result;
}

WebpDecodeResult WebpDecoder::Decode(const unsigned char* data, size_t size) {
    WebpDecodeResult result;

    WebPBitstreamFeatures features;
    if (WebPGetFeatures(data, size, &features) != VP8_STATUS_OK) return result;

    int width = 0, height = 0;
    unsigned char* rgb = WebPDecodeRGB(data, size, &width, &height);
    if (!rgb) return result;

    size_t pixel_count = static_cast<size_t>(width) * height * 3;
    result.pixels.assign(rgb, rgb + pixel_count);
    result.width = width;
    result.height = height;
    result.success = true;

    WebPFree(rgb);
    return result;
}

bool WebpDecoder::IsWebpExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".webp";
}

}  // namespace FTB
