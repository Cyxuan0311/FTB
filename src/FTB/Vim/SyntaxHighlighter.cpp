#include "FTB/Vim/SyntaxHighlighter.hpp"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace FTB {
namespace Vim {

SyntaxHighlighter::SyntaxHighlighter() : current_language_(Language::NONE) {
    InitializePatterns();
}

SyntaxHighlighter::~SyntaxHighlighter() {}

void SyntaxHighlighter::SetLanguage(Language lang) {
    current_language_ = lang;
}

void SyntaxHighlighter::InitializePatterns() {
    // C/C++ 关键字
    std::vector<std::string> cpp_keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
        "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return",
        "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile", "while", "asm", "bool", "catch", "class", "const_cast", "delete",
        "dynamic_cast", "explicit", "export", "false", "friend", "inline", "mutable", "namespace",
        "new", "operator", "private", "protected", "public", "reinterpret_cast", "static_cast",
        "template", "this", "throw", "true", "try", "typeid", "typename", "using", "virtual"
    };
    keyword_patterns_[Language::C] = cpp_keywords;
    keyword_patterns_[Language::CPP] = cpp_keywords;
    
    // Go 关键字
    std::vector<std::string> go_keywords = {
        "break", "case", "chan", "const", "continue", "default", "defer", "else", "fallthrough",
        "for", "func", "go", "goto", "if", "import", "interface", "map", "package", "range",
        "return", "select", "struct", "switch", "type", "var"
    };
    keyword_patterns_[Language::GO] = go_keywords;
    
    // Python 关键字
    std::vector<std::string> python_keywords = {
        "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else",
        "except", "exec", "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "with",
        "yield", "True", "False", "None"
    };
    keyword_patterns_[Language::PYTHON] = python_keywords;
    
    // C/C++ 类型
    std::vector<std::string> cpp_types = {
        "int", "char", "float", "double", "void", "bool", "string", "vector", "map", "set", "list"
    };
    type_patterns_[Language::C] = cpp_types;
    type_patterns_[Language::CPP] = cpp_types;
    
    // Go 类型
    std::vector<std::string> go_types = {
        "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
        "float32", "float64", "bool", "string", "byte", "rune"
    };
    type_patterns_[Language::GO] = go_types;
    
    // Python 类型
    std::vector<std::string> python_types = {
        "int", "float", "str", "bool", "list", "dict", "tuple", "set"
    };
    type_patterns_[Language::PYTHON] = python_types;
}

std::vector<SyntaxToken> SyntaxHighlighter::ParseLine(const std::string& line) {
    // 检查缓存
    auto cache_key = std::make_pair(current_language_, line);
    auto cache_it = token_cache_.find(cache_key);
    if (cache_it != token_cache_.end()) {
        return cache_it->second;
    }
    
    std::vector<SyntaxToken> tokens;
    
    if (current_language_ == Language::NONE || line.empty()) {
        tokens.push_back(SyntaxToken(line, TokenType::NORMAL, 0, static_cast<int>(line.length())));
        // 缓存结果
        token_cache_[cache_key] = tokens;
        return tokens;
    }
    
    // 预分配空间，避免动态扩容
    tokens.reserve(line.length() / 4); // 估算token数量
    
    size_t i = 0;
    std::string current_word;
    current_word.reserve(32); // 预分配单词空间
    
    while (i < line.length()) {
        char c = line[i];
        
        // 检查注释（最高优先级）
        if (IsComment(line, i, current_language_)) {
            if (!current_word.empty()) {
                tokens.emplace_back(current_word, TokenType::NORMAL, 
                    static_cast<int>(i - current_word.length()), static_cast<int>(i));
                current_word.clear();
            }
            size_t comment_end = FindCommentEnd(line, i, current_language_);
            tokens.emplace_back(line.substr(i, comment_end - i), TokenType::COMMENT, 
                static_cast<int>(i), static_cast<int>(comment_end));
            i = comment_end;
            continue;
        }
        
        // 检查字符串
        if (IsString(line, i)) {
            if (!current_word.empty()) {
                tokens.emplace_back(current_word, TokenType::NORMAL, 
                    static_cast<int>(i - current_word.length()), static_cast<int>(i));
                current_word.clear();
            }
            size_t string_end = FindStringEnd(line, i);
            tokens.emplace_back(line.substr(i, string_end - i), TokenType::STRING, 
                static_cast<int>(i), static_cast<int>(string_end));
            i = string_end;
            continue;
        }
        
        // 检查单词字符
        if (std::isalnum(c) || c == '_') {
            current_word += c;
        } else {
            if (!current_word.empty()) {
                TokenType type = TokenType::NORMAL;
                if (IsKeyword(current_word, current_language_)) {
                    type = TokenType::KEYWORD;
                } else if (IsType(current_word, current_language_)) {
                    type = TokenType::TYPE;
                } else if (IsNumber(current_word)) {
                    type = TokenType::NUMBER;
                }
                
                tokens.emplace_back(current_word, type, 
                    static_cast<int>(i - current_word.length()), static_cast<int>(i));
                current_word.clear();
            }
            tokens.emplace_back(std::string(1, c), TokenType::NORMAL, 
                static_cast<int>(i), static_cast<int>(i + 1));
        }
        i++;
    }
    
    // 处理最后一个单词
    if (!current_word.empty()) {
        TokenType type = TokenType::NORMAL;
        if (IsKeyword(current_word, current_language_)) {
            type = TokenType::KEYWORD;
        } else if (IsType(current_word, current_language_)) {
            type = TokenType::TYPE;
        } else if (IsNumber(current_word)) {
            type = TokenType::NUMBER;
        }
        
        tokens.emplace_back(current_word, type, 
            static_cast<int>(line.length() - current_word.length()), static_cast<int>(line.length()));
    }
    
    // 缓存结果
    token_cache_[cache_key] = tokens;
    return tokens;
}

ftxui::Color SyntaxHighlighter::GetTokenColor(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:
            return Color::Blue;
        case TokenType::STRING:
            return Color::Green;
        case TokenType::COMMENT:
            return Color::GrayLight;
        case TokenType::NUMBER:
            return Color::Yellow;
        case TokenType::FUNCTION:
            return Color::Cyan;
        case TokenType::TYPE:
            return Color::Magenta;
        case TokenType::OPERATOR:
            return Color::Red;
        case TokenType::NORMAL:
        default:
            return Color::White;
    }
}

Language SyntaxHighlighter::DetectLanguage(const std::string& filename) {
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    if (lower_filename.length() >= 2 && lower_filename.substr(lower_filename.length() - 2) == ".c") {
        return Language::C;
    } else if ((lower_filename.length() >= 4 && lower_filename.substr(lower_filename.length() - 4) == ".cpp") ||
               (lower_filename.length() >= 3 && lower_filename.substr(lower_filename.length() - 3) == ".cc") ||
               (lower_filename.length() >= 4 && lower_filename.substr(lower_filename.length() - 4) == ".cxx") ||
               (lower_filename.length() >= 4 && lower_filename.substr(lower_filename.length() - 4) == ".hpp") ||
               (lower_filename.length() >= 2 && lower_filename.substr(lower_filename.length() - 2) == ".h")) {
        return Language::CPP;
    } else if (lower_filename.length() >= 3 && lower_filename.substr(lower_filename.length() - 3) == ".go") {
        return Language::GO;
    } else if (lower_filename.length() >= 3 && lower_filename.substr(lower_filename.length() - 3) == ".py") {
        return Language::PYTHON;
    }
    
    return Language::NONE;
}

ftxui::Element SyntaxHighlighter::RenderLine(const std::string& line, bool is_cursor_line, int cursor_pos) {
    // 如果没有设置语言，直接返回普通文本
    if (current_language_ == Language::NONE) {
        if (is_cursor_line) {
            std::string left = line.substr(0, cursor_pos);
            std::string right = line.substr(cursor_pos);
            return hbox({
                text(left),
                text("|") | color(Color::BlueLight),
                text(right)
            });
        }
        return text(line);
    }
    
    std::vector<SyntaxToken> tokens = ParseLine(line);
    Elements elements;
    elements.reserve(tokens.size() + 1); // 预分配空间
    
    for (const auto& token : tokens) {
        if (is_cursor_line && token.start_pos <= cursor_pos && token.end_pos > cursor_pos) {
            // 光标在token中间，需要分割
            int offset = cursor_pos - token.start_pos;
            std::string before_cursor = token.text.substr(0, offset);
            std::string after_cursor = token.text.substr(offset);
            
            if (!before_cursor.empty()) {
                elements.emplace_back(text(before_cursor) | color(GetTokenColor(token.type)));
            }
            elements.emplace_back(text("|") | color(Color::BlueLight));
            if (!after_cursor.empty()) {
                elements.emplace_back(text(after_cursor) | color(GetTokenColor(token.type)));
            }
        } else {
            elements.emplace_back(text(token.text) | color(GetTokenColor(token.type)));
        }
    }
    
    // 如果光标在行末
    if (is_cursor_line && cursor_pos >= static_cast<int>(line.length())) {
        elements.emplace_back(text("|") | color(Color::BlueLight));
    }
    
    return hbox(elements);
}

bool SyntaxHighlighter::IsKeyword(const std::string& word, Language lang) {
    auto it = keyword_patterns_.find(lang);
    if (it != keyword_patterns_.end()) {
        return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
    }
    return false;
}

bool SyntaxHighlighter::IsType(const std::string& word, Language lang) {
    auto it = type_patterns_.find(lang);
    if (it != type_patterns_.end()) {
        return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
    }
    return false;
}

bool SyntaxHighlighter::IsFunction(const std::string& word, Language lang) {
    // 简单的函数检测：以字母开头，包含字母数字下划线
    if (word.empty() || !std::isalpha(word[0])) return false;
    
    for (char c : word) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

bool SyntaxHighlighter::IsNumber(const std::string& word) {
    if (word.empty()) return false;
    
    for (char c : word) {
        if (!std::isdigit(c) && c != '.') {
            return false;
        }
    }
    return true;
}

bool SyntaxHighlighter::IsString(const std::string& line, size_t pos) {
    return pos < line.length() && (line[pos] == '"' || line[pos] == '\'' || line[pos] == '`');
}

bool SyntaxHighlighter::IsComment(const std::string& line, size_t pos, Language lang) {
    if (lang == Language::PYTHON) {
        return pos < line.length() && line[pos] == '#';
    } else {
        return pos < line.length() - 1 && line.substr(pos, 2) == "//";
    }
}

size_t SyntaxHighlighter::FindStringEnd(const std::string& line, size_t start) {
    if (start >= line.length()) return start;
    
    char quote = line[start];
    size_t end = start + 1;
    
    while (end < line.length() && line[end] != quote) {
        if (line[end] == '\\' && end + 1 < line.length()) {
            end += 2; // 跳过转义字符
        } else {
            end++;
        }
    }
    
    return (end < line.length()) ? end + 1 : line.length();
}

size_t SyntaxHighlighter::FindCommentEnd(const std::string& line, size_t start, Language lang) {
    if (lang == Language::PYTHON) {
        return line.length(); // Python注释到行末
    } else {
        return line.length(); // C/C++/Go单行注释到行末
    }
}

} // namespace Vim
} // namespace FTB
