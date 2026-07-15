#pragma once

#include <string>
#include <vector>

namespace FTB {

struct WebpDecodeResult {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    bool success = false;
};

class WebpDecoder {
public:
    static WebpDecodeResult Decode(const std::string& path);
    static WebpDecodeResult Decode(const unsigned char* data, size_t size);

private:
    static bool IsWebpExtension(const std::string& path);
};

}  // namespace FTB
