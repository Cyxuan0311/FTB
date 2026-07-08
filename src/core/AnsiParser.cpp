#include "core/AnsiParser.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace FTB {

ftxui::Element AnsiStringToElement(const std::string& ansi_text) {
    if (ansi_text.empty()) {

        return ftxui::text("");
    }

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

}
