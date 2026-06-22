#ifndef DETAIL_ELEMENT_HPP
#define DETAIL_ELEMENT_HPP

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "FTB/ArchivePreview.hpp"
#include "FTB/ImagePreview.hpp"
#include <cstdio>
#include <mutex>
#include <ftxui/dom/elements.hpp>
#include "ClipboardManager.hpp"
#include "FileManager.hpp"
#include "IconMapper.hpp"
#include "FTB/ThemeManager.hpp"
#include "FTB/Editor/SyntaxHighlighter.hpp"
#include "FTB/SortMode.hpp"
#include "FTB/ConfigManager.hpp"

// ---- 主题感知颜色辅助 ----
// 使用 ThemeManager 获取当前主题颜色，确保预览区域跟随主题变化
inline ftxui::Color TC(const std::string& name) {
    return FTB::ThemeManager::GetInstance()->GetThemeColor(name);
}

// ---- 带主题颜色的面板边框装饰器 ----
inline ftxui::Decorator GetPanelBorder() {
    const auto& style = FTB::ConfigManager::GetInstance()->GetConfig().ui.panel_border;
    ftxui::Color border_color = TC("main_border");
    if (style == "rounded")  return ftxui::borderStyled(ftxui::ROUNDED, border_color);
    if (style == "sharp")    return ftxui::borderStyled(ftxui::LIGHT, border_color);
    if (style == "double")   return ftxui::borderStyled(ftxui::DOUBLE, border_color);
    if (style == "heavy")    return ftxui::borderStyled(ftxui::HEAVY, border_color);
    if (style == "none")     return ftxui::nothing;
    return ftxui::borderStyled(ftxui::ROUNDED, border_color);
}

// 保留 DetailColor 作为默认回退 (仅用于 ThemeManager 未初始化时)
namespace DetailColor {
    const ftxui::Color Base     = ftxui::Color::RGB(0x1e, 0x1e, 0x2e);
    const ftxui::Color Mantle   = ftxui::Color::RGB(0x18, 0x18, 0x25);
    const ftxui::Color Surface0 = ftxui::Color::RGB(0x31, 0x32, 0x44);
    const ftxui::Color Surface1 = ftxui::Color::RGB(0x45, 0x47, 0x5a);
    const ftxui::Color Surface2 = ftxui::Color::RGB(0x58, 0x5b, 0x70);
    const ftxui::Color Overlay0 = ftxui::Color::RGB(0x6c, 0x70, 0x86);
    const ftxui::Color Text     = ftxui::Color::RGB(0xcd, 0xd6, 0xf4);
    const ftxui::Color Subtext  = ftxui::Color::RGB(0xa6, 0xad, 0xbc);
    const ftxui::Color Blue     = ftxui::Color::RGB(0x89, 0xb4, 0xfa);
    const ftxui::Color Green    = ftxui::Color::RGB(0xa6, 0xe3, 0xa1);
    const ftxui::Color Red      = ftxui::Color::RGB(0xf3, 0x8b, 0xa8);
    const ftxui::Color Yellow   = ftxui::Color::RGB(0xf9, 0xe2, 0xaf);
    const ftxui::Color Peach    = ftxui::Color::RGB(0xfa, 0xb3, 0x87);
    const ftxui::Color Mauve    = ftxui::Color::RGB(0xcb, 0xa6, 0xf7);
    const ftxui::Color Pink     = ftxui::Color::RGB(0xf5, 0xc2, 0xe7);
    const ftxui::Color Teal     = ftxui::Color::RGB(0x94, 0xe2, 0xd5);
}

using namespace ftxui;
using namespace FileManager;
using namespace FTB;

// ---- 预览面板缓存 (使用 DirEntry + 延迟加载) ----
struct PreviewCache {
    std::string key;
    std::string selectedName;
    bool is_dir = false;
    bool exists = false;
    bool is_symlink = false;
    bool is_executable = false;
    bool is_regular = false;
    uintmax_t file_size = 0;
    std::string mod_time;
    std::string icon;
    std::vector<DirEntryInfo> dir_contents;
    std::string text_preview;
    std::vector<ArchiveEntry> archive_contents;
    bool dir_loaded = false;
    bool text_loaded = false;
    bool dir_sorted = false;
    bool archive_loaded = false;
    std::string glow_output;
    bool glow_loaded = false;
    bool glow_completed = false;
};

static PreviewCache g_preview_cache;
static std::mutex g_preview_mutex;

// ---- 预览面板语法高亮器 (全局共享) ----
static FTB::Editor::SyntaxHighlighter g_preview_highlighter;

// ---- Glow Markdown 外部渲染器状态 (全局共享) ----
inline bool g_glow_available = false;
inline bool g_glow_checked = false;
inline bool g_preview_md_show_source = false;

#include "FTB/TextSelection.hpp"

// ---- 字符级选中行渲染 (UTF-8 安全，按码点迭代) ----
inline ftxui::Element RenderLineWithSelection(const std::string& line,
                                               FTB::Editor::Language lang,
                                               int sel_x1, int sel_x2) {
    sel_x1 = std::max(0, sel_x1);
    sel_x2 = std::min(static_cast<int>(line.size()) - 1, sel_x2);

    if (lang == FTB::Editor::Language::NONE) {
        ftxui::Elements parts;
        ftxui::Color fg = TC("main_fg");
        if (sel_x1 > 0)
            parts.push_back(text(line.substr(0, sel_x1)) | ftxui::color(fg));
        if (sel_x1 <= sel_x2)
            parts.push_back(text(line.substr(sel_x1, sel_x2 - sel_x1 + 1)) | ftxui::color(fg) | bgcolor(TC("selection_bg")));
        if (sel_x2 + 1 < static_cast<int>(line.size()))
            parts.push_back(text(line.substr(sel_x2 + 1)) | ftxui::color(fg));
        return hbox(std::move(parts));
    }

    auto tokens = g_preview_highlighter.ParseLine(line);

    // --- 调试: 输出 tokens ---
    {
        std::string tok_info;
        for (const auto& t : tokens) {
            std::string type_name;
            switch (t.type) {
                case FTB::Editor::TokenType::KEYWORD:       type_name = "KW"; break;
                case FTB::Editor::TokenType::STRING:        type_name = "STR"; break;
                case FTB::Editor::TokenType::COMMENT:       type_name = "CMT"; break;
                case FTB::Editor::TokenType::NUMBER:        type_name = "NUM"; break;
                case FTB::Editor::TokenType::FUNCTION:      type_name = "FUNC"; break;
                case FTB::Editor::TokenType::TYPE:          type_name = "TYPE"; break;
                case FTB::Editor::TokenType::OPERATOR:      type_name = "OP"; break;
                case FTB::Editor::TokenType::PREPROCESSOR:  type_name = "PRE"; break;
                case FTB::Editor::TokenType::IDENTIFIER:    type_name = "ID"; break;
                case FTB::Editor::TokenType::PUNCTUATION:   type_name = "PUNCT"; break;
                case FTB::Editor::TokenType::PROPERTY:      type_name = "PROP"; break;
                case FTB::Editor::TokenType::NORMAL:        type_name = "NORM"; break;
                default:                                    type_name = "?"; break;
            }
            tok_info += "[" + type_name + "@" + std::to_string(t.start_pos) + "-" + std::to_string(t.end_pos) + ":'" + t.text.substr(0,6) + "']";
        }
    }

    ftxui::Elements parts;

    auto utf8_char_len = [](unsigned char lead) -> size_t {
        if ((lead & 0x80) == 0) return 1;
        if ((lead & 0xE0) == 0xC0) return 2;
        if ((lead & 0xF0) == 0xE0) return 3;
        if ((lead & 0xF8) == 0xF0) return 4;
        return 1;
    };

    size_t pos = 0;
    while (pos < line.size()) {
        size_t clen = utf8_char_len(static_cast<unsigned char>(line[pos]));
        if (pos + clen > line.size()) clen = 1;

        std::string ch = line.substr(pos, clen);

        ftxui::Color fg = TC("main_fg");
        for (const auto& t : tokens) {
            if (static_cast<int>(pos) >= t.start_pos && static_cast<int>(pos) < t.end_pos) {
                fg = FTB::Editor::SyntaxHighlighter::GetTokenColor(t.type);
                break;
            }
        }

        auto el = text(ch) | ftxui::color(fg);

        int byte_start = static_cast<int>(pos);
        int byte_end = static_cast<int>(pos + clen - 1);
        if (byte_end >= sel_x1 && byte_start <= sel_x2)
            el = el | bgcolor(TC("selection_bg"));

        parts.push_back(std::move(el));
        pos += clen;
    }

    return hbox(std::move(parts));
}

// ---- 更新预览缓存 (只在 selected 变化时调用) ----
inline void UpdatePreviewCache(const std::vector<DirEntryInfo>& entries,
                               int selected, const std::string& currentPath) {
    std::string new_key = currentPath + ":" + std::to_string(selected);

    {
        std::lock_guard<std::mutex> lock(g_preview_mutex);
        if (g_preview_cache.key == new_key) return;
    }

    auto start = std::chrono::steady_clock::now();

    PreviewCache new_cache;
    new_cache.key = new_key;

    if (selected < 0 || selected >= static_cast<int>(entries.size())) {
        std::lock_guard<std::mutex> lock(g_preview_mutex);
        g_preview_cache = std::move(new_cache);
        return;
    }

    // 直接从 DirEntry 缓存获取信息，零文件系统调用
    const DirEntryInfo& entry = entries[selected];
    new_cache.selectedName = entry.name;
    new_cache.is_dir = entry.is_dir;
    new_cache.exists = entry.exists;
    new_cache.is_symlink = entry.is_symlink;
    new_cache.is_executable = entry.is_executable;
    new_cache.is_regular = entry.is_regular;
    new_cache.file_size = entry.file_size;
    new_cache.mod_time = entry.mod_time;
    new_cache.icon = entry.icon;

    std::lock_guard<std::mutex> lock(g_preview_mutex);
    g_preview_cache = std::move(new_cache);
}

// ---- 异步延迟加载目录内容 (使用 DirEntryInfo, 后台预排序) ----
inline void EnsureDirContentsLoaded(const std::string& dirPath) {
    std::lock_guard<std::mutex> lock(g_preview_mutex);
    if (g_preview_cache.dir_loaded) return;
    g_preview_cache.dir_loaded = true;

    std::thread([dirPath]() {
        try {
            auto entries = FileManager::getDirectoryEntries(dirPath);
            {
                auto& cfg = FTB::ConfigManager::GetInstance()->GetConfig();
                auto mode = FTB::SortModeFromString(cfg.style.sort_mode);
                FTB::SortEntries(entries, mode);
            }
            std::lock_guard<std::mutex> lock2(g_preview_mutex);
            g_preview_cache.dir_contents = std::move(entries);
            g_preview_cache.dir_sorted = true;
        } catch (...) {
            std::lock_guard<std::mutex> lock2(g_preview_mutex);
            g_preview_cache.dir_contents.clear();
        }
    }).detach();
}

// ---- 异步延迟加载文本预览 ----
inline void EnsureTextPreviewLoaded(const std::string& filePath, uintmax_t fileSize) {
    std::lock_guard<std::mutex> lock(g_preview_mutex);
    if (g_preview_cache.text_loaded) return;
    g_preview_cache.text_loaded = true;

    if (fileSize < 100 * 1024) {
        // 异步加载文本预览
        std::thread([filePath]() {
            try {
                auto content = FileManager::readFileContent(filePath, 1, 100);
                std::lock_guard<std::mutex> lock2(g_preview_mutex);
                g_preview_cache.text_preview = std::move(content);
            } catch (...) {
                std::lock_guard<std::mutex> lock2(g_preview_mutex);
                g_preview_cache.text_preview.clear();
            }
        }).detach();
    }
}

// ---- 异步延迟加载档案内容 ----
inline void EnsureArchiveContentsLoaded(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(g_preview_mutex);
    if (g_preview_cache.archive_loaded) return;
    g_preview_cache.archive_loaded = true;

    std::thread([filePath]() {
        auto entries = ListArchiveContents(filePath);
        std::lock_guard<std::mutex> lock2(g_preview_mutex);
        g_preview_cache.archive_contents = std::move(entries);
    }).detach();
}

// ---- 格式化文件大小 ----
inline std::string FormatFileSize(uintmax_t size) {
    if (size < 1024) return std::to_string(size) + " B";
    if (size < 1024 * 1024) return std::to_string(size / 1024) + " KB";
    return std::to_string(size / (1024 * 1024)) + " MB";
}

// ---- 剪贴板待处理项渲染 ----
inline Element RenderPendingFiles() {
    auto& clipboard = ClipboardManager::getInstance();
    const auto& items = clipboard.getItems();

    Elements els;

    els.push_back(
        hbox({
            text(" Clipboard") | color(TC("title")) | bold,
            filler(),
            text(!items.empty() && clipboard.hasModeSelected()
                ? (clipboard.isCutMode() ? "CUT" : "COPY")
                : "") | color(clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied")) | bold
        })
    );
    els.push_back(separator() | color(TC("main_border")));

    if (items.empty()) {
        els.push_back(text("  (empty)") | color(TC("dim")));
    } else {
        for (const auto& path : items) {
            fs::path p(path);
            auto filename = p.filename().string();
            auto icon = FTB::Icons::GetIconForPath(p, fs::is_directory(p));
            auto fileElement = text("  " + icon + filename);

            if (clipboard.hasModeSelected()) {
                fileElement = fileElement | color(clipboard.isCutMode() ? TC("marker_cut") : TC("marker_copied"));
            } else {
                fileElement = fileElement | color(TC("main_fg"));
            }
            els.push_back(fileElement);
        }
    }

    return vbox(std::move(els)) | bgcolor(TC("status_bg"));
}

// ---- ANSI SGR 转义序列解析器 (用于 glow 输出) ----
inline ftxui::Element AnsiStringToElement(const std::string& ansi_text) {
    if (ansi_text.empty()) return ftxui::text("");

    struct AnsiStyle {
        bool bold = false;
        bool underline = false;
        ftxui::Color fg = ftxui::Color::Default;
        ftxui::Color bg = ftxui::Color::Default;
        bool operator==(const AnsiStyle& o) const {
            return bold == o.bold && underline == o.underline && fg == o.fg && bg == o.bg;
        }
        bool operator!=(const AnsiStyle& o) const { return !(*this == o); }
    };

    std::vector<ftxui::Elements> lines;
    ftxui::Elements current_line;
    AnsiStyle current_style;
    std::string current_text;

    auto flush_text = [&]() {
        if (!current_text.empty()) {
            ftxui::Element el = ftxui::text(current_text);
            if (current_style.bold)   el = el | ftxui::bold;
            if (current_style.underline) el = el | ftxui::underlined;
            if (current_style.fg != ftxui::Color::Default) el = el | ftxui::color(current_style.fg);
            if (current_style.bg != ftxui::Color::Default) el = el | ftxui::bgcolor(current_style.bg);
            current_line.push_back(std::move(el));
            current_text.clear();
        }
    };

    size_t i = 0;
    while (i < ansi_text.size()) {
        if (ansi_text[i] == '\033' && i + 1 < ansi_text.size() && ansi_text[i + 1] == '[') {
            flush_text();
            i += 2;

            std::vector<int> params;
            int current_param = 0;

            while (i < ansi_text.size() && ansi_text[i] != 'm') {
                if (ansi_text[i] == ';') {
                    params.push_back(current_param);
                    current_param = 0;
                    i++;
                } else if (ansi_text[i] >= '0' && ansi_text[i] <= '9') {
                    current_param = current_param * 10 + (ansi_text[i] - '0');
                    i++;
                } else {
                    i++;
                }
            }
            if (i < ansi_text.size() && ansi_text[i] == 'm') {
                i++;
            }
            params.push_back(current_param);

            AnsiStyle new_style = current_style;
            size_t p_idx = 0;
            while (p_idx < params.size()) {
                int p = params[p_idx];
                switch (p) {
                    case 0: new_style = AnsiStyle(); break;
                    case 1: new_style.bold = true; break;
                    case 4: new_style.underline = true; break;
                    case 22: new_style.bold = false; break;
                    case 24: new_style.underline = false; break;
                    case 30: case 31: case 32: case 33:
                    case 34: case 35: case 36: case 37:
                        new_style.fg = ftxui::Color::Palette16(p - 30); break;
                    case 38:
                        if (p_idx + 1 < params.size()) {
                            if (params[p_idx + 1] == 5 && p_idx + 2 < params.size()) {
                                int c = params[p_idx + 2];
                                if (c >= 0 && c <= 255)
                                    new_style.fg = ftxui::Color::Palette256(static_cast<uint8_t>(c));
                                p_idx += 2;
                            } else if (params[p_idx + 1] == 2 && p_idx + 4 < params.size()) {
                                new_style.fg = ftxui::Color::RGB(
                                    static_cast<uint8_t>(params[p_idx + 2]),
                                    static_cast<uint8_t>(params[p_idx + 3]),
                                    static_cast<uint8_t>(params[p_idx + 4]));
                                p_idx += 4;
                            }
                        }
                        break;
                    case 39: new_style.fg = ftxui::Color::Default; break;
                    case 40: case 41: case 42: case 43:
                    case 44: case 45: case 46: case 47:
                        new_style.bg = ftxui::Color::Palette16(p - 40); break;
                    case 48:
                        if (p_idx + 1 < params.size()) {
                            if (params[p_idx + 1] == 5 && p_idx + 2 < params.size()) {
                                int c = params[p_idx + 2];
                                if (c >= 0 && c <= 255)
                                    new_style.bg = ftxui::Color::Palette256(static_cast<uint8_t>(c));
                                p_idx += 2;
                            } else if (params[p_idx + 1] == 2 && p_idx + 4 < params.size()) {
                                new_style.bg = ftxui::Color::RGB(
                                    static_cast<uint8_t>(params[p_idx + 2]),
                                    static_cast<uint8_t>(params[p_idx + 3]),
                                    static_cast<uint8_t>(params[p_idx + 4]));
                                p_idx += 4;
                            }
                        }
                        break;
                    case 49: new_style.bg = ftxui::Color::Default; break;
                    case 90: case 91: case 92: case 93:
                    case 94: case 95: case 96: case 97:
                        new_style.fg = ftxui::Color::Palette16(p - 90 + 8); break;
                    case 100: case 101: case 102: case 103:
                    case 104: case 105: case 106: case 107:
                        new_style.bg = ftxui::Color::Palette16(p - 100 + 8); break;
                }
                p_idx++;
            }
            current_style = new_style;
        } else if (ansi_text[i] == '\n') {
            flush_text();
            lines.push_back(std::move(current_line));
            current_line.clear();
            i++;
        } else if (ansi_text[i] == '\r') {
            i++;
        } else {
            current_text += ansi_text[i];
            i++;
        }
    }

    flush_text();
    if (!current_line.empty()) {
        lines.push_back(std::move(current_line));
    }

    ftxui::Elements result_lines;
    for (auto& line : lines) {
        if (line.empty()) {
            result_lines.push_back(ftxui::text(""));
        } else {
            result_lines.push_back(ftxui::hbox(std::move(line)));
        }
    }

    if (result_lines.empty()) return ftxui::text("");
    return ftxui::vbox(std::move(result_lines));
}

// ---- 异步 glow markdown 预览加载 ----
inline void EnsureGlowPreviewLoaded(const std::string& filePath, int panel_width) {
    std::lock_guard<std::mutex> lock(g_preview_mutex);
    if (g_preview_cache.glow_loaded) return;

    if (!g_glow_checked) {
        g_glow_checked = true;
        FILE* fp = popen("command -v glow 2>/dev/null", "r");
        if (fp) {
            char buf[64];
            if (fgets(buf, sizeof(buf), fp) != nullptr) {
                g_glow_available = true;
            }
            pclose(fp);
        }
    }
    if (!g_glow_available) return;

    g_preview_cache.glow_loaded = true;

    std::thread([filePath, panel_width]() {
        try {
            int glow_width = std::max(20, panel_width - 2);
            std::string cmd = "CLICOLOR_FORCE=1 glow --width=" + std::to_string(glow_width)
                            + " --style=dark -n \"" + filePath + "\" 2>/dev/null";
            FILE* fp = popen(cmd.c_str(), "r");
            if (!fp) return;

            std::string result;
            char buf[4096];
            while (fgets(buf, sizeof(buf), fp) != nullptr) {
                result += buf;
            }
            int status = pclose(fp);

            std::lock_guard<std::mutex> lock2(g_preview_mutex);
            if (status == 0 && !result.empty()) {
                g_preview_cache.glow_output = std::move(result);
            }
            g_preview_cache.glow_completed = true;
        } catch (...) {
            std::lock_guard<std::mutex> lock2(g_preview_mutex);
            g_preview_cache.glow_completed = true;
        }
    }).detach();
}

// ---- yazi 风格: 文件预览面板 (使用 DirEntry 缓存) ----
inline Element CreateDetailElement(const std::vector<DirEntryInfo>& entries,
                                   int selected,
                                   const std::string& currentPath) {
    // 更新缓存 (只在 selected 变化时真正查询)
    UpdatePreviewCache(entries, selected, currentPath);

    // 从缓存读取
    PreviewCache cache;
    {
        std::lock_guard<std::mutex> lock(g_preview_mutex);
        cache = g_preview_cache;
    }

    Elements info_elements;

    // 检测是否为 Markdown 文件 (用于标题指示器)
    std::string ext;
    auto dot = cache.selectedName.find_last_of('.');
    if (dot != std::string::npos) {
        ext = cache.selectedName.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    bool is_md_file_preview = (ext == ".md" || ext == ".markdown");

    // 标题
    std::string preview_label = " Preview";
    if (is_md_file_preview && g_preview_md_show_source) preview_label += " (src)";
    info_elements.push_back(
        hbox({
            text(preview_label) | color(TC("title")) | bold,
            filler()
        })
    );
    info_elements.push_back(separator() | color(TC("main_border")));

    // 文件名 + 图标
    Color name_color = cache.is_dir ? TC("directory") : TC("main_fg");
    info_elements.push_back(
        hbox({
            text("  " + cache.icon + " "),
            text(cache.selectedName) | color(name_color) | bold
        })
    );

    // 文件类型
    if (!cache.selectedName.empty()) {
        std::string type_str = cache.is_dir ? "directory" : "file";
        if (cache.is_symlink) type_str = "symlink";
        if (!cache.is_dir && cache.is_executable) {
            // 排除 WSL Windows 分区上 stat 误报的可执行位
            auto dot = cache.selectedName.find_last_of('.');
            if (dot == std::string::npos) {
                type_str = "executable";
            } else {
                std::string ext = cache.selectedName.substr(dot);
                for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (ext == ".exe" || ext == ".sh" || ext == ".bash" || ext == ".bin" ||
                    ext == ".run" || ext == ".com" || ext == ".bat" || ext == ".cmd" ||
                    ext == ".out" || ext == ".appimage" || ext == ".msi")
                    type_str = "executable";
            }
        }
        info_elements.push_back(text("  " + type_str) | color(TC("dim")) | dim);
    }

    // 文件大小
    if (!cache.is_dir && cache.is_regular) {
        std::string size_str;
        if (cache.file_size < 1024) size_str = std::to_string(cache.file_size) + " B";
        else if (cache.file_size < 1024 * 1024) size_str = std::to_string(cache.file_size / 1024) + " KB";
        else size_str = std::to_string(cache.file_size / (1024 * 1024)) + " MB";
        info_elements.push_back(text("  " + size_str) | color(TC("syn_number")));
    }

    // 修改时间
    if (!cache.mod_time.empty()) {
        info_elements.push_back(text("  " + cache.mod_time) | color(TC("time")));
    }

    // 目录内容预览 (异步延迟加载, 使用预排序 DirEntryInfo)
    if (cache.is_dir && cache.exists) {
        std::string dirPath = (fs::path(currentPath) / cache.selectedName).string();
        EnsureDirContentsLoaded(dirPath);
        {
            std::lock_guard<std::mutex> lock(g_preview_mutex);
            cache.dir_contents = g_preview_cache.dir_contents;
            cache.dir_sorted = g_preview_cache.dir_sorted;
        }

        info_elements.push_back(separator() | color(TC("main_border")));
        if (cache.dir_contents.empty()) {
            info_elements.push_back(text("  Loading...") | color(TC("dim")) | dim);
        } else {
            int show_count = std::min(static_cast<int>(cache.dir_contents.size()), 8);
            for (int i = 0; i < show_count; ++i) {
                const auto& entry = cache.dir_contents[i];
                bool is_hidden = (!entry.name.empty() && entry.name[0] == '.');

                std::string branch = (i == show_count - 1) ? u8"\u2514\u2500\u2500 " : u8"\u251C\u2500\u2500 ";

                Element name_el;
                if (entry.is_dir) {
                    name_el = text(" " + entry.icon + entry.name) | bgcolor(TC("directory")) | color(TC("main_bg")) | bold;
                } else if (is_hidden) {
                    name_el = text(" " + entry.icon + entry.name) | color(TC("hidden"));
                } else {
                    name_el = text(" " + entry.icon + entry.name) | color(TC("main_fg"));
                }

                info_elements.push_back(
                    hbox({
                        text("  " + branch) | color(TC("dim")),
                        name_el
                    })
                );
            }
            if (static_cast<int>(cache.dir_contents.size()) > 8) {
                info_elements.push_back(
                    text("    \u2514\u2500\u2500 +" + std::to_string(cache.dir_contents.size() - 8) + " more") | color(TC("dim")) | dim
                );
            }
        }
    }

    // 图片预览 (优先于文本预览, SSH 模式下不可用)
    std::string filePath = (fs::path(currentPath) / cache.selectedName).string();
    bool is_image = false;
#ifdef FTB_ENABLE_SSH
    bool ssh_mode = (FileManager::getSSHConnection() != nullptr);
    if (!ssh_mode) {
        is_image = !cache.is_dir && FTB::ImagePreview::IsImageFile(filePath);
    }
#else
    is_image = !cache.is_dir && FTB::ImagePreview::IsImageFile(filePath);
#endif

    // 计算预览面板实际宽度
    auto term_dim = Terminal::Size();
    auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
    int preview_panel_width = std::max(20, static_cast<int>(term_dim.dimx * cfg.layout.preview_ratio));
    int usable_line_width = std::max(20, preview_panel_width - 6);  // 减去行号和边距

    if (is_image) {
        int img_w = std::max(20, preview_panel_width - 4);
        int img_h = std::max(10, term_dim.dimy - 12);

        FTB::ImagePreview::LoadAsync(filePath, img_w, img_h);

        FTB::ImageCache img_cache;
        if (FTB::ImagePreview::GetCached(filePath, img_cache) && img_cache.is_image) {
            info_elements.push_back(separator() | color(TC("main_border")));
            for (const auto& line : img_cache.image_lines) {
                Elements char_els;
                for (const auto& p : line.pixels) {
                    char_els.push_back(
                        text(L"\u2588") |
                        color(Color::RGB(p.r, p.g, p.b))
                    );
                }
                info_elements.push_back(hbox(char_els));
            }
        } else {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading image...") | color(TC("dim")) | dim);
        }
    }

    // 档案文件预览 (异步加载, 使用系统命令)
    bool is_archive = !cache.is_dir && cache.is_regular && IsArchiveFile(cache.selectedName);
    if (is_archive) {
        std::string archivePath = (fs::path(currentPath) / cache.selectedName).string();
        {
            std::lock_guard<std::mutex> lock(g_preview_mutex);
            cache.archive_contents = g_preview_cache.archive_contents;
            cache.archive_loaded = g_preview_cache.archive_loaded;
        }
        if (!cache.archive_loaded) {
            EnsureArchiveContentsLoaded(archivePath);
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading archive...") | color(TC("dim")) | dim);
        } else if (!cache.archive_contents.empty()) {
            info_elements.push_back(separator() | color(TC("main_border")));
            const int max_archive_entries = 50;
            int show_count = std::min(static_cast<int>(cache.archive_contents.size()), max_archive_entries);

            // 排序: 目录在前, 然后按名称字母序
            std::vector<ArchiveEntry> sorted = cache.archive_contents;
            {
                auto& cfg = FTB::ConfigManager::GetInstance()->GetConfig();
                auto mode = FTB::SortModeFromString(cfg.style.sort_mode);
                std::sort(sorted.begin(), sorted.end(),
                    [mode](const ArchiveEntry& a, const ArchiveEntry& b) {
                        if (a.is_dir != b.is_dir) return a.is_dir;
                        (void)mode;
                        // ArchiveEntry uses simple name sort for now
                        std::string na = a.name, nb = b.name;
                        std::transform(na.begin(), na.end(), na.begin(), ::tolower);
                        std::transform(nb.begin(), nb.end(), nb.begin(), ::tolower);
                        return na < nb;
                    });
            }

            for (int i = 0; i < show_count; ++i) {
                const auto& entry = sorted[i];
                // unzip/tar 对目录名附加尾随 /，先去掉以免干扰深度和显示名
                std::string clean_name = entry.name;
                if (!clean_name.empty() && clean_name.back() == '/')
                    clean_name.pop_back();
                // 计算路径深度 (按 / 分割)
                int depth = 0;
                for (char c : clean_name) if (c == '/') depth++;
                std::string indent(depth * 2, ' ');

                std::string branch = (i == show_count - 1) ? u8"\u2514\u2500\u2500 " : u8"\u251C\u2500\u2500 ";

                // 取最后一个路径组件作为显示名
                auto name_part = clean_name;
                auto slash_pos = name_part.rfind('/');
                if (slash_pos != std::string::npos)
                    name_part = name_part.substr(slash_pos + 1);
                if (entry.is_dir) name_part += "/";

                Element name_el;
                if (entry.is_dir) {
                    name_el = text(" " + name_part) | color(TC("directory")) | bold;
                } else {
                    name_el = text(" " + name_part) | color(TC("main_fg"));
                }

                Elements row = {
                    text("  " + indent + branch) | color(TC("dim")),
                    name_el
                };

                if (!entry.is_dir && entry.size > 0) {
                    row.push_back(filler());
                    row.push_back(text(" " + FormatFileSize(entry.size)) | color(TC("syn_number")));
                }

                info_elements.push_back(hbox(std::move(row)));
            }

            if (static_cast<int>(cache.archive_contents.size()) > max_archive_entries) {
                info_elements.push_back(
                    text("    \u2514\u2500\u2500 +" +
                         std::to_string(cache.archive_contents.size() - max_archive_entries) +
                         " more entries") | color(TC("dim")) | dim
                );
            }
        }
        // archive_contents empty = 命令不存在或空档案 → 静默降级, 不显示额外内容
    }

    // 文本文件预览 (异步延迟加载, 带语法高亮 or glow Markdown 渲染)
    if (!cache.is_dir && cache.is_regular && cache.file_size < 100 * 1024 && !is_image && !is_archive) {
        std::string filePath = (fs::path(currentPath) / cache.selectedName).string();

        // 检测 Markdown 文件
        std::string ext;
        auto dot = cache.selectedName.find_last_of('.');
        if (dot != std::string::npos) {
            ext = cache.selectedName.substr(dot);
            for (auto& c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        bool is_markdown = (ext == ".md" || ext == ".markdown");

        bool glow_used = false;

        if (is_markdown && !g_preview_md_show_source) {
            EnsureGlowPreviewLoaded(filePath, preview_panel_width);
            bool glow_ready = false;
            bool glow_done = false;
            {
                std::lock_guard<std::mutex> lock(g_preview_mutex);
                glow_ready = g_preview_cache.glow_completed && !g_preview_cache.glow_output.empty();
                glow_done = g_preview_cache.glow_completed;
            }

            if (glow_ready) {
                info_elements.push_back(separator() | color(TC("main_border")));
                std::string glow_out;
                {
                    std::lock_guard<std::mutex> lock(g_preview_mutex);
                    glow_out = g_preview_cache.glow_output;
                }
                info_elements.push_back(AnsiStringToElement(glow_out) | flex);
                glow_used = true;
            } else if (glow_done || (g_glow_checked && !g_glow_available)) {
                // glow 完成但无输出 / glow 不可用 → 回退到语法高亮文本预览
            } else {
                // glow 正在检测或加载中
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading markdown preview...") | color(TC("dim")) | dim);
                glow_used = true;
            }
        }

        if (!glow_used) {
            EnsureTextPreviewLoaded(filePath, cache.file_size);
            {
                std::lock_guard<std::mutex> lock(g_preview_mutex);
                cache.text_preview = g_preview_cache.text_preview;
            }

            if (!cache.text_preview.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                int max_lines = std::max(10, term_dim.dimy - 12);
                int line_num_width = std::max(1, static_cast<int>(std::to_string(max_lines).size()));

                // 更新选择状态中的元数据
                g_preview_line_num_width = line_num_width;
                g_preview_max_lines = max_lines;

                // 设置语法高亮语言
                auto lang = FTB::Editor::SyntaxHighlighter::DetectLanguage(cache.selectedName);
                g_preview_highlighter.SetLanguage(lang);
                g_preview_highlighter.ResetMultiLineState();

                // 保存文本快照用于选中提取
                g_preview_sel.lines.clear();

                Elements text_lines;
                std::istringstream iss(cache.text_preview);
                std::string line;
                int line_num = 1;
                int sel_y1 = g_preview_sel.active ? std::min(g_preview_sel.anchor_y, g_preview_sel.current_y) : -1;
                int sel_y2 = g_preview_sel.active ? std::max(g_preview_sel.anchor_y, g_preview_sel.current_y) : -1;

                while (std::getline(iss, line) && line_num <= max_lines) {
                    if (static_cast<int>(line.size()) > usable_line_width)
                        line = line.substr(0, usable_line_width - 3) + "...";

                    if (line_num <= 3) {
                        // 检测多字节 UTF-8 字节序列
                        std::string multi_bytes;
                        bool in_mb = false;
                        int mb_start = 0;
                        for (size_t bi = 0; bi < line.size() && bi < 60; bi++) {
                            unsigned char bc = line[bi];
                            if (bc >= 0x80) {
                                if (!in_mb) {
                                    mb_start = bi;
                                    in_mb = true;
                                    multi_bytes += "[@" + std::to_string(bi) + ":";
                                }
                                multi_bytes += std::to_string((int)bc) + " ";
                            } else {
                                if (in_mb) {
                                    int nbytes = bi - mb_start;
                                    multi_bytes += "w=" + std::to_string(nbytes >= 3 ? 2 : 1) + "] ";
                                    in_mb = false;
                                }
                            }
                        }
                        if (in_mb) {
                            multi_bytes += "w=2] ";
                        }
                        int total_byte = static_cast<int>(line.size());
                        int raw_nonascii = 0;
                        for (auto c : line)
                            if (static_cast<unsigned char>(c) >= 0x80) raw_nonascii++;
                    }

                    g_preview_sel.lines.push_back(line);

                    int csel_x1 = -1, csel_x2 = -1;
                    if (g_preview_sel.active) {
                        int line_y = g_preview_box.y_min + line_num - 1;
                        if (line_y >= sel_y1 && line_y <= sel_y2) {
                            int content_x = g_preview_box.x_min + line_num_width + 1;
                            csel_x1 = std::min(g_preview_sel.anchor_x, g_preview_sel.current_x) - content_x;
                            csel_x2 = std::max(g_preview_sel.anchor_x, g_preview_sel.current_x) - content_x;
                        }
                    }

                    ftxui::Element line_content;
                    if (g_preview_sel.active) {
                        line_content = RenderLineWithSelection(line, lang, csel_x1, csel_x2);
                    } else if (lang != FTB::Editor::Language::NONE) {
                        line_content = RenderLineWithSelection(line, lang, -1, -1);
                    } else {
                        line_content = text(line) | color(TC("main_fg"));
                    }

                    text_lines.push_back(
                        hbox({
                            text(std::to_string(line_num)) | color(TC("syn_line_number")) | size(WIDTH, EQUAL, line_num_width),
                            text(" "),
                            line_content
                        })
                    );
                    line_num++;
                }

                info_elements.push_back(
                    vbox(std::move(text_lines)) | reflect(g_preview_box)
                );

                if (line_num > max_lines) {
                    info_elements.push_back(
                        text("  ... more lines") | color(TC("dim")) | dim
                    );
                }
            }
        }
    }

    info_elements.push_back(separator() | color(TC("main_border")));

    // 剪贴板区域
    info_elements.push_back(RenderPendingFiles());

    return vbox(info_elements) | bgcolor(TC("main_bg")) | flex | yframe;
}

#endif // DETAIL_ELEMENT_HPP
