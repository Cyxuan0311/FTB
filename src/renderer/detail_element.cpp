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

#include "core/AnsiParser.hpp"
#include "preview/ArchivePreview.hpp"
#include "preview/ImagePreview.hpp"
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
#include "utils/PerfLogger.hpp"

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

Element CreateDetailElement(const std::vector<DirEntryInfo>& entries,
                            int selected,
                            const std::string& currentPath,
                            int scroll_y,
                            int scroll_x) {
    PERF_SCOPE("preview");
    auto& cache = PreviewCache::Instance();

    cache.Update(entries, selected, currentPath);

    PreviewData data = cache.Copy();

    Elements info_elements;

    auto term_dim = Terminal::Size();
    auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
    auto& cfg_preview = FTB::ConfigManager::GetInstance()->GetConfig().preview;
    int preview_panel_width = std::max(20, static_cast<int>(term_dim.dimx * cfg.layout.preview_ratio));
    int usable_line_width = std::max(20, preview_panel_width - 6);

    std::string ext;
    auto dot = data.selectedName.find_last_of('.');
    if (dot != std::string::npos) {
        ext = data.selectedName.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    bool is_md_file_preview = MarkdownPreview::IsMarkdownFile(data.selectedName);
    bool is_spreadsheet = SpreadsheetPreview::IsSpreadsheetFile(data.selectedName);
    bool is_media = MediaPreview::IsMediaFile(data.selectedName);
    bool is_audio = AudioPreview::IsAudioFile(data.selectedName);
    bool is_pdf = PdfPreview::IsPdfFile(data.selectedName);
    bool is_doc = DocPreview::IsDocFile(data.selectedName);

    std::string preview_label = " Preview";
    if (HexPreview::IsBinaryFile(data.selectedName) && HexPreview::IsEnabled()
        && !is_spreadsheet && !is_media && !is_audio && !is_pdf && !is_doc)
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

    if (data.is_dir && data.exists) {
        PERF_LOG("preview", "Directory preview: " + data.selectedName);
        std::string dirPath = (fs::path(currentPath) / data.selectedName).string();
        cache.EnsureDirLoaded(dirPath);
        cache.SyncDirData(data);

        info_elements.push_back(separator() | color(TC("main_border")));
        if (data.dir_contents.empty()) {
            info_elements.push_back(text("  Loading...") | color(TC("dim")) | dim);
        } else {
            int max_dir = cfg_preview.max_dir_entries;
            int show_count = (max_dir > 0)
                ? std::min(static_cast<int>(data.dir_contents.size()), max_dir)
                : static_cast<int>(data.dir_contents.size());
            // Cap to panel height for large directories
            int panel_usable = std::max(5, term_dim.dimy - 20);
            show_count = std::min(show_count, panel_usable);
            for (int i = 0; i < show_count; ++i) {
                const auto& entry = data.dir_contents[i];
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
            if (static_cast<int>(data.dir_contents.size()) > show_count) {
                info_elements.push_back(
                    text("    \u2514\u2500\u2500 +" + std::to_string(data.dir_contents.size() - show_count) + " more") | color(TC("dim")) | dim
                );
            }
        }
    }

    std::string filePath = (fs::path(currentPath) / data.selectedName).string();
    bool is_image = false;
#ifdef FTB_ENABLE_SSH
    bool ssh_mode = (FileManager::getSSHConnection() != nullptr);
    if (!ssh_mode) {
        is_image = !data.is_dir && FTB::ImagePreview::IsImageFile(filePath);
    }
#else
    is_image = !data.is_dir && FTB::ImagePreview::IsImageFile(filePath);
#endif
    PERF_LOG("preview", "Image preview: " + data.selectedName);
    if (is_image && is_media && MediaPreview::IsEnabled()) {
        is_image = false;
    }

    if (is_image) {
        PERF_LOG("preview", "Image preview: " + data.selectedName);
        int img_w = std::max(20, preview_panel_width - 4);
        int img_h = std::max(10, term_dim.dimy - 12);

        FTB::ImagePreview::LoadAsync(filePath, img_w, img_h);

        FTB::ImageCache img_cache;
        bool cached = FTB::ImagePreview::GetCached(filePath, img_cache);
        if (cached && img_cache.is_image) {
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
        } else if (cached && img_cache.failed) {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Failed to load image preview") | color(Color::Red));
        } else {
            info_elements.push_back(separator() | color(TC("main_border")));
            info_elements.push_back(text("  Loading image...") | color(TC("dim")) | dim);
        }
    }

    bool is_archive = !data.is_dir && data.is_regular && IsArchiveFile(data.selectedName);
    bool is_hex_binary = !data.is_dir && data.is_regular
        && HexPreview::IsBinaryFile(data.selectedName)
        && !is_image && !is_archive
        && !is_spreadsheet && !is_media && !is_audio && !is_pdf && !is_doc;

    if (is_archive) {
        PERF_LOG("preview", "Archive preview: " + data.selectedName);
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

            int effective_max = (max_archive_lines > 0) ? max_archive_lines
                : std::max(200, term_dim.dimy * 5);
            int remaining = effective_max;
            std::function<void(const std::vector<TreeNode>&, const std::string&)> render_tree;
            render_tree = [&](const std::vector<TreeNode>& nodes, const std::string& prefix) {
                for (size_t i = 0; i < nodes.size() && remaining > 0; i++) {
                    bool is_last = (i == nodes.size() - 1);
                    const auto& node = nodes[i];

                    std::string branch = is_last ? u8"\u2514\u2500\u2500 " : u8"\u251C\u2500\u2500 ";
                    std::string child_prefix = prefix + (is_last ? "    " : u8"\u2502   ");

                    Element name_el;
                    if (node.is_dir) {
                        name_el = text(" " + node.name + "/") | color(TC("directory")) | bold;
                    } else {
                        name_el = text(" " + node.name) | color(TC("main_fg"));
                    }

                    Elements row = {
                        text("  " + prefix + branch) | color(TC("dim")),
                        name_el
                    };

                    if (!node.is_dir && node.size > 0) {
                        row.push_back(filler());
                        row.push_back(text(" " + FormatFileSize(node.size)) | color(TC("syn_number")));
                    }

                    info_elements.push_back(hbox(std::move(row)));
                    remaining--;

                    if (!node.children.empty())
                        render_tree(node.children, child_prefix);
                }
            };

            render_tree(roots, "");

            if (max_archive_lines > 0 && static_cast<int>(data.archive_contents.size()) > max_archive_lines) {
                info_elements.push_back(
                    text("    \u2514\u2500\u2500 +" +
                         std::to_string(data.archive_contents.size() - max_archive_lines) +
                         " more entries") | color(TC("dim")) | dim
                );
            }
        }
    }

    if (is_spreadsheet) {
        PERF_LOG("preview", "Spreadsheet preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool xleak_used = false;

        if (!SpreadsheetPreview::ShowSource()) {
            SpreadsheetPreview::LoadAsync(fp, preview_panel_width);
            SpreadsheetCache sc;
            if (SpreadsheetPreview::GetCached(fp, sc)) {
                if (!sc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    info_elements.push_back(FTB::AnsiStringToElement(sc.output) | flex);
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
        PERF_LOG("preview", "Media preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        MediaPreview::LoadAsync(fp, preview_panel_width);
        MediaCache mc;
        if (MediaPreview::GetCached(fp, mc)) {
            if (!mc.output.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(FTB::AnsiStringToElement(mc.output) | flex);
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
        PERF_LOG("preview", "Audio preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        AudioPreview::LoadAsync(fp, preview_panel_width);
        AudioCache ac;
        if (AudioPreview::GetCached(fp, ac)) {
            if (!ac.output.empty()) {
                info_elements.push_back(separator() | color(TC("main_border")));
                info_elements.push_back(FTB::AnsiStringToElement(ac.output) | flex);
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
        PERF_LOG("preview", "Doc preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool pandoc_used = false;

        if (!DocPreview::ShowSource()) {
            DocPreview::LoadAsync(fp, preview_panel_width);
            DocCache dc;
            if (DocPreview::GetCached(fp, dc)) {
                if (!dc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    info_elements.push_back(FTB::AnsiStringToElement(dc.output) | flex);
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
        PERF_LOG("preview", "PDF preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        bool hygg_used = false;

        if (!PdfPreview::ShowSource()) {
            PdfPreview::LoadAsync(fp, preview_panel_width);
            PdfCache pc;
            if (PdfPreview::GetCached(fp, pc)) {
                if (!pc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    info_elements.push_back(FTB::AnsiStringToElement(pc.output) | flex);
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
        PERF_LOG("preview", "Hex preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();
        if (HexPreview::IsEnabled()) {
            HexPreview::LoadAsync(fp, data.file_size, preview_panel_width);
            HexCache hc;
            if (HexPreview::GetCached(fp, hc)) {
                if (!hc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    std::istringstream iss(hc.output);
                    std::string line;
                    while (std::getline(iss, line)) {
                        info_elements.push_back(text("  " + line) | color(TC("main_fg")));
                    }
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
        PERF_LOG("preview", "Text preview: " + data.selectedName);
        std::string fp = (fs::path(currentPath) / data.selectedName).string();

        bool is_markdown = MarkdownPreview::IsMarkdownFile(data.selectedName);

        bool glow_used = false;

        if (is_markdown && !MarkdownPreview::ShowSource()) {
            MarkdownPreview::LoadAsync(fp, preview_panel_width);
            MarkdownCache mc;
            if (MarkdownPreview::GetCached(fp, mc)) {
                if (!mc.output.empty()) {
                    info_elements.push_back(separator() | color(TC("main_border")));
                    info_elements.push_back(FTB::AnsiStringToElement(mc.output) | flex);
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

    info_elements.push_back(separator() | color(TC("main_border")));

    return vbox(info_elements) | bgcolor(TC("main_bg")) | flex | yframe;
}

} // namespace FTB
