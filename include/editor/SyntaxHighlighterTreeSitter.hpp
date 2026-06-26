#ifndef SYNTAX_HIGHLIGHTER_TREE_SITTER_HPP
#define SYNTAX_HIGHLIGHTER_TREE_SITTER_HPP

#include "editor/SyntaxHighlighter.hpp"
#include <map>
#include <string>
#include <vector>

// Tree-sitter 头文件 (仅在 FTB_ENABLE_TREE_SITTER 时可用)
#ifdef FTB_ENABLE_TREE_SITTER
#include <tree_sitter/api.h>
#endif

namespace FTB {
namespace Editor {

// 高亮片段：字节范围 [start, end) 及对应 token 类型
struct HighlightSegment {
    size_t start;
    size_t end;
    TokenType token_type;
};

// Tree-sitter 语法高亮器 (可选后端)
// 当 FTB_ENABLE_TREE_SITTER 宏定义时启用，否则自动回退到原生高亮
class SyntaxHighlighterTreeSitter {
public:
    SyntaxHighlighterTreeSitter();
    ~SyntaxHighlighterTreeSitter();

    // 禁用拷贝
    SyntaxHighlighterTreeSitter(const SyntaxHighlighterTreeSitter&) = delete;
    SyntaxHighlighterTreeSitter& operator=(const SyntaxHighlighterTreeSitter&) = delete;

    // 设置文件类型 (通过扩展名/文件名)
    void SetFileType(const std::string& file_type);

    // 设置语言 (通过 Language 枚举)
    void SetLanguage(Language lang);

    // 高亮一行代码
    ftxui::Element HighlightLine(const std::string& line);

    // 高亮多行代码 (更高效)
    ftxui::Element HighlightLines(const std::vector<std::string>& lines);

    // 解析代码并生成 token 列表 (用于与原生高亮合并)
    void ParseToTokens(const std::string& code, std::vector<SyntaxToken>& tokens);

    // 重置解析器状态
    void Reset();

    // 检查是否支持指定文件类型
    bool SupportsFileType(const std::string& file_type) const;

    // 检查 tree-sitter 是否支持当前语言
    bool HasLanguageSupport() const;

    // 检查 tree-sitter 是否可用
    static bool IsAvailable();

    // 获取当前语言
    Language GetLanguage() const { return current_lang_; }

private:
#ifdef FTB_ENABLE_TREE_SITTER
    TSParser* parser_ = nullptr;
    const TSLanguage* current_language_ = nullptr;
    std::map<std::string, const TSLanguage*> language_map_;
#endif

    std::string current_file_type_;
    Language current_lang_ = Language::NONE;

    // 初始化语言映射
    void InitializeLanguages();

    // 获取 Tree-sitter 语言
#ifdef FTB_ENABLE_TREE_SITTER
    const TSLanguage* GetLanguageForFileType(const std::string& file_type);
#endif

    // 将 Tree-sitter 节点类型映射到 TokenType
    TokenType GetTokenTypeForNode(const std::string& node_type,
                                  const std::string& parent_type) const;

    // 解析代码并生成高亮元素
    ftxui::Element ParseAndHighlight(const std::string& code);

#ifdef FTB_ENABLE_TREE_SITTER
    // 遍历语法树并生成高亮元素
    void TraverseTree(TSNode node, const std::string& source,
                      std::vector<ftxui::Element>& elements,
                      size_t& current_pos, const std::string& parent_type) const;

    // 遍历语法树并生成 token 列表
    void TraverseTreeToTokens(TSNode node, const std::string& source,
                              std::vector<SyntaxToken>& tokens,
                              size_t& current_pos, const std::string& parent_type) const;

    // 获取节点文本
    std::string GetNodeText(TSNode node, const std::string& source) const;
#endif
};

} // namespace Editor
} // namespace FTB

#endif // SYNTAX_HIGHLIGHTER_TREE_SITTER_HPP
