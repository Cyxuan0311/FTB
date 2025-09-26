#ifndef SYNTAX_HIGHLIGHTER_HPP
#define SYNTAX_HIGHLIGHTER_HPP

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <ftxui/dom/elements.hpp>

namespace FTB {
namespace Vim {

// 支持的语言类型
enum class Language {
    NONE,
    C,
    CPP,
    GO,
    PYTHON
};

// 语法高亮的token类型
enum class TokenType {
    KEYWORD,      // 关键字
    STRING,       // 字符串
    COMMENT,      // 注释
    NUMBER,       // 数字
    FUNCTION,     // 函数名
    TYPE,         // 类型
    OPERATOR,     // 操作符
    NORMAL        // 普通文本
};

// 语法高亮的token结构
struct SyntaxToken {
    std::string text;
    TokenType type;
    int start_pos;
    int end_pos;
    
    SyntaxToken(const std::string& t, TokenType ty, int start, int end)
        : text(t), type(ty), start_pos(start), end_pos(end) {}
};

// 语法高亮器类
class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();
    
    // 设置当前语言
    void SetLanguage(Language lang);
    Language GetLanguage() const { return current_language_; }
    
    // 解析单行语法高亮
    std::vector<SyntaxToken> ParseLine(const std::string& line);
    
    // 获取token对应的颜色
    ftxui::Color GetTokenColor(TokenType type);
    
    // 检测文件语言类型
    static Language DetectLanguage(const std::string& filename);
    
    // 渲染带语法高亮的行
    ftxui::Element RenderLine(const std::string& line, bool is_cursor_line = false, int cursor_pos = 0);

private:
    Language current_language_;
    
    // 各种语言的模式
    std::map<Language, std::vector<std::string>> keyword_patterns_;
    std::map<Language, std::vector<std::string>> type_patterns_;
    std::map<Language, std::vector<std::string>> function_patterns_;
    
    // 语法高亮缓存
    std::map<std::pair<Language, std::string>, std::vector<SyntaxToken>> token_cache_;
    
    // 初始化各种语言的模式
    void InitializePatterns();
    
    // 检查是否为关键字
    bool IsKeyword(const std::string& word, Language lang);
    
    // 检查是否为类型
    bool IsType(const std::string& word, Language lang);
    
    // 检查是否为函数
    bool IsFunction(const std::string& word, Language lang);
    
    // 检查是否为数字
    bool IsNumber(const std::string& word);
    
    // 检查是否为字符串
    bool IsString(const std::string& line, size_t pos);
    
    // 检查是否为注释
    bool IsComment(const std::string& line, size_t pos, Language lang);
    
    // 获取字符串结束位置
    size_t FindStringEnd(const std::string& line, size_t start);
    
    // 获取注释结束位置
    size_t FindCommentEnd(const std::string& line, size_t start, Language lang);
};

} // namespace Vim
} // namespace FTB

#endif // SYNTAX_HIGHLIGHTER_HPP
