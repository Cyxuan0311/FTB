#pragma once

#include <string>
#include <cstdint>

namespace FTB {
namespace UnicodeUtil {

// Decode a single UTF-8 character. Returns byte length (1-4), sets *codepoint.
int DecodeUtf8(const char* s, uint32_t* codepoint);

// Length of a UTF-8 sequence from its leading byte.
int Utf8CharLen(unsigned char lead);

// Whether the codepoint is an East Asian wide/fullwidth character (≥2 columns).
bool IsEastAsianWide(uint32_t codepoint);

// Whether the codepoint is a wide emoji.
bool IsWideEmoji(uint32_t codepoint);

// Display width of a single codepoint (1 for ASCII, 2 for CJK/wide emoji, etc.)
int CodepointDisplayWidth(uint32_t codepoint);

// Display width of a UTF-8 string, counting CJK as 2, emoji as 2, ASCII as 1.
int CalculateDisplayWidth(const std::string& text);

// Given a display column position, return the byte offset into str.
// If col exceeds the string's display width, returns str.size().
size_t ByteOffsetFromDisplayColumn(const std::string& str, int col);

// Truncate a UTF-8 string so its display width ≤ max_cols.
// Never splits a multi-byte character or a wide character in half.
std::string Utf8Truncate(const std::string& str, int max_cols);

} // namespace UnicodeUtil
} // namespace FTB
