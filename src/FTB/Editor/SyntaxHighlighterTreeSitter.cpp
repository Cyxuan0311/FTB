#include "FTB/Editor/SyntaxHighlighterTreeSitter.hpp"
#include "FTB/ThemeManager.hpp"
#include <algorithm>
#include <cstring>

#ifdef FTB_ENABLE_TREE_SITTER
#ifdef BUILD_TREE_SITTER_BASH
#include <tree_sitter/tree-sitter-bash.h>
#endif
#ifdef BUILD_TREE_SITTER_C
#include <tree_sitter/tree-sitter-c.h>
#endif
#ifdef BUILD_TREE_SITTER_CPP
#include <tree_sitter/tree-sitter-cpp.h>
#endif
#ifdef BUILD_TREE_SITTER_GO
#include <tree_sitter/tree-sitter-go.h>
#endif
#ifdef BUILD_TREE_SITTER_JAVA
#include <tree_sitter/tree-sitter-java.h>
#endif
#ifdef BUILD_TREE_SITTER_JAVASCRIPT
#include <tree_sitter/tree-sitter-javascript.h>
#endif
#ifdef BUILD_TREE_SITTER_JSON
#include <tree_sitter/tree-sitter-json.h>
#endif
#ifdef BUILD_TREE_SITTER_PYTHON
#include <tree_sitter/tree-sitter-python.h>
#endif
#ifdef BUILD_TREE_SITTER_RUST
#include <tree_sitter/tree-sitter-rust.h>
#endif
#ifdef BUILD_TREE_SITTER_TYPESCRIPT
#include <tree_sitter/tree-sitter-typescript.h>
#endif
#endif

using namespace ftxui;

namespace FTB {
namespace Editor {

// ---- 主题感知颜色辅助 ----
inline Color TC(const std::string& name) {
    return FTB::ThemeManager::GetInstance()->GetThemeColor(name);
}

// ---- 构造/析构 ----

SyntaxHighlighterTreeSitter::SyntaxHighlighterTreeSitter() {
#ifdef FTB_ENABLE_TREE_SITTER
    parser_ = ts_parser_new();
    if (parser_) {
        InitializeLanguages();
    }
#endif
}

SyntaxHighlighterTreeSitter::~SyntaxHighlighterTreeSitter() {
#ifdef FTB_ENABLE_TREE_SITTER
    if (parser_) {
        ts_parser_delete(parser_);
    }
#endif
}

// ---- 检查语言支持 ----

bool SyntaxHighlighterTreeSitter::HasLanguageSupport() const {
#ifdef FTB_ENABLE_TREE_SITTER
    return current_language_ != nullptr;
#else
    return false;
#endif
}

// ---- 静态方法 ----

bool SyntaxHighlighterTreeSitter::IsAvailable() {
#ifdef FTB_ENABLE_TREE_SITTER
    return true;
#else
    return false;
#endif
}

// ---- 语言映射初始化 ----

void SyntaxHighlighterTreeSitter::InitializeLanguages() {
#ifdef FTB_ENABLE_TREE_SITTER
    if (!parser_) return;

    // 各语言通过编译宏控制链接
    // 只有在 CMake 中启用了对应语言库时才会注册

#ifdef BUILD_TREE_SITTER_CPP
    if (auto* lang = tree_sitter_cpp()) {
        language_map_["cpp"] = lang;
        language_map_["cxx"] = lang;
        language_map_["cc"] = lang;
        language_map_["c++"] = lang;
        language_map_["hpp"] = lang;
        language_map_["hxx"] = lang;
        language_map_["hh"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_C
    if (auto* lang = tree_sitter_c()) {
        language_map_["c"] = lang;
        language_map_["h"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_PYTHON
    if (auto* lang = tree_sitter_python()) {
        language_map_["py"] = lang;
        language_map_["python"] = lang;
        language_map_["pyw"] = lang;
        language_map_["pyi"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JAVASCRIPT
    if (auto* lang = tree_sitter_javascript()) {
        language_map_["js"] = lang;
        language_map_["javascript"] = lang;
        language_map_["jsx"] = lang;
        language_map_["mjs"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_TYPESCRIPT
    if (auto* lang = tree_sitter_typescript()) {
        language_map_["ts"] = lang;
        language_map_["typescript"] = lang;
        language_map_["tsx"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_RUST
    if (auto* lang = tree_sitter_rust()) {
        language_map_["rs"] = lang;
        language_map_["rust"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_GO
    if (auto* lang = tree_sitter_go()) {
        language_map_["go"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JAVA
    if (auto* lang = tree_sitter_java()) {
        language_map_["java"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_CSHARP
    if (auto* lang = tree_sitter_c_sharp()) {
        language_map_["cs"] = lang;
        language_map_["csharp"] = lang;
        language_map_["csx"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_RUBY
    if (auto* lang = tree_sitter_ruby()) {
        language_map_["rb"] = lang;
        language_map_["ruby"] = lang;
        language_map_["rake"] = lang;
        language_map_["gemspec"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_PHP
    if (auto* lang = tree_sitter_php()) {
        language_map_["php"] = lang;
        language_map_["phtml"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_SWIFT
    if (auto* lang = tree_sitter_swift()) {
        language_map_["swift"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_KOTLIN
    if (auto* lang = tree_sitter_kotlin()) {
        language_map_["kt"] = lang;
        language_map_["kotlin"] = lang;
        language_map_["kts"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_SCALA
    if (auto* lang = tree_sitter_scala()) {
        language_map_["scala"] = lang;
        language_map_["sc"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_LUA
    if (auto* lang = tree_sitter_lua()) {
        language_map_["lua"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_BASH
    if (auto* lang = tree_sitter_bash()) {
        language_map_["sh"] = lang;
        language_map_["bash"] = lang;
        language_map_["shell"] = lang;
        language_map_["zsh"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_SQL
    if (auto* lang = tree_sitter_sql()) {
        language_map_["sql"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_CSS
    if (auto* lang = tree_sitter_css()) {
        language_map_["css"] = lang;
        language_map_["scss"] = lang;
        language_map_["sass"] = lang;
        language_map_["less"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JSON
    if (auto* lang = tree_sitter_json()) {
        language_map_["json"] = lang;
        language_map_["jsonc"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_YAML
    if (auto* lang = tree_sitter_yaml()) {
        language_map_["yaml"] = lang;
        language_map_["yml"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_TOML
    if (auto* lang = tree_sitter_toml()) {
        language_map_["toml"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_MARKDOWN
    if (auto* lang = tree_sitter_markdown()) {
        language_map_["md"] = lang;
        language_map_["markdown"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_CMAKE
    if (auto* lang = tree_sitter_cmake()) {
        language_map_["cmake"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_DOCKERFILE
    if (auto* lang = tree_sitter_dockerfile()) {
        language_map_["dockerfile"] = lang;
        language_map_["Dockerfile"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_VIM
    if (auto* lang = tree_sitter_vim()) {
        language_map_["vim"] = lang;
        language_map_["vimrc"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_ZIG
    if (auto* lang = tree_sitter_zig()) {
        language_map_["zig"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_HASKELL
    if (auto* lang = tree_sitter_haskell()) {
        language_map_["hs"] = lang;
        language_map_["haskell"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_PERL
    if (auto* lang = tree_sitter_perl()) {
        language_map_["pl"] = lang;
        language_map_["pm"] = lang;
        language_map_["perl"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_R
    if (auto* lang = tree_sitter_r()) {
        language_map_["r"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_FORTRAN
    if (auto* lang = tree_sitter_fortran()) {
        language_map_["f90"] = lang;
        language_map_["f95"] = lang;
        language_map_["f03"] = lang;
        language_map_["f"] = lang;
        language_map_["for"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_NIM
    if (auto* lang = tree_sitter_nim()) {
        language_map_["nim"] = lang;
        language_map_["nims"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_XML
    if (auto* lang = tree_sitter_xml()) {
        language_map_["xml"] = lang;
        language_map_["html"] = lang;
        language_map_["htm"] = lang;
        language_map_["svg"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_POWERSHELL
    if (auto* lang = tree_sitter_powershell()) {
        language_map_["ps1"] = lang;
        language_map_["powershell"] = lang;
        language_map_["psm1"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_ELIXIR
    if (auto* lang = tree_sitter_elixir()) {
        language_map_["ex"] = lang;
        language_map_["exs"] = lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_PROTO
    if (auto* lang = tree_sitter_proto()) {
        language_map_["proto"] = lang;
    }
#endif

#else
    // tree-sitter 未启用，无需初始化
#endif
}

// ---- 设置文件类型 ----

void SyntaxHighlighterTreeSitter::SetFileType(const std::string& file_type) {
    if (current_file_type_ == file_type) return;
    current_file_type_ = file_type;

#ifdef FTB_ENABLE_TREE_SITTER
    if (!parser_) return;

    auto* lang = GetLanguageForFileType(file_type);
    if (lang) {
        ts_parser_set_language(parser_, lang);
        current_language_ = lang;
    } else {
        current_language_ = nullptr;
    }
#endif
}

void SyntaxHighlighterTreeSitter::SetLanguage(Language lang) {
    current_lang_ = lang;
    // 将 Language 枚举映射为文件类型字符串
    std::string file_type;
    switch (lang) {
        case Language::C:           file_type = "c"; break;
        case Language::CPP:         file_type = "cpp"; break;
        case Language::GO:          file_type = "go"; break;
        case Language::PYTHON:      file_type = "py"; break;
        case Language::JAVASCRIPT:  file_type = "js"; break;
        case Language::TYPESCRIPT:  file_type = "ts"; break;
        case Language::RUST:        file_type = "rs"; break;
        case Language::JAVA:        file_type = "java"; break;
        case Language::CSHARP:      file_type = "cs"; break;
        case Language::RUBY:        file_type = "rb"; break;
        case Language::PHP:         file_type = "php"; break;
        case Language::SWIFT:       file_type = "swift"; break;
        case Language::KOTLIN:      file_type = "kt"; break;
        case Language::SCALA:       file_type = "scala"; break;
        case Language::LUA:         file_type = "lua"; break;
        case Language::SHELL: case Language::BASH: file_type = "sh"; break;
        case Language::SQL:         file_type = "sql"; break;
        case Language::HTML:        file_type = "html"; break;
        case Language::XML:         file_type = "xml"; break;
        case Language::CSS:         file_type = "css"; break;
        case Language::JSON:        file_type = "json"; break;
        case Language::YAML:        file_type = "yaml"; break;
        case Language::TOML:        file_type = "toml"; break;
        case Language::MARKDOWN:    file_type = "md"; break;
        case Language::CMAKE:       file_type = "cmake"; break;
        case Language::DOCKERFILE:  file_type = "dockerfile"; break;
        case Language::VIMSCRIPT:   file_type = "vim"; break;
        case Language::ZIG:         file_type = "zig"; break;
        case Language::ELIXIR:      file_type = "ex"; break;
        case Language::PERL:        file_type = "pl"; break;
        case Language::R:           file_type = "r"; break;
        case Language::HASKELL:     file_type = "hs"; break;
        case Language::FORTRAN:     file_type = "f90"; break;
        case Language::NIM:         file_type = "nim"; break;
        case Language::PROTO:       file_type = "proto"; break;
        case Language::POWERSHELL:  file_type = "ps1"; break;
        default: return;
    }
    SetFileType(file_type);
}

// ---- 检查支持 ----

bool SyntaxHighlighterTreeSitter::SupportsFileType(const std::string& file_type) const {
#ifdef FTB_ENABLE_TREE_SITTER
    return language_map_.find(file_type) != language_map_.end();
#else
    (void)file_type;
    return false;
#endif
}

// ---- 重置 ----

void SyntaxHighlighterTreeSitter::Reset() {
#ifdef FTB_ENABLE_TREE_SITTER
    if (parser_ && current_language_) {
        ts_parser_set_language(parser_, current_language_);
    }
#endif
}

// ---- 高亮单行 ----

ftxui::Element SyntaxHighlighterTreeSitter::HighlightLine(const std::string& line) {
#ifdef FTB_ENABLE_TREE_SITTER
    if (line.empty() || !parser_ || !current_language_) {
        return text(line) | color(TC("main_fg"));
    }
    return ParseAndHighlight(line);
#else
    return text(line) | color(TC("main_fg"));
#endif
}

// ---- 高亮多行 ----

ftxui::Element SyntaxHighlighterTreeSitter::HighlightLines(const std::vector<std::string>& lines) {
#ifdef FTB_ENABLE_TREE_SITTER
    if (lines.empty() || !parser_ || !current_language_) {
        Elements elements;
        for (const auto& line : lines) {
            elements.push_back(text(line) | color(TC("main_fg")));
        }
        return vbox(elements);
    }

    // 合并所有行为一个字符串进行解析 (更高效)
    std::string code;
    for (const auto& line : lines) {
        code += line + "\n";
    }
    return ParseAndHighlight(code);
#else
    Elements elements;
    for (const auto& line : lines) {
        elements.push_back(text(line) | color(TC("main_fg")));
    }
    return vbox(elements);
#endif
}

// ---- 解析为 token 列表 ----

void SyntaxHighlighterTreeSitter::ParseToTokens(const std::string& code, std::vector<SyntaxToken>& tokens) {
    tokens.clear();
#ifdef FTB_ENABLE_TREE_SITTER
    if (code.empty() || !parser_ || !current_language_) return;

    TSTree* tree = ts_parser_parse_string(parser_, nullptr, code.c_str(), code.length());
    if (!tree) return;

    TSNode root_node = ts_tree_root_node(tree);
    size_t current_pos = 0;
    TraverseTreeToTokens(root_node, code, tokens, current_pos, "");

    // 处理未覆盖的文本
    if (current_pos < code.length()) {
        tokens.emplace_back(code.substr(current_pos), TokenType::NORMAL,
            static_cast<int>(current_pos), static_cast<int>(code.length()));
    }

    ts_tree_delete(tree);
#else
    (void)code;
#endif
}

// ---- 解析并高亮 ----

ftxui::Element SyntaxHighlighterTreeSitter::ParseAndHighlight(const std::string& code) {
#ifdef FTB_ENABLE_TREE_SITTER
    if (code.empty() || !parser_ || !current_language_) {
        return text(code) | color(TC("main_fg"));
    }

    TSTree* tree = ts_parser_parse_string(parser_, nullptr, code.c_str(), code.length());
    if (!tree) {
        return text(code) | color(TC("main_fg"));
    }

    TSNode root_node = ts_tree_root_node(tree);

    Elements elements;
    size_t current_pos = 0;
    TraverseTree(root_node, code, elements, current_pos, "");

    // 处理未覆盖的文本
    if (current_pos < code.length()) {
        std::string remaining = code.substr(current_pos);
        elements.push_back(text(remaining) | color(TC("main_fg")));
    }

    ts_tree_delete(tree);
    return hbox(elements);
#else
    return text(code) | color(TC("main_fg"));
#endif
}

// ---- Tree-sitter 语言查找 ----

#ifdef FTB_ENABLE_TREE_SITTER
const TSLanguage* SyntaxHighlighterTreeSitter::GetLanguageForFileType(const std::string& file_type) {
    auto it = language_map_.find(file_type);
    if (it != language_map_.end()) {
        return it->second;
    }
    return nullptr;
}
#endif

// ---- 节点类型 -> TokenType 映射 ----

TokenType SyntaxHighlighterTreeSitter::GetTokenTypeForNode(
    const std::string& node_type, const std::string& parent_type) const {

    // 函数调用中的 identifier -> FUNCTION
    if (parent_type == "call_expression" || parent_type == "call" ||
        parent_type == "function_call" || parent_type == "method_invocation" ||
        parent_type == "template_function") {
        if (node_type == "identifier" || node_type == "qualified_identifier" ||
            node_type == "field_identifier" || node_type == "scoped_identifier") {
            return TokenType::FUNCTION;
        }
    }

    // 关键字
    if (node_type.find("keyword") != std::string::npos ||
        node_type == "if" || node_type == "else" || node_type == "for" ||
        node_type == "while" || node_type == "return" || node_type == "class" ||
        node_type == "function" || node_type == "const" || node_type == "let" ||
        node_type == "var" || node_type == "import" || node_type == "export" ||
        node_type == "try" || node_type == "catch" || node_type == "throw" ||
        node_type == "new" || node_type == "delete" || node_type == "def" ||
        node_type == "async" || node_type == "await" || node_type == "yield" ||
        node_type == "match" || node_type == "with" || node_type == "as" ||
        node_type == "storage_class_specifier" || node_type == "type_qualifier" ||
        node_type.find("_statement") != std::string::npos ||
        node_type.find("_specifier") != std::string::npos) {
        return TokenType::KEYWORD;
    }

    // 字符串
    if (node_type.find("string") != std::string::npos ||
        node_type == "string_content" || node_type == "string_literal" ||
        node_type == "char_literal" || node_type == "raw_string_literal") {
        return TokenType::STRING;
    }

    // 注释
    if (node_type.find("comment") != std::string::npos) {
        return TokenType::COMMENT;
    }

    // 数字
    if (node_type.find("number") != std::string::npos ||
        node_type == "integer" || node_type == "float" ||
        node_type == "float_literal" || node_type == "integer_literal") {
        return TokenType::NUMBER;
    }

    // 函数声明/定义
    if (node_type.find("function") != std::string::npos ||
        node_type == "call_expression" || node_type == "method_invocation" ||
        node_type == "call" || node_type == "function_declarator" ||
        node_type == "template_function" || node_type == "template_method") {
        return TokenType::FUNCTION;
    }

    // 类型
    if (node_type.find("type") != std::string::npos ||
        node_type == "class_declaration" || node_type == "struct_specifier" ||
        node_type == "enum_specifier" || node_type == "namespace_identifier" ||
        node_type == "primitive_type" || node_type == "sized_type_specifier") {
        return TokenType::TYPE;
    }

    // 操作符
    if (node_type.find("operator") != std::string::npos ||
        node_type == "+" || node_type == "-" || node_type == "*" ||
        node_type == "/" || node_type == "=" || node_type == "==" ||
        node_type == "binary_expression" || node_type == "unary_expression" ||
        node_type == "assignment_expression") {
        return TokenType::OPERATOR;
    }

    // 预处理器
    if (node_type.find("preproc") != std::string::npos ||
        node_type == "preprocessor_directive" ||
        node_type == "preproc_include" || node_type == "preproc_def") {
        return TokenType::PREPROCESSOR;
    }

    // 属性/字段
    if (node_type.find("property") != std::string::npos ||
        node_type.find("field") != std::string::npos) {
        return TokenType::PROPERTY;
    }

    // 标签 (HTML/XML)
    if (node_type.find("tag") != std::string::npos) {
        return TokenType::TAG;
    }

    // 属性名
    if (node_type.find("attribute") != std::string::npos) {
        return TokenType::ATTRIBUTE;
    }

    // 装饰器
    if (node_type == "decorator" || node_type == "annotation" ||
        node_type == "attribute_accessor") {
        return TokenType::DECORATOR;
    }

    return TokenType::IDENTIFIER;
}

// ---- Tree-sitter 树遍历 ----

#ifdef FTB_ENABLE_TREE_SITTER
void SyntaxHighlighterTreeSitter::TraverseTree(
    TSNode node, const std::string& source,
    std::vector<ftxui::Element>& elements,
    size_t& current_pos, const std::string& parent_type) const {

    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    // 处理节点前的文本
    if (current_pos < start_byte) {
        std::string before = source.substr(current_pos, start_byte - current_pos);
        elements.push_back(text(before) | color(TC("main_fg")));
        current_pos = start_byte;
    }

    const char* node_type_cstr = ts_node_type(node);
    std::string node_type = node_type_cstr ? node_type_cstr : "";
    std::string node_text = GetNodeText(node, source);

    TokenType token_type = GetTokenTypeForNode(node_type, parent_type);
    Color node_color = SyntaxHighlighter::GetTokenColor(token_type);

    if (ts_node_child_count(node) == 0) {
        elements.push_back(text(node_text) | color(node_color));
        current_pos = end_byte;
    } else {
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; ++i) {
            TSNode child = ts_node_child(node, i);
            TraverseTree(child, source, elements, current_pos, node_type);
        }
    }
}

void SyntaxHighlighterTreeSitter::TraverseTreeToTokens(
    TSNode node, const std::string& source,
    std::vector<SyntaxToken>& tokens,
    size_t& current_pos, const std::string& parent_type) const {

    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    // 处理节点前的文本
    if (current_pos < start_byte) {
        std::string before = source.substr(current_pos, start_byte - current_pos);
        tokens.emplace_back(before, TokenType::NORMAL,
            static_cast<int>(current_pos), static_cast<int>(start_byte));
        current_pos = start_byte;
    }

    const char* node_type_cstr = ts_node_type(node);
    std::string node_type = node_type_cstr ? node_type_cstr : "";

    TokenType token_type = GetTokenTypeForNode(node_type, parent_type);

    if (ts_node_child_count(node) == 0) {
        std::string node_text = GetNodeText(node, source);
        tokens.emplace_back(node_text, token_type,
            static_cast<int>(start_byte), static_cast<int>(end_byte));
        current_pos = end_byte;
    } else {
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; ++i) {
            TSNode child = ts_node_child(node, i);
            TraverseTreeToTokens(child, source, tokens, current_pos, node_type);
        }
    }
}

std::string SyntaxHighlighterTreeSitter::GetNodeText(TSNode node, const std::string& source) const {
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    if (start_byte >= source.length()) return "";

    size_t start = static_cast<size_t>(start_byte);
    size_t end = std::min(static_cast<size_t>(end_byte), source.length());

    return source.substr(start, end - start);
}
#endif // FTB_ENABLE_TREE_SITTER

} // namespace Editor
} // namespace FTB
