#include "dialog/AIDialog.hpp"
#include "ai/AIAgent.hpp"
#include "ai/SessionManager.hpp"
#include "config/ConfigManager.hpp"
#include "renderer/detail_element.hpp"

#include <sstream>
#include <algorithm>

namespace FTB {

AIPanelState::AIPanelState() = default;
AIPanelState::~AIPanelState() = default;
AIPanelState::AIPanelState(AIPanelState&&) noexcept = default;
AIPanelState& AIPanelState::operator=(AIPanelState&&) noexcept = default;

} // namespace FTB

namespace FTB::UI {
using namespace ftxui;

static std::vector<std::string> wrapText(const std::string& str, int max_width) {
    if (max_width <= 0) return {str};
    std::vector<std::string> result;
    std::string remaining = str;
    while (!remaining.empty()) {
        if (static_cast<int>(remaining.size()) <= max_width) {
            result.push_back(remaining);
            break;
        }
        int break_at = max_width;
        while (break_at > 0 && remaining[break_at - 1] != ' ') break_at--;
        if (break_at == 0) break_at = max_width;
        result.push_back(remaining.substr(0, break_at));
        remaining = remaining.substr(break_at);
        while (!remaining.empty() && remaining[0] == ' ') remaining.erase(0, 1);
    }
    if (result.empty()) result.push_back("");
    return result;
}



// Character-level run-length highlighted text line
static Element makeTextLine(const std::string& str, ftxui::Color col,
                            int abs_line, const AIPanelState& ai,
                            bool dim_flag = false) {
    auto withDim = [&](Element el) {
        return dim_flag ? el | dim : el;
    };

    Element base = text(str) | color(col);

    if (ai.sel_dragging) {
        int sel_start_line = std::min(ai.sel_anchor_line, ai.sel_current_line);
        int sel_end_line = std::max(ai.sel_anchor_line, ai.sel_current_line);

        if (abs_line >= sel_start_line && abs_line <= sel_end_line) {
            int col_start = std::min(ai.sel_anchor_col, ai.sel_current_col);
            int col_end = std::max(ai.sel_anchor_col, ai.sel_current_col);
            int text_len = static_cast<int>(str.size());
            int left = 0, right = text_len;

            if (sel_start_line == sel_end_line) {
                left = std::clamp(col_start, 0, text_len);
                right = std::clamp(col_end, left, text_len);
            } else if (abs_line == sel_start_line) {
                left = std::clamp(col_start, 0, text_len);
            } else if (abs_line == sel_end_line) {
                right = std::clamp(col_end, 0, text_len);
            }

            if (left <= 0 && right >= text_len)
                return withDim(base | bgcolor(TC("selection_bg")));

            Elements parts;
            if (left > 0)
                parts.push_back(withDim(text(str.substr(0, left)) | color(col)));
            if (right > left)
                parts.push_back(withDim(text(str.substr(left, right - left)) | color(col) | bgcolor(TC("selection_bg"))));
            if (right < text_len)
                parts.push_back(withDim(text(str.substr(right)) | color(col)));
            return hbox(std::move(parts));
        }
    }

    return withDim(base);
}

Element RenderAIPanel(MainState& state, int tw, int th) {

    if (!state.ai_agent) {
        state.ai_agent = std::make_unique<AIAgent>(state);
    }
    auto& agent = *state.ai_agent;
    auto& ai = state.ai;

    agent.onRenderTick();

    int pw = std::min(76, tw - 6);
    int content_width = pw - 4;
    int conv_height = std::clamp(th * 6 / 10, 8, 30);
    ai.conv_height = conv_height;
    {
        int input_lines = std::max(1, 1 + static_cast<int>(ai.input_text.size()) / std::max(1, content_width - 4));
        int dialog_height = conv_height + input_lines + 8;
        int dialog_top = std::max(0, (th - dialog_height) / 2);
        int dialog_left = (tw - pw) / 2;
        ai.conv_area_x = dialog_left + 1;
        ai.conv_area_y = dialog_top + 3;
        ai.conv_content_width = content_width;
    }

    // Pre-compute display line count per entry group
    ai.entry_line_heights.clear();
    ai.total_display_lines = 0;
    {
        int ei = 0;
        auto& entries = ai.entries;
        while (ei < static_cast<int>(entries.size())) {
            auto type = entries[ei].type;
            int group_lines = 0;
            if (type == AILogEntry::User) {
                int text_lines = 0;
                while (ei < static_cast<int>(entries.size()) && entries[ei].type == AILogEntry::User) {
                    text_lines += std::max(1, static_cast<int>(entries[ei].text.size()) / std::max(1, content_width));
                    ei++;
                }
                group_lines = text_lines + 2;
            } else if (type == AILogEntry::Assistant) {
                int text_lines = 0, step_lines = 0;
                while (ei < static_cast<int>(entries.size()) && entries[ei].type != AILogEntry::User) {
                    int w = std::max(1, static_cast<int>(entries[ei].text.size()) / std::max(1, content_width - 2));
                    if (entries[ei].type == AILogEntry::Assistant) text_lines += w;
                    else step_lines += w;
                    ei++;
                }
                group_lines = 1 + text_lines + step_lines;
            } else {
                while (ei < static_cast<int>(entries.size())
                       && entries[ei].type != AILogEntry::User
                       && entries[ei].type != AILogEntry::Assistant) {
                    group_lines += std::max(1, static_cast<int>(entries[ei].text.size()) / std::max(1, content_width));
                    ei++;
                }
            }
            ai.entry_line_heights.push_back(group_lines);
            ai.total_display_lines += group_lines;
        }
    }

    int max_scroll = std::max(0, ai.total_display_lines - conv_height);
    if (ai.auto_scroll) ai.log_scroll = max_scroll;
    ai.log_scroll = std::clamp(ai.log_scroll, 0, max_scroll);

    static const char spinner_chars[] = {'|', '/', '-', '\\'};
    static const int spinner_len = 4;
    if (agent.isProcessing()) {
        ai.spinner_index = (ai.spinner_index + 1) % spinner_len;
        if (!agent.getStreamBuffer().empty()
            && ai.stream_visible_len < static_cast<int>(agent.getStreamBuffer().size())) {
            if (!agent.isStreaming() && !agent.getPendingResponse().empty()) {
                ai.stream_visible_len = static_cast<int>(agent.getStreamBuffer().size());
            } else {
                int chunk = std::max(1, static_cast<int>(agent.getStreamBuffer().size()) / 40);
                ai.stream_visible_len = std::min(
                    ai.stream_visible_len + chunk,
                    static_cast<int>(agent.getStreamBuffer().size()));
            }
        }
    } else {
        ai.spinner_index = 0;
        ai.stream_visible_len = 0;
    }

    Elements main_els;

    // Title bar
    {
        auto& cfg = ConfigManager::GetInstance()->GetConfig();
        const auto& active_key = cfg.ai.keys.empty()
            ? AIKeyConfig{} : cfg.ai.keys[cfg.ai.active_key % cfg.ai.keys.size()];
        std::string title = " AI  " + active_key.model + "  [";
        title += (active_key.backend == "openai") ? "OpenAI" : "Ollama";
        title += "]";

        auto& sm = SessionManager::getInstance();
        std::string session_info;
        if (auto* s = sm.getSession(ai.current_session_id)) {
            session_info = "  <" + s->name + ">";
        }

        if (agent.isProcessing()) {
            if (agent.isStreaming() && !agent.getStreamBuffer().empty()) {
                title += std::string("  ") + spinner_chars[ai.spinner_index] + " Receiving";
            } else {
                title += std::string("  ") + spinner_chars[ai.spinner_index] + " Thinking";
            }
        }

        main_els.push_back(
            hbox({ text(title) | bold | color(TC("title")),
                   filler(),
                   text(session_info) | color(TC("syn_keyword")) | dim })
        );
        main_els.push_back(separator() | color(TC("main_border")));
    }

    // Conversation area
    {
        if (ai.entries.empty()) {
            main_els.push_back(
                text("  Enter a request below to start.") | color(TC("dim")) | center
            );
        } else {
            // Find first visible group from line offset
            int line_offset = ai.log_scroll;
            int first_group = 0;
            for (int g = 0; g < static_cast<int>(ai.entry_line_heights.size()); ++g) {
                if (ai.entry_line_heights[g] > line_offset) {
                    first_group = g;
                    break;
                }
                line_offset -= ai.entry_line_heights[g];
            }

            // Count visible groups
            int rem = conv_height;
            int visible_groups = 0;
            for (int g = first_group; g < static_cast<int>(ai.entry_line_heights.size()); ++g) {
                if (ai.entry_line_heights[g] <= rem) {
                    rem -= ai.entry_line_heights[g];
                    visible_groups++;
                } else {
                    break;
                }
            }
            if (visible_groups == 0 && first_group < static_cast<int>(ai.entry_line_heights.size())) {
                visible_groups = 1;
            }

            Elements conversation;

            // Precompute selection line range for per-line highlight
            int sel_start_line = ai.sel_dragging
                ? std::min(ai.sel_anchor_line, ai.sel_current_line) : 0;
            int sel_end_line = ai.sel_dragging
                ? std::max(ai.sel_anchor_line, ai.sel_current_line) : -1;
            auto isSelected = [&](int abs_line) -> bool {
                return ai.sel_dragging && abs_line >= sel_start_line && abs_line <= sel_end_line;
            };

            // Build group_map: group index → entry range
            struct GMap { int start_entry, end_entry; };
            std::vector<GMap> group_map;
            {
                int ei = 0;
                for (int g = 0; g < static_cast<int>(ai.entry_line_heights.size()); ++g) {
                    GMap gm;
                    gm.start_entry = ei;
                    auto type = ai.entries[ei].type;
                    if (type == AILogEntry::User) {
                        while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type == AILogEntry::User) ei++;
                    } else if (type == AILogEntry::Assistant) {
                        while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type != AILogEntry::User) ei++;
                    } else {
                        while (ei < static_cast<int>(ai.entries.size())
                            && ai.entries[ei].type != AILogEntry::User
                            && ai.entries[ei].type != AILogEntry::Assistant) ei++;
                    }
                    gm.end_entry = ei - 1;
                    group_map.push_back(gm);
                }
            }

            int last_group_idx = std::min(first_group + visible_groups, static_cast<int>(ai.entry_line_heights.size()));
            int current_abs_line = ai.log_scroll;

            for (int g = first_group; g < last_group_idx; ++g) {
                int gs = group_map[g].start_entry;
                int ge = group_map[g].end_entry;
                auto current_type = ai.entries[gs].type;

                if (current_type == AILogEntry::User) {
                    // "You" header
                    {
                        auto el = hbox({ filler(), text("You") | bold | color(TC("syn_keyword")) });
                        if (isSelected(current_abs_line)) el = el | bgcolor(TC("selection_bg"));
                        conversation.push_back(el);
                        current_abs_line++;
                    }
                    // Wrapped text lines
                    int text_lines = 0;
                    for (int j = gs; j <= ge; ++j) {
                        auto wl = wrapText(ai.entries[j].text, content_width);
                        text_lines += static_cast<int>(wl.size());
                        for (const auto& l : wl) {
                            auto el = hbox({ filler(), text(l) | color(TC("main_fg")) });
                            if (isSelected(current_abs_line)) el = el | bgcolor(TC("selection_bg"));
                            conversation.push_back(el);
                            current_abs_line++;
                        }
                    }
                    // Footer
                    {
                        auto el = hbox({ filler(), text("[" + std::to_string(text_lines) + " lines]") | color(TC("dim")) | dim });
                        if (isSelected(current_abs_line)) el = el | bgcolor(TC("selection_bg"));
                        conversation.push_back(el);
                        current_abs_line++;
                    }
                    conversation.push_back(text(""));
                }
                else if (current_type == AILogEntry::Assistant) {
                    // "Assistant" header
                    {
                        conversation.push_back(makeTextLine("Assistant", TC("success"), current_abs_line, ai) | bold);
                        current_abs_line++;
                    }
                    // Collect content by type
                    std::vector<std::string> texts, steps, successes, errors, systems;
                    for (int j = gs; j <= ge; ++j) {
                        auto t = ai.entries[j].type;
                        if (t == AILogEntry::Assistant) texts.push_back(ai.entries[j].text);
                        else if (t == AILogEntry::Step) steps.push_back(ai.entries[j].text);
                        else if (t == AILogEntry::Success) successes.push_back(ai.entries[j].text);
                        else if (t == AILogEntry::Error) errors.push_back(ai.entries[j].text);
                        else if (t == AILogEntry::System) systems.push_back(ai.entries[j].text);
                    }
                    // Stream buffer (only for last visible group when streaming)
                    std::string sb;
                    if (agent.isStreaming() && !agent.getStreamBuffer().empty()) {
                        if (g == last_group_idx - 1) {
                            int vis = std::min(ai.stream_visible_len,
                                static_cast<int>(agent.getStreamBuffer().size()));
                            sb = agent.getStreamBuffer().substr(0, vis);
                        }
                    }
                    // Content lines (each type with its own prefix + wrap-width)
                    auto renderLines = [&](const std::vector<std::string>& src,
                                            int indent, const std::string& prefix,
                                            ftxui::Color col, bool dim_flag) {
                        for (const auto& s : src) {
                            auto wl = wrapText(s, content_width - indent);
                            for (const auto& l : wl) {
                                conversation.push_back(makeTextLine(prefix + l, col, current_abs_line, ai, dim_flag));
                                current_abs_line++;
                            }
                        }
                    };
                    renderLines(texts, 2, "  ", TC("success"), false);
                    renderLines(std::vector<std::string>(sb.empty() ? 0 : 1, sb),
                                2, "  ", TC("success"), true);
                    renderLines(steps, 4, "    > ", TC("warning"), false);
                    renderLines(successes, 4, "    + ", TC("success"), false);
                    renderLines(errors, 4, "    x ", TC("error"), false);
                    renderLines(systems, 2, "  ", TC("dim"), false);
                    conversation.push_back(text(""));
                }
                else {
                    for (int j = gs; j <= ge; ++j) {
                        auto wl = wrapText(ai.entries[j].text, content_width);
                        for (const auto& l : wl) {
                            conversation.push_back(makeTextLine(" " + l, TC("dim"), current_abs_line, ai));
                            current_abs_line++;
                        }
                    }
                }
            }

            // Stream buffer when no assistant entry yet
            if (agent.isStreaming() && !agent.getStreamBuffer().empty()) {
                bool has_assistant = false;
                for (int j = static_cast<int>(ai.entries.size()) - 1; j >= 0; --j) {
                    if (ai.entries[j].type == AILogEntry::Assistant) { has_assistant = true; break; }
                    if (ai.entries[j].type == AILogEntry::User) break;
                }
                if (!has_assistant) {
                    int vis = std::min(ai.stream_visible_len, static_cast<int>(agent.getStreamBuffer().size()));
                    auto sb = agent.getStreamBuffer().substr(0, vis);
                    {
                        conversation.push_back(makeTextLine("Assistant", TC("success"), current_abs_line, ai) | bold);
                        current_abs_line++;
                    }
                    auto wl = wrapText(sb, content_width - 2);
                    for (const auto& l : wl) {
                        conversation.push_back(makeTextLine("  " + l, TC("success"), current_abs_line, ai, true));
                        current_abs_line++;
                    }
                }
            }

            if (agent.isProcessing() && !agent.isStreaming() && agent.getStreamBuffer().empty()) {
                std::string spin = std::string(1, spinner_chars[ai.spinner_index]);
                conversation.push_back(text("Assistant  " + spin) | color(TC("success")));
            }

            main_els.push_back(vbox(std::move(conversation)) | size(HEIGHT, EQUAL, conv_height));
        }

        if (ai.total_display_lines > 0) {
            int vis_start = ai.log_scroll + 1;
            int vis_end = std::min(ai.log_scroll + conv_height, ai.total_display_lines);
            main_els.push_back(
                text("  Ln " + std::to_string(vis_start) + "-" + std::to_string(vis_end)
                    + " of " + std::to_string(ai.total_display_lines))
                | color(TC("dim")) | dim
            );
        }
    }

    // Input area
    {
        main_els.push_back(separator() | color(TC("main_border")));

        if (agent.isProcessing()) {
            auto input_content = text("  waiting...") | color(TC("dim")) | flex_grow;
            main_els.push_back(
                hbox({ input_content })
                | bgcolor(TC("main_bg"))
                | borderStyled(ROUNDED, TC("main_border"))
            );
        } else {
            ftxui::Color text_color = ai.input_focused
                ? TC("selection_bg") : TC("main_fg");

            int input_w = content_width - 4;
            std::string full_text = ai.input_text;
            int cursor = std::min(ai.input_cursor, static_cast<int>(full_text.size()));

            std::vector<std::string> input_lines;
            for (int pos = 0; pos < static_cast<int>(full_text.size()); pos += input_w) {
                input_lines.push_back(full_text.substr(pos, input_w));
            }
            if (input_lines.empty()) input_lines.push_back("");

            int cursor_row = cursor / input_w;
            int cursor_col = cursor % input_w;
            cursor_row = std::min(cursor_row, static_cast<int>(input_lines.size()) - 1);
            cursor_col = std::min(cursor_col, static_cast<int>(input_lines[cursor_row].size()));

            Elements rows;
            for (int r = 0; r < static_cast<int>(input_lines.size()); r++) {
                const auto& line = input_lines[r];
                Elements line_parts;
                if (r == 0) {
                    line_parts.push_back(text(" > ") | bold | color(TC("indicator")));
                } else {
                    line_parts.push_back(text("   ") | color(TC("dim")));
                }
                if (r == cursor_row) {
                    std::string left = line.substr(0, cursor_col);
                    std::string right = line.substr(cursor_col);
                    if (!left.empty()) line_parts.push_back(text(left) | color(text_color));
                    line_parts.push_back(text(" ") | inverted | bold | color(text_color));
                    if (!right.empty()) line_parts.push_back(text(right) | color(text_color));
                } else {
                    line_parts.push_back(text(line) | color(text_color));
                }
                line_parts.push_back(filler());
                rows.push_back(hbox(std::move(line_parts)));
            }

            main_els.push_back(
                vbox(std::move(rows))
                | borderStyled(ROUNDED, TC("main_border"))
            );
        }
    }

    // Footer hints
    {
        std::string hints;
        if (ai.input_focused && !agent.isProcessing()) {
            hints = " [Enter]=Send  [Tab]=Log  [Esc]=Close";
        } else if (agent.isProcessing()) {
            hints = " [Esc]=Cancel";
        } else {
            hints = " j/k=Scroll  PgUp/PgDn=Page  g/G=Top/Bot  [Tab]=Input  [<]/[>]=Session  [Ctrl+N]=New  [Esc]=Close";
        }
        main_els.push_back(text(hints) | color(TC("dim")) | dim);
    }

    return vbox(std::move(main_els))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, EQUAL, pw)
        | center;
}

static void switchToSession(MainState& state, const std::string& session_id) {
    auto& sm = SessionManager::getInstance();
    auto& agent = *state.ai_agent;
    agent.syncToSessionManager();
    sm.setActiveSession(session_id);
    state.ai.current_session_id = session_id;
    state.ai.entries.clear();
    state.ai.log_scroll = 0;
    state.ai.auto_scroll = true;
    if (auto* session = sm.getSession(session_id)) {
        agent.loadSession(session->messages);
    }
}

bool HandleAIEvent(MainState& state, const Event& event) {
    if (!state.ai_agent) {
        state.ai_agent = std::make_unique<AIAgent>(state);
    }
    auto& agent = *state.ai_agent;
    auto& ai = state.ai;

    if (event == Event::Escape) {
        if (agent.isProcessing()) {
            agent.cancel();
            state.ai.entries.push_back({AILogEntry::Error, "Cancelled by user"});
        } else {
            state.active_panel = ActivePanel::None;
            state.panel_input.clear();
            state.panel_message.clear();
        }
        return true;
    }

    if (event == Event::Tab) {
        ai.input_focused = !ai.input_focused;
        return true;
    }

    if (event == Event::Return) {
        if (ai.input_focused && !agent.isProcessing() && !ai.input_text.empty()) {
            std::string msg = ai.input_text;
            ai.input_text.clear();
            ai.input_cursor = 0;
            ai.input_history.push_back(msg);
            if (ai.input_history.size() > 50) ai.input_history.erase(ai.input_history.begin());
            ai.history_pos = -1;
            ai.saved_input.clear();
            state.ai.entries.push_back({AILogEntry::User, msg});
            agent.sendRequest(msg);
        }
        return true;
    }

    // Input focus mode
    if (ai.input_focused && !agent.isProcessing()) {
        if (event == Event::Backspace) {
            if (ai.input_cursor > 0) {
                ai.input_text.erase(ai.input_cursor - 1, 1);
                ai.input_cursor--;
            }
            return true;
        }
        if (event == Event::Delete) {
            if (ai.input_cursor < static_cast<int>(ai.input_text.size())) {
                ai.input_text.erase(ai.input_cursor, 1);
            }
            return true;
        }
        if (event == Event::ArrowLeft) {
            if (ai.input_cursor > 0) ai.input_cursor--;
            return true;
        }
        if (event == Event::ArrowRight) {
            if (ai.input_cursor < static_cast<int>(ai.input_text.size()))
                ai.input_cursor++;
            return true;
        }
        if (event == Event::ArrowUp) {
            if (!ai.input_history.empty()) {
                if (ai.history_pos == -1) {
                    ai.saved_input = ai.input_text;
                    ai.history_pos = static_cast<int>(ai.input_history.size()) - 1;
                } else if (ai.history_pos > 0) {
                    ai.history_pos--;
                }
                ai.input_text = ai.input_history[ai.history_pos];
                ai.input_cursor = static_cast<int>(ai.input_text.size());
            }
            return true;
        }
        if (event == Event::ArrowDown) {
            if (ai.history_pos != -1) {
                ai.history_pos++;
                if (ai.history_pos >= static_cast<int>(ai.input_history.size())) {
                    ai.history_pos = -1;
                    ai.input_text = ai.saved_input;
                    ai.saved_input.clear();
                } else {
                    ai.input_text = ai.input_history[ai.history_pos];
                }
                ai.input_cursor = static_cast<int>(ai.input_text.size());
            }
            return true;
        }
        if (event == Event::Home) {
            ai.input_cursor = 0;
            return true;
        }
        if (event == Event::End) {
            ai.input_cursor = static_cast<int>(ai.input_text.size());
            return true;
        }
        if (event.is_character() && !event.character().empty()) {
            char c = event.character()[0];
            if (c >= 32 && c <= 126) {
                ai.input_text.insert(ai.input_cursor, 1, c);
                ai.input_cursor++;
            }
            return true;
        }
        return true;
    }

    // Log scroll focus mode (line-based)
    int max_scroll = std::max(0, ai.total_display_lines - ai.conv_height);
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (ai.log_scroll < max_scroll) ai.log_scroll++;
        ai.auto_scroll = (ai.log_scroll >= max_scroll);
        return true;
    }
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (ai.log_scroll > 0) ai.log_scroll--;
        ai.auto_scroll = false;
        return true;
    }
    if (event == Event::PageDown || event == Event::Character('f')) {
        int page = std::max(1, ai.conv_height - 1);
        ai.log_scroll = std::min(max_scroll, ai.log_scroll + page);
        ai.auto_scroll = (ai.log_scroll >= max_scroll);
        return true;
    }
    if (event == Event::PageUp || event == Event::Character('b')) {
        int page = std::max(1, ai.conv_height - 1);
        ai.log_scroll = std::max(0, ai.log_scroll - page);
        ai.auto_scroll = false;
        return true;
    }
    if (event == Event::Home || event == Event::Character('g')) {
        if (ai.input_focused) {
            ai.input_cursor = 0;
        } else {
            ai.log_scroll = 0;
            ai.auto_scroll = false;
        }
        return true;
    }
    if (event == Event::End || event == Event::Character('G')) {
        if (ai.input_focused) {
            ai.input_cursor = static_cast<int>(ai.input_text.size());
        } else {
            ai.log_scroll = max_scroll;
            ai.auto_scroll = true;
        }
        return true;
    }

    // Session switching (only in log focus mode, not processing)
    if (!ai.input_focused && !agent.isProcessing()) {
        if (event == Event::Character('<')) {
            auto& sm = SessionManager::getInstance();
            const auto& sessions = sm.sessions();
            if (sessions.size() > 1) {
                int idx = -1;
                for (size_t i = 0; i < sessions.size(); ++i) {
                    if (sessions[i].id == ai.current_session_id) { idx = i; break; }
                }
                int prev = (idx - 1 + static_cast<int>(sessions.size())) % sessions.size();
                switchToSession(state, sessions[prev].id);
            }
            return true;
        }
        if (event == Event::Character('>')) {
            auto& sm = SessionManager::getInstance();
            const auto& sessions = sm.sessions();
            if (sessions.size() > 1) {
                int idx = -1;
                for (size_t i = 0; i < sessions.size(); ++i) {
                    if (sessions[i].id == ai.current_session_id) { idx = i; break; }
                }
                int next = (idx + 1) % sessions.size();
                switchToSession(state, sessions[next].id);
            }
            return true;
        }
        if (event == Event::Character('\x0e')) {
            auto& sm = SessionManager::getInstance();
            std::string nid = sm.createSession();
            ai.current_session_id = nid;
            state.ai_agent->loadSession({});
            return true;
        }
        if (event == Event::Character('\x04')) {
            auto& sm = SessionManager::getInstance();
            const auto& sessions = sm.sessions();
            if (sessions.size() <= 1) return true;
            agent.syncToSessionManager();
            sm.deleteSession(ai.current_session_id);
            ai.current_session_id = sm.activeSessionId();
            sm.save(sm.filePath());
            if (auto* s = sm.getSession(sm.activeSessionId())) {
                agent.loadSession(s->messages);
            }
            return true;
        }
    }

    return true;
}

std::string ExtractAISelectedText(const MainState& state) {
    const auto& ai = state.ai;
    int sel_start = std::min(ai.sel_anchor_line, ai.sel_current_line);
    int sel_end = std::max(ai.sel_anchor_line, ai.sel_current_line);
    if (sel_start > sel_end || sel_end < 0 || sel_start >= ai.total_display_lines) return "";

    sel_start = std::max(0, sel_start);
    sel_end = std::min(sel_end, ai.total_display_lines - 1);

    int col_start = std::min(ai.sel_anchor_col, ai.sel_current_col);
    int col_end = std::max(ai.sel_anchor_col, ai.sel_current_col);

    int content_width = ai.conv_content_width;
    if (content_width <= 0) content_width = 72;

    std::string result;
    int ei = 0;
    int accum = 0;

    // Append line with column-range trimming
    auto appendLine = [&](std::string line, int abs_line) {
        if (col_end > col_start && abs_line >= sel_start && abs_line <= sel_end) {
            int left = 0, right = static_cast<int>(line.size());
            if (sel_start == sel_end) {
                left = std::clamp(col_start, 0, right);
                right = std::clamp(col_end, left, right);
            } else if (abs_line == sel_start) {
                left = std::clamp(col_start, 0, right);
            } else if (abs_line == sel_end) {
                right = std::clamp(col_end, 0, right);
            }
            line = line.substr(left, right - left);
        }
        if (!line.empty()) {
            if (!result.empty()) result += "\n";
            result += line;
        }
    };

    for (int g = 0; g < static_cast<int>(ai.entry_line_heights.size()) && accum <= sel_end; ++g) {
        int gh = ai.entry_line_heights[g];
        int g_end = accum + gh - 1;
        if (g_end < sel_start) {
            accum += gh;
            auto t = ai.entries[ei].type;
            if (t == AILogEntry::User) {
                while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type == AILogEntry::User) ei++;
            } else if (t == AILogEntry::Assistant) {
                while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type != AILogEntry::User) ei++;
            } else {
                while (ei < static_cast<int>(ai.entries.size())
                    && ai.entries[ei].type != AILogEntry::User
                    && ai.entries[ei].type != AILogEntry::Assistant) ei++;
            }
            continue;
        }

        int local_start = std::max(0, sel_start - accum);
        int local_end = std::min(gh - 1, sel_end - accum);
        auto type = ai.entries[ei].type;

        if (type == AILogEntry::User) {
            // Lines: 0=header, 1..N-1=content, N=footer
            int line_in_group = 0;
            // Header
            int abs_line = accum;
            if (line_in_group >= local_start && line_in_group <= local_end)
                appendLine("You", abs_line);
            line_in_group++; abs_line++;

            int temp_ei = ei;
            while (temp_ei < static_cast<int>(ai.entries.size()) && ai.entries[temp_ei].type == AILogEntry::User) {
                auto wl = wrapText(ai.entries[temp_ei].text, content_width);
                for (const auto& l : wl) {
                    if (line_in_group >= local_start && line_in_group <= local_end)
                        appendLine(l, abs_line);
                    line_in_group++; abs_line++;
                }
                temp_ei++;
            }
            // Footer
            if (line_in_group >= local_start && line_in_group <= local_end)
                appendLine("[...]", abs_line);
            line_in_group++; abs_line++;
        }
        else if (type == AILogEntry::Assistant) {
            int line_in_group = 0;
            int abs_line = accum;
            // Header
            if (line_in_group >= local_start && line_in_group <= local_end)
                appendLine("Assistant", abs_line);
            line_in_group++; abs_line++;

            int temp_ei = ei;
            while (temp_ei < static_cast<int>(ai.entries.size()) && ai.entries[temp_ei].type != AILogEntry::User) {
                auto te = ai.entries[temp_ei].type;
                int indent;
                std::string prefix;
                if (te == AILogEntry::Assistant) {
                    indent = 2; prefix = "  ";
                } else if (te == AILogEntry::Step) {
                    indent = 4; prefix = "    > ";
                } else if (te == AILogEntry::Success) {
                    indent = 4; prefix = "    + ";
                } else if (te == AILogEntry::Error) {
                    indent = 4; prefix = "    x ";
                } else {
                    indent = 2; prefix = "  ";
                }
                auto wl = wrapText(ai.entries[temp_ei].text, content_width - indent);
                for (const auto& l : wl) {
                    if (line_in_group >= local_start && line_in_group <= local_end)
                        appendLine(prefix + l, abs_line);
                    line_in_group++; abs_line++;
                }
                temp_ei++;
            }
        }
        else {
            int line_in_group = 0;
            int abs_line = accum;
            int temp_ei = ei;
            while (temp_ei < static_cast<int>(ai.entries.size())
                && ai.entries[temp_ei].type != AILogEntry::User
                && ai.entries[temp_ei].type != AILogEntry::Assistant) {
                auto wl = wrapText(ai.entries[temp_ei].text, content_width);
                for (const auto& l : wl) {
                    if (line_in_group >= local_start && line_in_group <= local_end)
                        appendLine(" " + l, abs_line);
                    line_in_group++; abs_line++;
                }
                temp_ei++;
            }
        }

        accum += gh;
        auto t = ai.entries[ei].type;
        if (t == AILogEntry::User) {
            while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type == AILogEntry::User) ei++;
        } else if (t == AILogEntry::Assistant) {
            while (ei < static_cast<int>(ai.entries.size()) && ai.entries[ei].type != AILogEntry::User) ei++;
        } else {
            while (ei < static_cast<int>(ai.entries.size())
                && ai.entries[ei].type != AILogEntry::User
                && ai.entries[ei].type != AILogEntry::Assistant) ei++;
        }
    }

    return result;
}

} // namespace FTB::UI
