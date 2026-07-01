#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace FTB {
namespace detail {

inline std::string Base64Encode(const unsigned char* data, size_t len) {
    static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int val = static_cast<unsigned int>(
            (static_cast<unsigned int>(data[i]) << 16) |
            (i + 1 < len ? (static_cast<unsigned int>(data[i + 1]) << 8) : 0) |
            (i + 2 < len ? static_cast<unsigned int>(data[i + 2]) : 0));
        out.push_back(enc[(val >> 18) & 0x3F]);
        out.push_back(enc[(val >> 12) & 0x3F]);
        out.push_back(i + 1 < len ? enc[(val >> 6) & 0x3F] : '=');
        out.push_back(i + 2 < len ? enc[val & 0x3F] : '=');
    }
    return out;
}

}  // namespace detail
}  // namespace FTB
