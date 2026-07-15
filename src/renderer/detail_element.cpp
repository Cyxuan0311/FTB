#include "renderer/detail_element.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cctype>
#include <mutex>
#include <thread>
#include <algorithm>
#include <functional>
#include <ftxui/dom/elements.hpp>

#include "core/MainUI.hpp"
#include "core/AnsiParser.hpp"
#include "ops/PluginManager.hpp"
#include "preview/ArchivePreview.hpp"
#include "preview/ImagePreview.hpp"
#include "protocols/ImageOutputManager.hpp"
#include "utils/PerfLogger.hpp"
#include "preview/MarkdownPreview.hpp"
#include "preview/SpreadsheetPreview.hpp"
#include "preview/MediaPreview.hpp"
#include "preview/AudioPreview.hpp"
#include "preview/PdfPreview.hpp"
#include "preview/DocPreview.hpp"
#include "preview/HexPreview.hpp"
#include "browser/ClipboardManager.hpp"
#include "renderer/IconMapper.hpp"
#include "browser/SortMode.hpp"
#include "editor/SyntaxHighlighter.hpp"
#include "renderer/TextSelection.hpp"
#include "utils/UnicodeUtil.hpp"


namespace fs = std::filesystem;

using namespace ftxui;
using namespace FileManager;

namespace FTB {

static Element RenderLineWithSelection(const std::string& line,
                                        FTB::Editor::Language lang,
                                        int sel_x1, int sel_x2) {
    // sel_x1/sel_x2 are display column positions. Convert to byte offsets.
    size_t byte_x1 = UnicodeUtil::ByteOffsetFromDisplayColumn(line, std::max(0, sel_x1));
    size_t byte_x2 = UnicodeUtil::ByteOffsetFromDisplayColumn(line, std::max(0, sel_x2 + 1));
    int tab_width = FTB::ConfigManager::GetInstance()->GetConfig().preview.tab_width;

    if (lang == FTB::Editor::Language::NONE) {
        ftxui::Elements parts;
        ftxui::Color fg = TC("main_fg");
        if (byte_x1 > 0)
            parts.push_back(text(line.substr(0, byte_x1)) | ftxui::color(fg));
        if (byte_x1 <= byte_x2 && byte_x1 < line.size())
            parts.push_back(text(line.substr(byte_x1, byte_x2 - byte_x1)) | ftxui::color(fg) | bgcolor(TC("selection_bg")));
        if (byte_x2 < line.size())
            parts.push_back(text(line.substr(byte_x2)) | ftxui::color(fg));
        return hbox(std::move(parts));
    }

    auto& hl = PreviewCache::Instance().Highlighter();
    auto tokens = hl.ParseLine(line);

    ftxui::Elements parts;

    auto utf8_char_len = [](unsigned char lead) -> size_t {
        if ((lead & 0x80) == 0) return 1;
        if ((lead & 0xE0) == 0xC0) return 2;
        if ((lead & 0xF0) == 0xE0) return 3;
        if ((lead & 0xF8) == 0xF0) return 4;
        return 1;
    };

    size_t pos = 0;
    int display_col = 0;
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

        Element el;
        uint32_t codepoint = 0;
        UnicodeUtil::DecodeUtf8(ch.c_str(), &codepoint);
        if (codepoint == 0x09) {
            int tab_w = tab_width - (display_col % tab_width);
            el = text(std::string(tab_w, ' ')) | ftxui::color(fg);
            display_col += tab_w;
        } else {
            el = text(ch) | ftxui::color(fg);
            display_col += UnicodeUtil::CodepointDisplayWidth(codepoint);
        }

        int byte_start = static_cast<int>(pos);
        int byte_end = static_cast<int>(pos + clen - 1);
        if (byte_end >= static_cast<int>(byte_x1) && byte_start < static_cast<int>(byte_x2))
            el = el | bgcolor(TC("selection_bg"));

        parts.push_back(std::move(el));
        pos += clen;
    }

    return hbox(std::move(parts));
}

static std::string FormatFileSize(uintmax_t size) {
    if (size < 1024) return std::to_string(size) + " B";
    if (size < 1024 * 1024) return std::to_string(size / 1024) + " KB";
    return std::to_string(size / (1024 * 1024)) + " MB";
}

static std::string SanitizePreviewLine(const std::string& line) {
    std::string out;
    out.reserve(line.size());
    for (size_t i = 0; i < line.size();) {
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (c == 0x00) { ++i; continue; }
        if (c == 0x1B) {
            if (i + 1 < line.size() && line[i + 1] == '[') {
                i += 2;
                while (i < line.size()) {
                    c = line[i];
                    ++i;
                    if (isalpha(c) || c == '~') break;
                }
                continue;
            }
            ++i;
            continue;
        }
        out += line[i];
        ++i;
    }
    return out;
}

static std::vector<std::string> SplitLines(const std::string& s) {
    std::vector<std::string> lines;
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line))
        lines.push_back(line);
    return lines;
}

// Unified scrollable content inserter.
// Takes a vector of per-line Elements, applies scroll_y skip/max_show limit,
// and pushes scroll indicator(s) + content to info_elements.
// If total lines <= max_show, scroll is forcibly disabled (skip=0, no indicators).
static void PushScrollableContent(Elements& info_elements,
                                   Elements content_lines,
                                   int scroll_y, int max_show) {
    int total = static_cast<int>(content_lines.size());
    if (total == 0) return;

    bool needs_scroll = total > max_show;
    int skip = needs_scroll ? std::min(scroll_y, total) : 0;
    int show = needs_scroll ? std::min(total - skip, max_show) : total;

    if (skip > 0) {
        info_elements.push_back(
            text("  ... " + std::to_string(skip) + " lines above (Alt+K to scroll up)") | color(TC("dim")) | dim
        );
    }
    Elements visible;
    for (int i = skip; i < skip + show; i++)
        visible.push_back(std::move(content_lines[i]));
    info_elements.push_back(vbox(std::move(visible)) | flex);

    int remaining = total - skip - show;
    if (remaining > 0) {
        info_elements.push_back(
            text("  ... " + std::to_string(remaining) + " more lines (Alt+J to scroll down)") | color(TC("dim")) | dim
        );
    }
}

// Convenience: split ANSI text into lines, convert each, then call PushScrollableContent.
static void PushAnsiScrollable(Elements& info_elements,
                                const std::string& ansi_output,
                                int scroll_y, int max_show) {
    auto lines = SplitLines(ansi_output);
    if (lines.empty()) return;
    Elements content;
    for (const auto& l : lines)
        content.push_back(FTB::AnsiStringToElement(l));
    PushScrollableContent(info_elements, std::move(content), scroll_y, max_show);
}

Element CreateDetailElement(const std::vector<DirEntryInfo>& entries,
                            int selected,
                            const std::string& currentPath,
                            int scroll_y,
                            int scroll_x) {
    auto& cache = PreviewCache::Instance();

    cache.Update(entries, selected, currentPath);

    PreviewData data = cache.Copy();

    Elements info_elements;

    auto term_dim = Terminal::Size();
    auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
    auto& cfg_preview = FTB::ConfigManager::GetInstance()->GetConfig().preview;
    int preview_panel_width = std::max(20, static_cast<int>(term_dim.dimx * cfg.layout.preview_ratio));
    int usable_line_width = std::max(20, preview_panel_width - 6);
    int preview_max_lines = std::max(5, term_dim.dimy - 14);

    std::string ext;
    bool is_md_file_preview = false;
    bool is_spreadsheet = false;
    bool is_media = false;
    bool is_audio = false;
    bool is_pdf = false;
    bool is_doc = false;

    auto dot = data.selectedName.find_last_of('.');
    if (dot != std::string::npos) {
        ext = data.selectedName.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    is_md_file_preview = MarkdownPreview::IsMarkdownFile(data.selectedName);
    is_spreadsheet = SpreadsheetPreview::IsSpreadsheetFile(data.selectedName);
    is_media = MediaPreview::IsMediaFile(data.selectedName);
    is_audio = AudioPreview::IsAudioFile(data.selectedName);
    is_pdf = PdfPreview::IsPdfFile(data.selectedName);
    is_doc = DocPreview::IsDocFile(data.selectedName);

    std::string preview_label = " Preview";
    if (HexPreview::IsBinaryFile(data.selectedName) && HexPreview::IsEnabled()
        && !is_spreadsheet && !is_media && !is_audio && !is_pdf && !is_doc
        && !FTB::ImagePreview::IsImageFile(data.selectedName))
        preview_label += " (hex)";
    if (is_audio && AudioPreview::IsEnabled()) preview_label += " (aud)";
    if (is_md_file_preview && MarkdownPreview::ShowSource()) preview_label += " (src)";
    if (is_spreadsheet && SpreadsheetPreview::ShowSource()) preview_label += " (src)";
    if (is_pdf && PdfPreview::ShowSource()) preview_label += " (src)";
    if (is_doc && DocPreview::ShowSource()) preview_label += " (src)";
    if (is_media && !MediaPreview::IsEnabled()) preview_label += " (disabled)";
    if (is_audio && !AudioPreview::IsEnabled()) preview_label += " (disabled)";
    info_elements.push_back(
        hbox({
            text(preview_label) | color(TC("title")) | bold,
            filler()
        })
    );
    info_elements.push_back(separator() | color(TC("main_border")));

    Color name_color = data.is_dir ? TC("directory") : TC("main_fg");
    info_elements.push_back(
        hbox({
            text("  " + data.icon + " "),
            text(data.selectedName) | color(name_color) | bold
        })
    );

    if (!data.selectedName.empty()) {
        std::string type_str = data.is_dir ? "directory" : "file";
        if (data.is_symlink) type_str = "symlink";
        if (!data.is_dir && data.is_executable) {
            auto dot = data.selectedName.find_last_of('.');
            if (dot == std::string::npos) {
                type_str = "executable";
            } else {
                std::string ext2 = data.selectedName.substr(dot);
                for (auto& c : ext2) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (ext2 == ".exe" || ext2 == ".sh" || ext2 == ".bash" || ext2 == ".bin" ||
                    ext2 == ".run" || ext2 == ".com" || ext2 == ".bat" || ext2 == ".cmd" ||
                    ext2 == ".out" || ext2 == ".appimage" || ext2 == ".msi")
                    type_str = "executable";
            }
        }
        info_elements.push_back(text("  " + type_str) | color(TC("dim")) | dim);
    }

    if (!data.is_dir && data.is_regular) {
        std::string size_str;
        if (data.file_size < 1024) size_str = std::to_string(data.file_size) + " B";
        else if (data.file_size < 1024 * 1024) size_str = std::to_string(data.file_size / 1024) + " KB";
        else size_str = std::to_string(data.file_size / (1024 * 1024)) + " MB";
        info_elements.push_back(text("  " + size_str) | color(TC("syn_number")));
    }

    if (!data.mod_time.empty()) {
        info_elements.push_back(text("  " + data.mod_time) | color(TC("time")));
    }

    if (FTB::ConfigManager::GetInstance()->IsNoPreviewExtension(ext)) {
        info_elements.push_back(separator() | color(TC("main_border")));
        info_elements.push_back(text("  Preview not available") | color(TC("dim")) | dim);
        info_elements.push_back(separator() | color(TC("main_border")));
        return vbox(info_elements) | bgcolor(TC("main_bg")) | flex | yframe;
    }

#ifdef FTB_ENABLE_PLUGINS
    // === Plugin Previewer ===
    bool plugin_preview_handled = false;
    if (!data.is_dir && data.exists) {
        auto* pm = FTB::PluginManager::GetInstance();
        if (pm) {
            std::string mime;
            if (!ext.empty() && ext[0] == '.')
                mime = "text/" + ext.substr(1);
            std::string plugin_name = pm->FindPreviewer(mime, ext);
            if (!plugin_name.empty()) {
                std::string fp = (fs::path(currentPath) / data.selectedName).string();

                static std::string s_plugin_cache_key;
                static std::vector<std::string> s_plugin_cached_lines;
                std::string plugin_cache_key = plugin_name + ":" + fp;

                if (plugin_cache_key == s_plugin_cache_key) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    Elements plugin_lines;
                    for (const auto& l : s_plugin_cached_lines)
                        plugin_lines.push_back(FTB::AnsiStringToElement(l));
                    PushScrollableContent(info_elements, std::move(plugin_lines), scroll_y, preview_max_lines);
                    plugin_preview_handled = true;
                } else {
                    FTB::PluginContext pctx;
                    pctx.current_path = currentPath;
                    pctx.selected_file = data.selectedName;
                    pctx.selected_file_path = fp;
                    pctx.selected_is_dir = data.is_dir;
                    pctx.selected_size = data.file_size;
                    pctx.panel_width = preview_panel_width;

                    pm->ExecutePluginPreview(plugin_name, pctx, preview_panel_width);

                    FTB::PluginPreviewResult ppr;
                    if (pm->PollPluginPreview(plugin_name, ppr)) {
                        if (ppr.completed) {
                            if (!ppr.failed && !ppr.output.empty()) {
                                s_plugin_cached_lines = SplitLines(ppr.output);
                                s_plugin_cache_key = std::move(plugin_cache_key);

                                info_elements.push_back(separator() | color(TC("main_border")));
                                Elements plugin_lines;
                                for (const auto& l : s_plugin_cached_lines)
                                    plugin_lines.push_back(FTB::AnsiStringToElement(l));
                                PushScrollableContent(info_elements, std::move(plugin_lines), scroll_y, preview_max_lines);
                                plugin_preview_handled = true;
                            }
                        } else {
                            info_elements.push_back(separator() | color(TC("main_border")));
                            std::string loading_label = ppr.label.empty() ? plugin_name : ppr.label;
                            info_elements.push_back(
                                text("  " + loading_label + " preview (loading...)") | color(TC("dim")) | dim
                            );
                            plugin_preview_handled = true;
                        }
                    }
                }
            }
        }
    }

    if (plugin_preview_handled) {
        return vbox(info_elements) | bgcolor(TC("main_bg")) | flex | yframe;
    }
#endif

    if (data.is_dir && data.exists) {
        std::string dirPath = (fs::path(currentPath) / data.selectedName).string();
        cache.EnsureDirLoaded(dirPath);
        cache.SyncDirData(data);

        info_elements.push_back(separator() | color(TC("main_border")));
        if (data.dir_contents.empty()) {
            if (data.dir_sorted) {
                info_elements.push_back(text("  (empty)") | color(TC("dim")) | dim);
            } else {
                info_elements.push_back(text("  Loading...") | color(TC("dim")) | dim);
            }
        } else {
            int max_dir = cfg_preview.max_dir_entries;
            int total_dir = static_cast<int>(data.dir_contents.size());
            int limit = total_dir;
            if (max_dir > 0) limit = std::min(limit, max_dir);
            Elements dir_lines;
            for (int i = 0; i < limit; ++i) {
                const auto& entry = data.dir_contents[i];
                bool is_last = (i == limit - 1);
                std::string branch = is_last ? u8"\u2514\u2500\u2500 " : u8"\u251C\u2500\u2500 ";
                Element name_el;
                if (entry.is_dir) {
                    name_el = text(" " + entry.icon + entry.name) | bgcolor(TC("directory")) | color(TC("main_bg")) | bold;
                } else {
                    name_el = text(" " + entry.icon + entry.name) | color(GetEntryColor(entry));
                }
                dir_lines.push_back(
                    hbox({
                        text("  " + branch) | color(TC("dim")),
                        name_el
                    })
                );
            }
            PushScrollableContent(info_elements, std::move(dir_lines), scroll_y, preview_max_lines);
        }
    }

    std::string filePath = (fs::path(currentPath) / data.selectedName).string();
    PERF_LOG("DetailEl", "filePath=" + filePath + " cwd=" + currentPath
        + " selected=" + data.selectedName + " is_dir=" + std::to_string(data.is_dir));
    bool is_image = false;
#ifdef FTB_ENABLE_SSH
    bool ssh_mode = (FileManager::getSSHConnection() != nullptr);
    if (!ssh_mode) {
        is_image = !data.is_dir && FTB::ImagePreview::IsImageFile(filePath);
    }
#else
    is_image = !data.is_dir && FTB::ImagePreview::IsImageFile(filePath);
#endif
    bool use_proto_for_gif = is_image && is_media && MediaPreview::IsEnabled()
        && FTB::ImageOutputManager::ActiveProtocol();
    if (is_image && is_media && MediaPreview::IsEnabled() && !use_proto_for_gif) {
        is_image = false;
    }
    if (use_proto_for_gif) {
        is_media = false;
    }
    if (is_image) {
        int img_w = std::max(20, preview_panel_width - 4);
        int img_h = std::max(10, term_dim.dimy - 12);

        {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
        }

        auto* proto = FTB::ImageOutputManager::ActiveProtocol();

        // If user switched to a different file, clear previous image
        if (proto && FTB::ImageOutputManager::IsActive()) {
            std::string dummy;
            int dummy_rows;
            if (!FTB::ImageOutputManager::GetCached(filePath, dummy, dummy_rows)) {
                FTB::ImageOutputManager::ClearCurrent();
            }
        }

        bool use_protocol = false;
        std::string img_data;
        int img_rows = 0, render_w = 0, render_h = 0;

        if (proto) {
            bool have_cached = FTB::ImageOutputManager::GetCached(filePath, img_data, img_rows, &render_w, &render_h);
            bool has_failed = FTB::ImageOutputManager::HasFailed(filePath);

            if (!have_cached && !has_failed) {
                int pixel_w = img_w * 9;
                int pixel_h = std::max(5, term_dim.dimy - 5) * 18;
                if (!FTB::ImageOutputManager::IsEncoding(filePath)) {
                    FTB::ImageOutputManager::EncodeAsync(filePath, pixel_w, pixel_h);
                }
            }

            if (have_cached && img_rows > 0) {
                use_protocol = true;
            } else if (!has_failed) {
                use_protocol = true; // encoding still in progress
            }
            // else has_failed: fall through to ImagePreview
        }

        if (use_protocol) {
            info_elements.push_back(separator() | color(TC("main_border")));
            int panel_start_row = static_cast<int>(info_elements.size());
            int max_chars_h = std::max(5, term_dim.dimy - panel_start_row - 5);

            if (!img_data.empty() && img_rows > 0) {
                int preview_start_col = std::max(0, term_dim.dimx - preview_panel_width);
                FTB::ImageOutputManager::SetPending(
                    filePath, img_data, panel_start_row, preview_start_col,
                    img_rows, max_chars_h, preview_panel_width, render_w, render_h);

                {
                    auto gif_ext = std::filesystem::path(filePath).extension();
                    if (gif_ext == ".gif" || gif_ext == ".GIF") {
                        PERF_LOG("DetailEl", "trigger GIF anim path=" + filePath
                            + " row=" + std::to_string(panel_start_row)
                            + " isAnimating="
                            + std::to_string(FTB::ImageOutputManager::IsAnimating()));
                        FTB::ImageOutputManager::StartGifAnimation(
                            filePath, panel_start_row, preview_start_col);
                    }
                }

                std::string proto_note = "  [" +
                    FTB::ImageOutputManager::ProtocolName() + "] " +
                    std::to_string(img_rows) + " rows";
                info_elements.push_back(
                    text(proto_note) | color(TC("dim")) | dim);
            } else {
                info_elements.push_back(
                    text("  Encoding image...") | color(TC("dim")) | dim);
            }
        }

        if (!use_protocol) {
            FTB::ImagePreview::LoadAsync(filePath, img_w, img_h);

            FTB::ImageCache img_cache;
            bool cached = FTB::ImagePreview::GetCached(filePath, img_cache);
            if (cached && img_cache.is_image) {
                info_elements.push_back(separator() | color(TC("main_border")));
                for (const auto& line : img_cache.image_lines) {
                    Elements char_els;
                    if (!line.half_block_bottom.empty()) {
                        for (size_t x = 0; x < line.pixels.size(); ++x) {
                            auto& top = line.pixels[x];
                            auto& bot = line.half_block_bottom[x];
                            if (top.r == bot.r && top.g == bot.g && top.b == bot.b) {
                                char_els.push_back(
                                    text(L"\u2588") |
                                    color(Color::RGB(top.r, top.g, top.b))
                                );
                            } else {
                                char_els.push_back(
                                    text(L"\u2584") |
                                    color(Color::RGB(bot.r, bot.g, bot.b)) |
                                    bgcolor(Color::RGB(top.r, top.g, top.b))
                                );
                            }
                        }
                    } else {
                        for (const auto& p : line.pixels) {
                            char_els.push_back(
                                text(L"\u2588") |
                                color(Color::RGB(p.r, p.g, p.b))
                            );
                        }
                    }
                    info_elements.push_back(hbox(char_els));
                }
            } else if (cached && img_cache.failed) {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Failed to load image preview") | color(Color::Red));
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading image...") | color(TC("dim")) | dim);
            }
        }
    } else {
        // Not an image - clear any active image
        if (FTB::ImageOutputManager::IsActive()) {
            PERF_LOG("DetailEl", "non-image ClearCurrent path=" + filePath);
            FTB::ImageOutputManager::ClearCurrent();
        }
    }

    bool is_archive = !data.is_dir && data.is_regular && IsArchiveFile(data.selectedName);
    bool is_hex_binary = !data.is_dir && data.is_regular
        && HexPreview::IsBinaryFile(data.selectedName)
        && !is_image && !is_archive
        && !is_spreadsheet && !is_media && !is_audio && !is_pdf && !is_doc;

    if (is_archive) {
        std::string archivePath = (fs::path(currentPath) / data.selectedName).string();
        cache.SyncArchiveData(data);
        if (!data.archive_loaded) {
            cache.EnsureArchiveLoaded(archivePath);
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading archive...") | color(TC("dim")) | dim);
        } else if (!data.archive_contents.empty()) {
            info_elements.push_back(separator() | color(TC("main_border")));
            int max_archive_lines = cfg_preview.max_archive_nodes;

            std::vector<ArchiveEntry> sorted = data.archive_contents;
            {
                auto& cfg2 = FTB::ConfigManager::GetInstance()->GetConfig();
                auto mode = FTB::SortModeFromString(cfg2.style.sort_mode);
                std::sort(sorted.begin(), sorted.end(),
                    [mode](const ArchiveEntry& a, const ArchiveEntry& b) {
                        if (a.is_dir != b.is_dir) return a.is_dir;
                        (void)mode;
                        std::string na = a.name, nb = b.name;
                        std::transform(na.begin(), na.end(), na.begin(), ::tolower);
                        std::transform(nb.begin(), nb.end(), nb.begin(), ::tolower);
                        return na < nb;
                    });
            }

            struct TreeNode {
                std::string name;
                bool is_dir;
                uintmax_t size;
                std::vector<TreeNode> children;
            };

            std::vector<TreeNode> roots;
            for (const auto& entry : sorted) {
                std::string clean = entry.name;
                if (!clean.empty() && clean.back() == '/')
                    clean.pop_back();

                std::vector<std::string> parts;
                size_t start = 0, end;
                while ((end = clean.find('/', start)) != std::string::npos) {
                    parts.push_back(clean.substr(start, end - start));
                    start = end + 1;
                }
                parts.push_back(clean.substr(start));

                std::vector<TreeNode>* current = &roots;
                for (size_t pi = 0; pi < parts.size(); pi++) {
                    bool is_intermediate = (pi < parts.size() - 1);
                    auto it = std::find_if(current->begin(), current->end(),
                        [&](const TreeNode& n) { return n.name == parts[pi]; });
                    if (it != current->end()) {
                        if (is_intermediate) it->is_dir = true;
                        current = &it->children;
                    } else {
                        current->push_back({parts[pi], is_intermediate || entry.is_dir,
                                            (pi == parts.size() - 1) ? entry.size : 0, {}});
                        current = &current->back().children;
                    }
                }
            }

            Elements arch_lines;
            std::function<void(const std::vector<TreeNode>&, const std::string&)> flatten_tree;
            flatten_tree = [&](const std::vector<TreeNode>& nodes, const std::string& prefix) {
                for (size_t i = 0; i < nodes.size(); i++) {
                    const auto& node = nodes[i];
                    bool is_last = (i == nodes.size() - 1);
                    std::string branch = is_last ? u8"\u2514\u2500\u2500 " : u8"\u251C\u2500\u2500 ";
                    std::string child_prefix = prefix + (is_last ? "    " : u8"\u2502   ");

                    Element name_el;
                    if (node.is_dir) {
                        name_el = text(" " + node.name + "/") | color(TC("directory")) | bold;
                    } else {
                        name_el = text(" " + node.name) | color(TC("file"));
                    }

                    Elements row = {
                        text("  " + prefix + branch) | color(TC("dim")),
                        name_el
                    };

                    if (!node.is_dir && node.size > 0) {
                        row.push_back(filler());
                        row.push_back(text(" " + FormatFileSize(node.size)) | color(TC("syn_number")));
                    }

                    arch_lines.push_back(hbox(std::move(row)));

                    if (!node.children.empty())
                        flatten_tree(node.children, child_prefix);
                }
            };

            flatten_tree(roots, "");

            if (max_archive_lines > 0 && static_cast<int>(arch_lines.size()) > max_archive_lines) {
                arch_lines.resize(max_archive_lines);
            }

            PushScrollableContent(info_elements, std::move(arch_lines), scroll_y, preview_max_lines);
        }
    }

    
    if (is_spreadsheet) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool xleak_used = false;

        if (!SpreadsheetPreview::ShowSource()) {
            SpreadsheetPreview::LoadAsync(fp, preview_panel_width);
            SpreadsheetCache sc;
            if (SpreadsheetPreview::GetCached(fp, sc)) {
                if (!sc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    PushAnsiScrollable(info_elements, sc.output, scroll_y, preview_max_lines);
                    xleak_used = true;
                }
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading spreadsheet preview...") | color(TC("dim")) | dim);
                xleak_used = true;
            }
        }

        if (!xleak_used) {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Spreadsheet (use Ctrl+B xlsx to toggle preview)") | color(TC("dim")) | dim);
        }
    }

    if (is_media && MediaPreview::IsEnabled()) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        MediaPreview::LoadAsync(fp, preview_panel_width);
        MediaCache mc;
        if (MediaPreview::GetCached(fp, mc)) {
            if (!mc.output.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                PushAnsiScrollable(info_elements, mc.output, scroll_y, preview_max_lines);
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Media file (use Ctrl+B timg to play)") | color(TC("dim")) | dim);
            }
        } else {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading media preview...") | color(TC("dim")) | dim);
        }
    }

    if (is_audio && AudioPreview::IsEnabled()) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        AudioPreview::LoadAsync(fp, preview_panel_width);
        AudioCache ac;
        if (AudioPreview::GetCached(fp, ac)) {
            if (!ac.output.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                PushAnsiScrollable(info_elements, ac.output, scroll_y, preview_max_lines);
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Audio file (use Ctrl+B aud to toggle)") | color(TC("dim")) | dim);
            }
        } else {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading audio preview...") | color(TC("dim")) | dim);
        }
    }

    if (is_doc) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool pandoc_used = false;

        if (!DocPreview::ShowSource()) {
            DocPreview::LoadAsync(fp, preview_panel_width);
            DocCache dc;
            if (DocPreview::GetCached(fp, dc)) {
                if (!dc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    PushAnsiScrollable(info_elements, dc.output, scroll_y, preview_max_lines);
                    pandoc_used = true;
                }
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading document preview...") | color(TC("dim")) | dim);
                pandoc_used = true;
            }
        }

        if (!pandoc_used) {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  DOC/DOCX (use Ctrl+B doc to toggle preview)") | color(TC("dim")) | dim);
        }
    }

    if (is_pdf) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool hygg_used = false;

        if (!PdfPreview::ShowSource()) {
            PdfPreview::LoadAsync(fp, preview_panel_width);
            PdfCache pc;
            if (PdfPreview::GetCached(fp, pc)) {
                if (!pc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    PushAnsiScrollable(info_elements, pc.output, scroll_y, preview_max_lines);
                    hygg_used = true;
                }
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading PDF preview...") | color(TC("dim")) | dim);
                hygg_used = true;
            }
        }

        if (!hygg_used) {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  PDF (use Ctrl+B pdf to toggle preview)") | color(TC("dim")) | dim);
        }
    }

    if (is_hex_binary) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        if (HexPreview::IsEnabled()) {
            HexPreview::LoadAsync(fp, data.file_size, preview_panel_width);
            HexCache hc;
            if (HexPreview::GetCached(fp, hc)) {
                if (!hc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    auto hex_lines = SplitLines(hc.output);
                    Elements hex_elements;
                    for (const auto& l : hex_lines)
                        hex_elements.push_back(text("  " + l) | color(TC("main_fg")));
                    PushScrollableContent(info_elements, std::move(hex_elements), scroll_y, preview_max_lines);
                }
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading hex preview...") | color(TC("dim")) | dim);
            }
        } else {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Hex preview disabled (use Ctrl+B hex to enable)") | color(TC("dim")) | dim);
        }
    }

    bool text_preview_eligible = cfg_preview.max_text_file_size_kb == 0
        || data.file_size <= static_cast<uintmax_t>(cfg_preview.max_text_file_size_kb) * 1024;

    if (!data.is_dir && data.is_regular && text_preview_eligible && !is_image && !is_archive && !is_spreadsheet && !is_media && !is_audio && !is_pdf && !is_doc && !is_hex_binary) {
        std::string fp = (fs::path(currentPath) / data.selectedName).string();

        bool is_markdown = MarkdownPreview::IsMarkdownFile(data.selectedName);

        bool glow_used = false;

        if (is_markdown && !MarkdownPreview::ShowSource()) {
            MarkdownPreview::LoadAsync(fp, preview_panel_width);
            MarkdownCache mc;
            if (MarkdownPreview::GetCached(fp, mc)) {
                if (!mc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    PushAnsiScrollable(info_elements, mc.output, scroll_y, preview_max_lines);
                    glow_used = true;
                }
            } else {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(text("  Loading markdown preview...") | color(TC("dim")) | dim);
                glow_used = true;
            }
        }

        if (!glow_used) {
            cache.EnsureTextLoaded(fp, data.file_size);
            cache.SyncTextData(data);

            if (!data.text_preview.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                int max_lines = std::max(5, term_dim.dimy - 10);
                int line_num_width = std::max(1, static_cast<int>(std::to_string(max_lines + scroll_y).size()));

                g_preview_line_num_width = line_num_width;
                g_preview_max_lines = max_lines;

                auto lang = FTB::Editor::SyntaxHighlighter::DetectLanguage(data.selectedName);
                cache.Highlighter().SetLanguage(lang);
                cache.Highlighter().ResetMultiLineState();

                g_preview_sel.lines.clear();
                g_preview_sel.scroll_y = scroll_y;

                // Count total lines for scroll clamping
                {
                    std::istringstream count_ss(data.text_preview);
                    std::string tmp;
                    int total = 0;
                    while (std::getline(count_ss, tmp)) total++;
                    int max_scroll = std::max(0, total - max_lines);
                    if (scroll_y > max_scroll) scroll_y = max_scroll;
                }

                if (scroll_y > 0) {
                    info_elements.push_back(
                        text("  ... " + std::to_string(scroll_y) + " lines above (Alt+K to scroll up)") | color(TC("dim")) | dim
                    );
                }

                Elements text_lines;
                std::istringstream iss(data.text_preview);
                std::string line;
                int line_num = 1;
                int displayed = 0;
                int sel_y1 = g_preview_sel.active ? std::min(g_preview_sel.anchor_y, g_preview_sel.current_y) : -1;
                int sel_y2 = g_preview_sel.active ? std::max(g_preview_sel.anchor_y, g_preview_sel.current_y) : -1;

                while (std::getline(iss, line)) {
                    line = SanitizePreviewLine(line);
                    if (line_num <= scroll_y) {
                        g_preview_sel.lines.push_back(line);
                        line_num++;
                        continue;
                    }
                    if (displayed >= max_lines) break;

                    // Apply horizontal scroll (scroll_x is display columns, convert to byte offset)
                    if (scroll_x > 0 && UnicodeUtil::CalculateDisplayWidth(line) > scroll_x) {
                        size_t byte_off = UnicodeUtil::ByteOffsetFromDisplayColumn(line, scroll_x);
                        line = line.substr(byte_off);
                    } else if (scroll_x > 0) {
                        line = "";
                    }

                    if (cfg_preview.truncate_long_lines && UnicodeUtil::CalculateDisplayWidth(line) > usable_line_width)
                        line = UnicodeUtil::Utf8Truncate(line, usable_line_width - 3) + "...";

                    g_preview_sel.lines.push_back(line);

                    int csel_x1 = -1, csel_x2 = -1;
                    if (g_preview_sel.active) {
                        int line_y = g_preview_box.y_min + displayed;
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
                    displayed++;
                }

                info_elements.push_back(
                    vbox(std::move(text_lines)) | reflect(g_preview_box)
                );

                // Check if there are more lines below
                {
                    std::istringstream check_ss(data.text_preview);
                    std::string tmp;
                    int total = 0;
                    while (std::getline(check_ss, tmp)) total++;
                    if (line_num <= total) {
                        info_elements.push_back(
                            text("  ... " + std::to_string(total - line_num + 1) + " more lines (Alt+J to scroll down)") | color(TC("dim")) | dim
                        );
                    }
                    // Lazy load more content when scrolling near the bottom
                    if (scroll_y + max_lines + cfg_preview.virtual_scroll_margin > data.text_total_lines && data.text_total_lines > 0) {
                        cache.LoadMoreTextLines(fp, data.text_total_lines + 1, cfg_preview.chunk_size_lines);
                    }
                }
            }
        }
    }

    return vbox(info_elements) | bgcolor(TC("main_bg")) | flex | yframe;
}

} // namespace FTB
