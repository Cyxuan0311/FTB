#include "utils/UnicodeUtil.hpp"

namespace FTB {
namespace UnicodeUtil {

int DecodeUtf8(const char* s, uint32_t* codepoint) {
    unsigned char c = (unsigned char)*s;

    if (c < 0x80) {
        *codepoint = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        *codepoint = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        *codepoint = ((c & 0x0F) << 12) |
                     (((unsigned char)s[1] & 0x3F) << 6) |
                     ((unsigned char)s[2] & 0x3F);
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        *codepoint = ((c & 0x07) << 18) |
                     (((unsigned char)s[1] & 0x3F) << 12) |
                     (((unsigned char)s[2] & 0x3F) << 6) |
                     ((unsigned char)s[3] & 0x3F);
        return 4;
    }

    *codepoint = 0xFFFD;
    return 1;
}

int Utf8CharLen(unsigned char lead) {
    if ((lead & 0x80) == 0) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

bool IsEastAsianWide(uint32_t codepoint) {
    // CJK Unified Ideographs
    if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) ||
        (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
        (codepoint >= 0x20000 && codepoint <= 0x2A6DF) ||
        (codepoint >= 0x2A700 && codepoint <= 0x2B73F) ||
        (codepoint >= 0x2B740 && codepoint <= 0x2B81F) ||
        (codepoint >= 0x2B820 && codepoint <= 0x2CEAF) ||
        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
        (codepoint >= 0x2F800 && codepoint <= 0x2FA1F))
        return true;

    // Fullwidth Forms
    if ((codepoint >= 0xFF01 && codepoint <= 0xFF60) ||
        (codepoint >= 0xFFE0 && codepoint <= 0xFFE6))
        return true;

    // Hangul Syllables
    if (codepoint >= 0xAC00 && codepoint <= 0xD7A3)
        return true;

    // Hiragana & Katakana
    if ((codepoint >= 0x3040 && codepoint <= 0x309F) ||
        (codepoint >= 0x30A0 && codepoint <= 0x30FF))
        return true;

    // Other East Asian wide characters
    if ((codepoint >= 0x1100 && codepoint <= 0x115F) ||
        (codepoint >= 0x2E80 && codepoint <= 0x2EFF) ||
        (codepoint >= 0x2F00 && codepoint <= 0x2FDF) ||
        (codepoint >= 0x3000 && codepoint <= 0x303F) ||
        (codepoint >= 0x3200 && codepoint <= 0x32FF) ||
        (codepoint >= 0x3300 && codepoint <= 0x33FF))
        return true;

    return false;
}

bool IsWideEmoji(uint32_t codepoint) {
    if ((codepoint >= 0x1F300 && codepoint <= 0x1F9FF) ||
        (codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||
        (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||
        (codepoint >= 0x2600 && codepoint <= 0x26FF) ||
        (codepoint >= 0x2700 && codepoint <= 0x27BF) ||
        (codepoint >= 0x1FA00 && codepoint <= 0x1FA6F) ||
        (codepoint >= 0x1F900 && codepoint <= 0x1F9FF))
        return true;
    return false;
}

int CodepointDisplayWidth(uint32_t codepoint) {
    if (codepoint == 0x200D || codepoint == 0xFE0F ||
        (codepoint >= 0xFE00 && codepoint <= 0xFE0F))
        return 0;
    if (IsEastAsianWide(codepoint)) return 2;
    if (IsWideEmoji(codepoint)) return 2;
    if (codepoint >= 0x20 && codepoint < 0x7F) return 1;
    if (codepoint == 0x09) return 8;
    if (codepoint >= 0x80) return 1;
    return 0;
}

int CalculateDisplayWidth(const std::string& text) {
    if (text.empty()) return 0;
    int width = 0;
    const char* p = text.c_str();
    while (*p) {
        if (*p == '\033' && p[1] == '[') {
            p += 2;
            while (*p && *p != 'm') p++;
            if (*p == 'm') p++;
            continue;
        }
        uint32_t codepoint;
        int bytes = DecodeUtf8(p, &codepoint);
        if (bytes == 0 || bytes > 4) { p++; continue; }
        width += CodepointDisplayWidth(codepoint);
        p += bytes;
    }
    return width;
}

size_t ByteOffsetFromDisplayColumn(const std::string& str, int col) {
    if (col <= 0) return 0;
    int cur_col = 0;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char lead = static_cast<unsigned char>(str[i]);
        int clen = Utf8CharLen(lead);
        if (i + static_cast<size_t>(clen) > str.size()) clen = 1;

        uint32_t codepoint;
        DecodeUtf8(&str[i], &codepoint);
        int w = CodepointDisplayWidth(codepoint);

        if (cur_col + w > col) return i;
        cur_col += w;
        i += static_cast<size_t>(clen);
    }
    return str.size();
}

std::string Utf8Truncate(const std::string& str, int max_cols) {
    if (max_cols <= 0) return {};
    if (CalculateDisplayWidth(str) <= max_cols) return str;

    int cur_col = 0;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char lead = static_cast<unsigned char>(str[i]);
        int clen = Utf8CharLen(lead);
        if (i + static_cast<size_t>(clen) > str.size()) clen = 1;

        uint32_t codepoint;
        DecodeUtf8(&str[i], &codepoint);
        int w = CodepointDisplayWidth(codepoint);

        if (cur_col + w > max_cols) break;
        cur_col += w;
        i += static_cast<size_t>(clen);
    }
    return str.substr(0, i);
}

} // namespace UnicodeUtil
} // namespace FTB
