#ifndef SYNTAX_HIGHLIGHTER_HPP
#define SYNTAX_HIGHLIGHTER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <ftxui/dom/elements.hpp>

namespace FTB { namespace Editor {
class SyntaxHighlighterTreeSitter;
}}

namespace FTB {
namespace Editor {

// 支持的语言类型
enum class Language {
    NONE,
    C, CPP,
    GO,
    PYTHON,
    JAVASCRIPT, TYPESCRIPT,
    RUST,
    JAVA,
    CSHARP,
    RUBY,
    PHP,
    SWIFT,
    KOTLIN,
    SCALA,
    LUA,
    SHELL, BASH,
    SQL,
    HTML, XML, CSS,
    JSON, YAML, TOML,
    MARKDOWN,
    CMAKE, MAKEFILE,
    DOCKERFILE,
    VIMSCRIPT,
    ZIG,
    ELIXIR,
    PERL,
    R,
    HASKELL,
    FORTRAN,
    NIM,
    PROTO,
    MESON,
    POWERSHELL,
    DART,
    JULIA,
    ERLANG,
    CLOJURE,
    FSHARP,
};

// 语法高亮的 token 类型
enum class TokenType {
    KEYWORD,       // 关键字
    STRING,        // 字符串
    COMMENT,       // 注释
    NUMBER,        // 数字
    FUNCTION,      // 函数名
    TYPE,          // 类型
    OPERATOR,      // 操作符
    PREPROCESSOR,  // 预处理指令 (#include, #define 等)
    IDENTIFIER,    // 标识符
    PUNCTUATION,   // 标点符号
    PROPERTY,      // 属性/字段
    TAG,           // HTML/XML 标签
    ATTRIBUTE,     // 属性名
    REGEX,         // 正则表达式
    DECORATOR,     // 装饰器 (@decorator)
    NORMAL         // 普通文本
};

// 语法高亮的 token 结构
struct SyntaxToken {
    std::string text;
    TokenType type;
    int start_pos;
    int end_pos;

    SyntaxToken(const std::string& t, TokenType ty, int start, int end)
        : text(t), type(ty), start_pos(start), end_pos(end) {}
};

// 语法高亮后端
enum class HighlightBackend {
    NATIVE,        // 内置关键词匹配
    TREE_SITTER    // tree-sitter (可选)
};

// 语法高亮器类
class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    // 设置当前语言
    void SetLanguage(Language lang);
    Language GetLanguage() const { return current_language_; }

    // 设置后端
    void SetBackend(HighlightBackend backend);
    HighlightBackend GetBackend() const { return backend_; }

    // 解析单行语法高亮
    std::vector<SyntaxToken> ParseLine(const std::string& line);

    // 获取 token 对应的颜色 (主题感知, 静态方法供外部调用)
    static ftxui::Color GetTokenColor(TokenType type);

    // 检测文件语言类型
    static Language DetectLanguage(const std::string& filename);

    // 获取语言显示名称
    static std::string GetLanguageName(Language lang);

    // 渲染带语法高亮的行
    ftxui::Element RenderLine(const std::string& line, bool is_cursor_line = false, int cursor_pos = 0);

    // 重置多行状态 (如多行注释)
    void ResetMultiLineState();

    // 是否在多行注释中
    bool IsInMultiLineComment() const { return in_multiline_comment_; }

private:
    Language current_language_ = Language::NONE;
    HighlightBackend backend_ = HighlightBackend::NATIVE;
    std::unique_ptr<SyntaxHighlighterTreeSitter> ts_highlighter_;

    // 多行状态
    bool in_multiline_comment_ = false;

    // 各种语言的模式 (使用 unordered_set 加速查找)
    std::unordered_map<Language, std::unordered_set<std::string>> keyword_patterns_;
    std::unordered_map<Language, std::unordered_set<std::string>> type_patterns_;

    // 语法高亮缓存
    std::unordered_map<size_t, std::vector<SyntaxToken>> token_cache_;

    // 初始化各种语言的模式
    void InitializePatterns();

    // 缓存 key 计算
    size_t ComputeCacheKey(Language lang, const std::string& line) const;

    // 检查是否为关键字
    bool IsKeyword(const std::string& word, Language lang);

    // 检查是否为类型
    bool IsType(const std::string& word, Language lang);

    // 检查是否为数字
    bool IsNumber(const std::string& word);

    // 检查是否为字符串起始
    bool IsString(const std::string& line, size_t pos);

    // 检查是否为注释起始
    bool IsComment(const std::string& line, size_t pos, Language lang);

    // 获取字符串结束位置
    size_t FindStringEnd(const std::string& line, size_t start);

    // 获取注释结束位置
    size_t FindCommentEnd(const std::string& line, size_t start, Language lang);

    // 检查是否为预处理指令
    bool IsPreprocessor(const std::string& line, size_t pos, Language lang);

    // 检查是否为运算符
    bool IsOperator(char c);

    // 检查是否为装饰器
    bool IsDecorator(const std::string& line, size_t pos, Language lang);
};

} // namespace Editor
} // namespace FTB

#endif // SYNTAX_HIGHLIGHTER_HPP
