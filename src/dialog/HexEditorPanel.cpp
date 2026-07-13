#include "dialog/HexEditorPanel.hpp"
#include "core/MainUI.hpp"
#include "config/ThemeManager.hpp"
#include "browser/BinaryFileHandler.hpp"
#include "utils/StatusMessage.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace FTB {
namespace UI {

using namespace ftxui;

static Color TC(const std::string& name) {
    return ThemeManager::GetInstance()->GetThemeColor(name);
}

// ─── Hex buffer cache ────────────────────────────────────────────────

static std::mutex s_hex_cache_mutex;
static std::string s_hex_cache_key;
static std::vector<uint8_t> s_hex_cache_data;
static uintmax_t s_hex_cache_file_size = 0;

static void LoadHexFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
    if (s_hex_cache_key == filePath && !s_hex_cache_data.empty()) return;

    s_hex_cache_key = filePath;
    s_hex_cache_data.clear();
    s_hex_cache_file_size = 0;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return;

    auto size = file.tellg();
    if (size <= 0) return;
    s_hex_cache_file_size = static_cast<uintmax_t>(size);

    uintmax_t max_bytes = std::min(s_hex_cache_file_size, static_cast<uintmax_t>(64 * 1024 * 1024));
    file.seekg(0, std::ios::beg);

    s_hex_cache_data.resize(static_cast<size_t>(max_bytes));
    file.read(reinterpret_cast<char*>(s_hex_cache_data.data()), static_cast<std::streamsize>(max_bytes));
}

static bool IsHexFileCached(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
    return s_hex_cache_key == filePath && !s_hex_cache_data.empty();
}

static constexpr int kBytesPerLine = 16;

// ─── Save ────────────────────────────────────────────────────────────

static bool SaveHexFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
    if (s_hex_cache_key != filePath) return false;
    if (s_hex_cache_data.empty()) return false;

    // Truncate to original file size if we only read part of it
    size_t write_size = s_hex_cache_data.size();
    if (s_hex_cache_file_size < write_size)
        write_size = static_cast<size_t>(s_hex_cache_file_size);

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) return false;

    file.write(reinterpret_cast<const char*>(s_hex_cache_data.data()),
               static_cast<std::streamsize>(write_size));
    return file.good();
}

// ─── Scroll helper ───────────────────────────────────────────────────

static void EnsureCursorVisible(int cursor_byte, int& scroll_y, int visible_rows) {
    int cursor_row = cursor_byte / kBytesPerLine;
    if (cursor_row < scroll_y)
        scroll_y = cursor_row;
    else if (cursor_row >= scroll_y + visible_rows)
        scroll_y = cursor_row - visible_rows + 1;
}

// ─── Hex digit helpers ───────────────────────────────────────────────

static int HexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static bool IsHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// ─── Rendering ────────────────────────────────────────────────────────

Element RenderHexEditorPanel(MainState& state, int term_width, int term_height) {
    auto& tab = state.tabManager.active();
    if (tab.viewer_filepath.empty()) return text("No file");

    std::string filePath = tab.viewer_filepath;
    std::string filename = std::filesystem::path(filePath).filename().string();

    if (!IsHexFileCached(filePath)) {
        LoadHexFile(filePath);
    }

    int panel_h = term_height;
    int status_h = 1;
    int header_h = 1;
    int content_h = panel_h - header_h - status_h;
    if (content_h < 3) content_h = 3;
    int visible_rows = content_h - 1;
    if (visible_rows < 1) visible_rows = 1;

    auto& cursor_byte = state.hex_cursor_byte;
    auto& input_nibble = state.hex_input_nibble;
    auto& modified = state.hex_modified;
    auto& scroll_y = state.viewer_scroll_y;

    Elements lines;

    // ── Header ──
    std::string header_text = " " + filename + " (hex)";
    if (modified) header_text += " [*]";
    Element header = hbox({
        text(header_text) | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        filler() | bgcolor(TC("status_bg")),
        text(" [Esc] Close  [Arrows] Navigate  [0-9a-f] Edit  [Ctrl+S] Save ")
            | color(Color::Grey37) | bgcolor(TC("status_bg")),
    }) | size(WIDTH, EQUAL, term_width);
    lines.push_back(header);

    // ── Column header ──
    {
        Elements header_elements;

        // Offset header: 12 chars, matching data "  XXXXXXXX  "
        header_elements.push_back(text("  Offset    ") | color(TC("syn_comment")) | bold);

        // Hex byte index headers (00-0F) with same separators as data rows
        char idx_buf[4];
        for (int b = 0; b < kBytesPerLine; ++b) {
            std::snprintf(idx_buf, sizeof(idx_buf), "%02X", b);
            header_elements.push_back(text(idx_buf) | color(TC("syn_comment")) | bold);
            if (b < kBytesPerLine - 1) {
                header_elements.push_back(text(" ") | color(TC("syn_comment")) | bold);
            }
        }

        // Gap before ASCII label (matching data "  ")
        header_elements.push_back(text("  ") | color(TC("syn_comment")) | bold);

        // ASCII label
        header_elements.push_back(text("ASCII") | color(TC("syn_comment")) | bold);

        lines.push_back(hbox(std::move(header_elements)));
    }

    // ── Data rows ──
    {
        std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
        if (s_hex_cache_data.empty() && s_hex_cache_key == filePath) {
            lines.push_back(text("  (empty file or could not read)") | color(TC("dim")) | dim);
        } else if (s_hex_cache_key != filePath) {
            lines.push_back(text("  Loading...") | color(TC("dim")) | dim);
        } else {
            size_t data_size = s_hex_cache_data.size();
            int total_rows = static_cast<int>((data_size + kBytesPerLine - 1) / kBytesPerLine);

            // Ensure cursor is within bounds
            if (cursor_byte < 0) cursor_byte = 0;
            if (cursor_byte >= static_cast<int>(data_size) && data_size > 0)
                cursor_byte = static_cast<int>(data_size) - 1;

            // Ensure cursor is visible
            EnsureCursorVisible(cursor_byte, scroll_y, visible_rows);
            scroll_y = std::max(0, std::min(scroll_y, std::max(0, total_rows - visible_rows)));

            int start_row = scroll_y;
            int end_row = std::min(start_row + visible_rows, total_rows);

            for (int row = start_row; row < end_row; ++row) {
                size_t offset = static_cast<size_t>(row) * kBytesPerLine;
                size_t bytes_remaining = (offset < data_size) ? data_size - offset : 0;
                int bytes_in_line = std::min(kBytesPerLine, static_cast<int>(bytes_remaining));

                Elements row_elements;

                // ── Offset ──
                char offset_buf[32];
                std::snprintf(offset_buf, sizeof(offset_buf), "%08zX", offset);
                row_elements.push_back(text("  " + std::string(offset_buf) + "  "));

                // ── Hex bytes ──
                for (int b = 0; b < kBytesPerLine; ++b) {
                    if (b < bytes_in_line) {
                        std::snprintf(offset_buf, sizeof(offset_buf), "%02X", s_hex_cache_data[offset + b]);
                        std::string byte_str = offset_buf;
                        bool is_cursor = (static_cast<int>(offset + b) == cursor_byte);
                        if (is_cursor) {
                            row_elements.push_back(text(byte_str) | inverted);
                        } else {
                            bool is_offset_line = ((offset / kBytesPerLine) % 16 == 0);
                            row_elements.push_back(text(byte_str) | color(is_offset_line ? TC("main_fg") : Color::Default));
                        }
                    } else {
                        row_elements.push_back(text("  "));
                    }
                    if (b == 7) {
                        row_elements.push_back(text(" "));
                    } else if (b < kBytesPerLine - 1) {
                        row_elements.push_back(text(" "));
                    }
                }

                row_elements.push_back(text("  "));

                // ── ASCII ──
                for (int b = 0; b < bytes_in_line; ++b) {
                    uint8_t byte = s_hex_cache_data[offset + b];
                    std::string ascii_char(1, (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.');
                    bool is_cursor = (static_cast<int>(offset + b) == cursor_byte);
                    if (is_cursor) {
                        row_elements.push_back(text(ascii_char) | inverted);
                    } else {
                        row_elements.push_back(text(ascii_char));
                    }
                }

                lines.push_back(hbox(std::move(row_elements)));
            }

            // Fill remaining space
            for (int i = end_row; i < start_row + visible_rows; ++i) {
                lines.push_back(text(""));
            }
        }
    }

    // ── Status bar ──
    std::string status_text;
    {
        std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
        if (s_hex_cache_key == filePath) {
            int total_rows = static_cast<int>((s_hex_cache_data.size() + kBytesPerLine - 1) / kBytesPerLine);
            int max_scroll = std::max(0, total_rows - visible_rows);

            std::string size_str;
            if (s_hex_cache_file_size < 1024)
                size_str = std::to_string(s_hex_cache_file_size) + " B";
            else if (s_hex_cache_file_size < 1024 * 1024)
                size_str = std::to_string(s_hex_cache_file_size / 1024) + " KB";
            else
                size_str = std::to_string(s_hex_cache_file_size / (1024 * 1024)) + " MB";

            status_text = "  " + filename + "  " + size_str + "  "
                + "Byte 0x" + std::to_string(cursor_byte)
                + " (row " + std::to_string(scroll_y) + "/" + std::to_string(max_scroll) + ")"
                + "  Total " + std::to_string(total_rows) + " rows";
            if (input_nibble == 1)
                status_text += "  [waiting for low nibble...]";
        }
    }

    Element status = hbox({
        text(status_text) | color(TC("status_fg")) | bgcolor(TC("status_bg")),
        filler() | bgcolor(TC("status_bg")),
    }) | size(WIDTH, EQUAL, term_width);

    lines.push_back(status);

    return vbox(std::move(lines)) | bgcolor(TC("main_bg")) | flex;
}

// ─── Event Handling ──────────────────────────────────────────────────

bool HandleHexEditorEvent(MainState& state, ftxui::Event& event) {
    auto& tab = state.tabManager.active();
    if (tab.type != TabType::HexEditor) return false;

    auto& cursor_byte = state.hex_cursor_byte;
    auto& input_nibble = state.hex_input_nibble;
    auto& modified = state.hex_modified;
    auto& scroll_y = state.viewer_scroll_y;

    // Get total data size
    int data_size = 0;
    {
        std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
        if (s_hex_cache_key == tab.viewer_filepath)
            data_size = static_cast<int>(s_hex_cache_data.size());
    }

    if (data_size <= 0) {
        // No data loaded yet, but allow close
        if (event == Event::Escape) {
            state.saveTabState();
            int idx = state.tabManager.activeIndex();
            if (state.tabManager.isHexTab(idx) && state.tabManager.canClose()) {
                state.tabManager.closeTab(idx);
                state.tabManager.loadTabState(state, state.tabManager.activeIndex());
                return true;
            }
            if (state.tabManager.isHexTab(idx) && !state.tabManager.canClose()) {
                const char* home = std::getenv("HOME");
                state.tabManager.createTab(home ? home : "/");
                state.tabManager.closeTab(idx);
                state.tabManager.loadTabState(state, state.tabManager.activeIndex());
                return true;
            }
            return false;
        }
        return false;
    }

    // Clamp cursor
    cursor_byte = std::clamp(cursor_byte, 0, data_size - 1);

    // ── Escape: close ──
    if (event == Event::Escape) {
        state.saveTabState();
        int idx = state.tabManager.activeIndex();
        if (state.tabManager.isHexTab(idx) && state.tabManager.canClose()) {
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        if (state.tabManager.isHexTab(idx) && !state.tabManager.canClose()) {
            const char* home = std::getenv("HOME");
            state.tabManager.createTab(home ? home : "/");
            state.tabManager.closeTab(idx);
            state.tabManager.loadTabState(state, state.tabManager.activeIndex());
            return true;
        }
        return false;
    }

    // ── Ctrl+S: Save ──
    if (event == Event::CtrlS) {
        if (modified && SaveHexFile(tab.viewer_filepath)) {
            modified = false;
            StatusMessage::Show("Saved");
        } else if (!modified) {
            StatusMessage::Show("No changes to save");
        } else {
            StatusMessage::Show("Save failed");
        }
        return true;
    }

    // ── Navigation ──
    if (event == Event::ArrowLeft) {
        if (input_nibble == 1) {
            input_nibble = 0;
        } else {
            cursor_byte = std::max(0, cursor_byte - 1);
        }
        return true;
    }

    if (event == Event::ArrowRight) {
        if (input_nibble == 0) {
            input_nibble = 1;
        } else {
            if (cursor_byte < data_size - 1) {
                cursor_byte++;
                input_nibble = 0;
            }
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        cursor_byte = std::max(0, cursor_byte - kBytesPerLine);
        input_nibble = 0;
        return true;
    }

    if (event == Event::ArrowDown) {
        cursor_byte = std::min(data_size - 1, cursor_byte + kBytesPerLine);
        input_nibble = 0;
        return true;
    }

    if (event == Event::Home) {
        cursor_byte = 0;
        input_nibble = 0;
        scroll_y = 0;
        return true;
    }

    if (event == Event::End) {
        cursor_byte = data_size - 1;
        input_nibble = 0;
        return true;
    }

    if (event == Event::PageUp || event == Event::Character('b')) {
        int visible_rows = 20;
        cursor_byte = std::max(0, cursor_byte - visible_rows * kBytesPerLine);
        input_nibble = 0;
        return true;
    }

    if (event == Event::PageDown || event == Event::Character('f')) {
        int visible_rows = 20;
        cursor_byte = std::min(data_size - 1, cursor_byte + visible_rows * kBytesPerLine);
        input_nibble = 0;
        return true;
    }

    // ── Hex digit input ──
    if (event.is_character()) {
        const std::string& ch = event.character();
        if (!ch.empty() && IsHexDigit(ch[0])) {
            int digit_val = HexDigitValue(ch[0]);
            if (digit_val >= 0 && digit_val <= 15) {
                {
                    std::lock_guard<std::mutex> lock(s_hex_cache_mutex);
                    if (s_hex_cache_key == tab.viewer_filepath &&
                        cursor_byte < static_cast<int>(s_hex_cache_data.size())) {
                        if (input_nibble == 0) {
                            // High nibble
                            s_hex_cache_data[cursor_byte] =
                                (s_hex_cache_data[cursor_byte] & 0x0F) | (static_cast<uint8_t>(digit_val) << 4);
                            input_nibble = 1;
                        } else {
                            // Low nibble
                            s_hex_cache_data[cursor_byte] =
                                (s_hex_cache_data[cursor_byte] & 0xF0) | static_cast<uint8_t>(digit_val);
                            input_nibble = 0;
                            // Advance to next byte
                            if (cursor_byte < data_size - 1)
                                cursor_byte++;
                        }
                        modified = true;
                    }
                }
            }
            return true;
        }
    }

    // ── Mouse ──
    if (event.is_mouse()) {
        auto& me = event.mouse();
        if (me.button == Mouse::WheelUp) {
            scroll_y = std::max(0, scroll_y - 3);
            return true;
        }
        if (me.button == Mouse::WheelDown) {
            int total_rows = (data_size + kBytesPerLine - 1) / kBytesPerLine;
            int visible_rows = 20;
            scroll_y = std::min(std::max(0, total_rows - visible_rows), scroll_y + 3);
            return true;
        }
    }

    return false;
}

}
}
