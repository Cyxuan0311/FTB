#pragma once

#include <functional>
#include <string>
#include <vector>
#include <cstdint>

namespace FTB {

struct DecodedFrame {
    std::vector<unsigned char> rgba;
    int width = 0;
    int height = 0;
    int delay_ms = 100;
};

class GifFrameDecoder {
public:
    using FrameCallback = std::function<bool(int index, DecodedFrame& frame)>;

    static bool Decode(const std::string& path,
                       int max_w, int max_h,
                       FrameCallback callback,
                       int& out_w, int& out_h);
};

}
