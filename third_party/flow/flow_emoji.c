#include "flow_emoji.h"

/**
 * 检测是否为宽字符emoji（在终端中占2个显示宽度）
 * 
 * 判断规则（基于Unicode标准和终端实际显示）：
 * 1. 真正的emoji范围（0x1F300-0x1F9FF等）通常是2格
 * 2. Miscellaneous Symbols (0x2600-0x26FF) 需要精确判断，大多数是1格
 * 3. Dingbats (0x2700-0x27BF) 需要精确判断，部分是2格，部分是1格
 * 4. 其他特殊符号需要单独判断
 * 
 * 注意：此函数只返回确定是2格的字符，其他字符返回false（按1格处理）
 */
bool is_wide_emoji(unsigned int codepoint) {
    // ============================================
    // 1. 真正的emoji范围（确定是2格）
    // ============================================
    if ((codepoint >= 0x1F300 && codepoint <= 0x1F9FF) ||   // Miscellaneous Symbols and Pictographs, Emoticons
        (codepoint >= 0x1F600 && codepoint <= 0x1F64F) ||   // Emoticons
        (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) ||   // Transport and Map Symbols
        (codepoint >= 0x1F700 && codepoint <= 0x1F77F) ||   // Alchemical Symbols
        (codepoint >= 0x1F780 && codepoint <= 0x1F7FF) ||   // Geometric Shapes Extended
        (codepoint >= 0x1F800 && codepoint <= 0x1F8FF) ||   // Supplemental Arrows-C
        (codepoint >= 0x1F900 && codepoint <= 0x1F9FF) ||   // Supplemental Symbols and Pictographs
        (codepoint >= 0x1FA00 && codepoint <= 0x1FA6F) ||   // Chess Symbols
        (codepoint >= 0x1FA70 && codepoint <= 0x1FAFF) ||   // Symbols and Pictographs Extended-A
        (codepoint >= 0x1F1E6 && codepoint <= 0x1F1FF)) {  // Regional Indicator Symbols (国旗)
        return true;
    }
    
    // ============================================
    // 2. Miscellaneous Symbols (0x2600-0x26FF)
    // ============================================
    // 大多数是1格，只处理确定是2格的符号
    // 注意：这个范围的符号大多数是1格，只有少数是2格
    // 为了安全起见，这里只处理确定是2格的符号
    // 如果发现更多2格的符号，可以在这里添加
    if (codepoint >= 0x2600 && codepoint <= 0x26FF) {
        // 排除确定是1格的符号
        if (codepoint == 0x26A1) {  // ⚡ 闪电符号，通常是1格
            return true;
        }
        // 注意：Miscellaneous Symbols范围很广，大多数是1格
        // 目前没有发现其他确定是2格的符号，所以这里返回false
        // 如果将来发现更多2格的符号，可以在这里添加
        return false;  // 保守处理：默认按1格处理
    }
    
    // ============================================
    // 3. Dingbats (0x2700-0x27BF)
    // ============================================
    // 部分字符是2格（如 ✅ ✂️ ✏️ ✨），部分可能是1格
    // 只处理确定是2格的字符
    if (codepoint >= 0x2700 && codepoint <= 0x27BF) {
        // 常见的宽字符Dingbats（确定是2格）
        if (codepoint == 0x2702 ||  // ✂ 剪刀
            codepoint == 0x2705 ||  // ✅ check mark
            codepoint == 0x2708 ||  // ✈ 飞机
            codepoint == 0x2709 ||  // ✉ 信封
            codepoint == 0x270A ||  // ✊ 拳头
            codepoint == 0x270B ||  // ✋ 手
            codepoint == 0x270C ||  // ✌ 胜利手势
            codepoint == 0x270D ||  // ✍ 写字
            codepoint == 0x270F ||  // ✏ 铅笔
            codepoint == 0x2728 ||  // ✨ 星星
            codepoint == 0x2744 ||  // ❄ 雪花
            codepoint == 0x274C ||  // ❌ 叉号
            codepoint == 0x274E ||  // ❎ 带框的叉号
            codepoint == 0x2753 ||  // ❓ 问号
            codepoint == 0x2754 ||  // ❔ 问号
            codepoint == 0x2755 ||  // ❕ 感叹号
            codepoint == 0x2757 ||  // ❗ 感叹号
            codepoint == 0x2763 ||  // ❣ 心形
            codepoint == 0x2764 ||  // ❤ 心形
            codepoint == 0x2795 ||  // ➕ 加号
            codepoint == 0x2796 ||  // ➖ 减号
            codepoint == 0x2797 ||  // ➗ 除号
            codepoint == 0x27A1 ||  // ➡ 箭头
            codepoint == 0x27B0 ||  // ➰ 循环
            codepoint == 0x27BF) {  // ➿ 双循环
            return true;
        }
        // 其他Dingbats按1格处理
        return false;
    }
    
    // ============================================
    // 4. 其他特殊符号
    // ============================================
    // 某些特殊符号也是2格
    if (codepoint == 0x23F0) {  // ⏰ 闹钟符号，通常是2格
        return true;
    }
    
    return false;
}

