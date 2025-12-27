#ifndef FLOW_EMOJI_H
#define FLOW_EMOJI_H

#include <stdbool.h>

/**
 * 检测Unicode字符是否为宽字符emoji（在终端中占2个显示宽度）
 * 
 * 注意：不是所有Unicode符号都是2格，需要精确判断
 * - 真正的emoji（0x1F300-0x1F9FF等）通常是2格
 * - Miscellaneous Symbols (0x2600-0x26FF) 大多数是1格，但某些是2格
 * - Dingbats (0x2700-0x27BF) 大多数是2格
 * 
 * @param codepoint Unicode码点
 * @return true 如果是宽字符emoji（2格），false 否则（1格）
 */
bool is_wide_emoji(unsigned int codepoint);

#endif // FLOW_EMOJI_H

