#include "dialog/AIConfigDialog.hpp"
#include "config/ConfigManager.hpp"
#include "utils/StatusMessage.hpp"

#include <algorithm>

namespace FTB { namespace UI {

using namespace ftxui;

static const std::vector<std::string> kBackends = {"ollama", "openai"};

// ── helpers ──

static std::string backendLabel(const std::string& b) {
    return (b == "openai") ? "OpenAI" : "Ollama";
}

static const AIKeyConfig& currentKey(const MainState& state) {
    static AIKeyConfig fallback;
    if (state.ai_config_keys.empty()) return fallback;
    int idx = std::clamp(state.ai_config_selected, 0,
        static_cast<int>(state.ai_config_keys.size()) - 1);
    return state.ai_config_keys[idx];
}

static AIKeyConfig& currentKeyMut(MainState& state) {
    static AIKeyConfig fallback;
    if (state.ai_config_keys.empty()) {
        state.ai_config_keys.push_back({});
        state.ai_config_selected = 0;
    }
    int idx = std::clamp(state.ai_config_selected, 0,
        static_cast<int>(state.ai_config_keys.size()) - 1);
    return state.ai_config_keys[idx];
}

// ── render single field ──

static Element renderField(const std::string& label, const std::string& value,
                           bool active, bool masked) {
    Elements row;
    row.push_back(text(active ? " > " : "   ") | color(TC("indicator")));
    row.push_back(text(label) | color(TC("title")) | bold | size(WIDTH, EQUAL, 14));

    std::string display;
    if (masked && !value.empty()) {
        display = std::string(value.size(), '*');
    } else {
        display = value.empty() ? "(empty)" : value;
    }

    row.push_back(text(display)
        | (active ? color(TC("syn_keyword")) | bold : color(TC("main_fg")))
        | flex_grow);

    if (label == "Backend") {
        row.push_back(text("  [Tab to toggle]") | color(TC("dim")));
    }

    return hbox(std::move(row));
}

// ── render key list row ──

static Element renderKeyRow(int idx, const AIKeyConfig& k, bool selected,
                            bool show_model, bool is_active) {
    std::string marker = selected ? " \u25b8 " : "   ";
    std::string num = std::to_string(idx + 1);
    Elements row;
    row.push_back(text(marker) | (selected ? color(TC("indicator")) : color(TC("dim"))));
    row.push_back(text(num + ".") | color(TC("dim")) | size(WIDTH, EQUAL, 3));

    Color name_col = selected ? TC("syn_keyword") : TC("main_fg");
    row.push_back(text(k.name) | color(name_col) | bold | size(WIDTH, EQUAL, 18));

    if (show_model) {
        row.push_back(text(k.model) | color(TC("main_fg")) | size(WIDTH, EQUAL, 20));
    } else {
        row.push_back(text(backendLabel(k.backend)) | color(TC("main_fg")) | size(WIDTH, EQUAL, 10));
        std::string ep = k.endpoint;
        if (ep.size() > 32) ep = ep.substr(0, 30) + "...";
        row.push_back(text(ep) | color(TC("dim")) | flex_grow);
    }

    if (is_active) {
        row.push_back(text("  \u2605") | color(TC("success")) | bold);
    }

    return hbox(std::move(row));
}

// ── tab labels ──

static const std::vector<std::string> kTabLabels = {" API Keys ", " Model Switcher "};

// ── RenderAIConfigPanel ──

Element RenderAIConfigPanel(MainState& state, int /*tw*/, int /*th*/) {
    Elements rows;

    // Title
    rows.push_back(hbox({ text(" AI Configuration") | color(TC("title")) | bold, filler() }));
    rows.push_back(separator() | color(TC("main_border")));

    // Tab bar
    {
        Elements tab_els;
        for (int i = 0; i < static_cast<int>(kTabLabels.size()); ++i) {
            bool active = (i == state.ai_config_tab);
            auto el = text(kTabLabels[i])
                | (active ? (bold | color(TC("title"))) : (color(TC("dim")) | dim));
            if (active) el = el | bgcolor(TC("selection_bg"));
            tab_els.push_back(el);
            if (i < static_cast<int>(kTabLabels.size()) - 1) {
                tab_els.push_back(text(" | ") | color(TC("main_border")));
            }
        }
        rows.push_back(hbox(std::move(tab_els)));
    }

    rows.push_back(separatorLight() | color(TC("main_border")));

    if (state.ai_config_tab == 0) {
        // ── Tab 1: API Keys ──
        if (state.ai_config_keys.empty()) {
            rows.push_back(text("  No keys configured. Press Ctrl+N to add.") | color(TC("dim")));
        } else {
            for (int i = 0; i < static_cast<int>(state.ai_config_keys.size()); ++i) {
                rows.push_back(renderKeyRow(i, state.ai_config_keys[i],
                    i == state.ai_config_selected, false,
                    i == state.ai_config_active_key));
            }
        }

        rows.push_back(text(""));

        if (state.ai_config_editing && !state.ai_config_keys.empty()) {
            // Edit form
            rows.push_back(separator() | color(TC("main_border")));
            const auto& k = currentKey(state);
            rows.push_back(renderField("Name", k.name, state.ai_config_field == 0, false));
            rows.push_back(renderField("Backend", backendLabel(k.backend), state.ai_config_field == 1, false));
            rows.push_back(renderField("Endpoint", k.endpoint, state.ai_config_field == 2, false));
            rows.push_back(renderField("API Key", k.api_key, state.ai_config_field == 3, true));
            rows.push_back(renderField("Model", k.model, state.ai_config_field == 4, false));
        }

        rows.push_back(separatorLight() | color(TC("main_border")));
        if (state.ai_config_editing) {
            rows.push_back(
                hbox({
                    text("  \u2191\u2195 Field  ") | color(TC("dim")),
                    text("Tab Toggle Backend  ") | color(TC("dim")),
                    text("Enter Save  ") | color(TC("syn_keyword")) | bold,
                    text("Esc Cancel") | color(TC("dim")),
                    filler()
                })
            );
        } else {
            rows.push_back(
                hbox({
                    text("  \u2191\u2195 Select  ") | color(TC("dim")),
                    text("Enter Edit  ") | color(TC("syn_keyword")) | bold,
                    text("Ctrl+N Add  ") | color(TC("success")) | bold,
                    text("Ctrl+D Delete  ") | color(TC("error")) | bold,
                    text("\u2190\u2192 Tabs  ") | color(TC("dim")),
                    text("Esc Close") | color(TC("dim")),
                    filler()
                })
            );
        }
    } else {
        // ── Tab 2: Model Switcher ──
        if (state.ai_config_keys.empty()) {
            rows.push_back(text("  No keys configured. Press Ctrl+N to add.") | color(TC("dim")));
        } else {
            for (int i = 0; i < static_cast<int>(state.ai_config_keys.size()); ++i) {
                rows.push_back(renderKeyRow(i, state.ai_config_keys[i],
                    i == state.ai_config_selected, true,
                    i == state.ai_config_active_key));
            }
        }

        rows.push_back(separatorLight() | color(TC("main_border")));
        rows.push_back(
            hbox({
                text("  \u2191\u2195 Select  ") | color(TC("dim")),
                text("Enter Activate  ") | color(TC("syn_keyword")) | bold,
                text("E Edit  ") | color(TC("success")) | bold,
                text("Ctrl+N Add  ") | color(TC("success")) | bold,
                text("Ctrl+D Delete  ") | color(TC("error")) | bold,
                text("Esc Close") | color(TC("dim")),
                filler()
            })
        );
    }

    Element panel = vbox(std::move(rows))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, GREATER_THAN, 64);

    return panel | center;
}

// ── HandleAIConfigEvent ──

bool HandleAIConfigEvent(MainState& state, const Event& event) {
    // ── Common: close ──
    if (event == Event::Escape) {
        if (state.ai_config_editing) {
            state.ai_config_editing = false;
            return true;
        }
        // Save before closing
        auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
        ai_cfg.keys = state.ai_config_keys;
        ai_cfg.active_key = state.ai_config_active_key;
        ConfigManager::GetInstance()->SaveConfig();
        StatusMessage::Show("AI config saved");
        state.active_panel = ActivePanel::None;
        return true;
    }

    // ── Common: save & close (Ctrl+S or Enter in non-edit mode) ──
    if (event == Event::Character('\x13')  // Ctrl+S
        || (event == Event::Return && !state.ai_config_editing && state.ai_config_tab == 0))
    {
        auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
        ai_cfg.keys = state.ai_config_keys;
        ai_cfg.active_key = state.ai_config_active_key;
        ConfigManager::GetInstance()->SaveConfig();
        StatusMessage::Show("AI config saved");
        state.active_panel = ActivePanel::None;
        return true;
    }

    // ── Tab switching ──
    if (event == Event::Tab && !state.ai_config_editing) {
        state.ai_config_tab = (state.ai_config_tab + 1) % kTabLabels.size();
        return true;
    }
    if ((event == Event::ArrowRight || event == Event::Character('l'))
        && !state.ai_config_editing)
    {
        if (state.ai_config_tab < static_cast<int>(kTabLabels.size()) - 1)
            state.ai_config_tab++;
        return true;
    }
    if ((event == Event::ArrowLeft || event == Event::Character('h'))
        && !state.ai_config_editing)
    {
        if (state.ai_config_tab > 0)
            state.ai_config_tab--;
        return true;
    }

    // ── Tab 1: API Keys ──
    if (state.ai_config_tab == 0) {
        if (state.ai_config_editing) {
            // ── Editing mode: navigate/edit fields ──
            if (event == Event::ArrowUp) {
                if (state.ai_config_field > 0) state.ai_config_field--;
                return true;
            }
            if (event == Event::ArrowDown) {
                if (state.ai_config_field < 4) state.ai_config_field++;
                return true;
            }

            // Tab: toggle backend on field 1, then advance to next field
            if (event == Event::Tab) {
                if (state.ai_config_field == 1) {
                    auto& k = currentKeyMut(state);
                    int idx = 0;
                    for (size_t i = 0; i < kBackends.size(); ++i) {
                        if (kBackends[i] == k.backend) {
                            idx = (static_cast<int>(i) + 1) % kBackends.size();
                            break;
                        }
                    }
                    k.backend = kBackends[idx];
                }
                if (state.ai_config_field < 4)
                    state.ai_config_field++;
                else
                    state.ai_config_field = 0;
                return true;
            }

            // Enter: exit editing mode (field changes already applied)
            if (event == Event::Return) {
                state.ai_config_editing = false;
                return true;
            }

            // Character input and backspace
            if (event == Event::Backspace) {
                auto& k = currentKeyMut(state);
                switch (state.ai_config_field) {
                    case 0: if (!k.name.empty()) k.name.pop_back(); break;
                    case 2: if (!k.endpoint.empty()) k.endpoint.pop_back(); break;
                    case 3: if (!k.api_key.empty()) k.api_key.pop_back(); break;
                    case 4: if (!k.model.empty()) k.model.pop_back(); break;
                    default: break;
                }
                return true;
            }

            if (event.is_character()) {
                char c = event.character()[0];
                if (c < 32 || c > 126) return true;
                auto& k = currentKeyMut(state);
                switch (state.ai_config_field) {
                    case 0: k.name += c; break;
                    case 2: k.endpoint += c; break;
                    case 3: k.api_key += c; break;
                    case 4: k.model += c; break;
                    default: break;
                }
                return true;
            }

            return true;
        }
        // ── Non-editing mode ──
        else {
            if (event == Event::ArrowUp || event == Event::Character('k')) {
                if (state.ai_config_selected > 0) state.ai_config_selected--;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j')) {
                if (state.ai_config_selected <
                    static_cast<int>(state.ai_config_keys.size()) - 1)
                    state.ai_config_selected++;
                return true;
            }

            if (event == Event::Return) {
                state.ai_config_editing = true;
                state.ai_config_field = 0;
                return true;
            }

            // Ctrl+N: add new key
            if (event == Event::Character('\x0e')) {
                AIKeyConfig new_key;
                if (!state.ai_config_keys.empty()) {
                    auto& ref = state.ai_config_keys[state.ai_config_selected];
                    new_key.name = ref.name + " (copy)";
                    new_key.backend = ref.backend;
                    new_key.endpoint = ref.endpoint;
                    new_key.api_key = ref.api_key;
                    new_key.model = ref.model;
                }
                int insert_pos = std::min(state.ai_config_selected + 1,
                    static_cast<int>(state.ai_config_keys.size()));
                state.ai_config_keys.insert(
                    state.ai_config_keys.begin() + insert_pos, new_key);
                state.ai_config_selected = insert_pos;
                return true;
            }

            // Ctrl+D: delete key
            if (event == Event::Character('\x04')) {
                if (state.ai_config_keys.size() <= 1) return true;
                int idx = state.ai_config_selected;
                state.ai_config_keys.erase(
                    state.ai_config_keys.begin() + idx);
                if (state.ai_config_active_key == idx)
                    state.ai_config_active_key = 0;
                else if (state.ai_config_active_key > idx)
                    state.ai_config_active_key--;
                state.ai_config_selected = std::min(idx,
                    static_cast<int>(state.ai_config_keys.size()) - 1);
                return true;
            }

            return true;
        }
    }

    // ── Tab 2: Model Switcher ──
    if (state.ai_config_tab == 1) {
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            if (state.ai_config_selected > 0) state.ai_config_selected--;
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            if (state.ai_config_selected <
                static_cast<int>(state.ai_config_keys.size()) - 1)
                state.ai_config_selected++;
            return true;
        }

        // Enter: activate this key
        if (event == Event::Return) {
            state.ai_config_active_key = state.ai_config_selected;
            auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
            ai_cfg.keys = state.ai_config_keys;
            ai_cfg.active_key = state.ai_config_active_key;
            ConfigManager::GetInstance()->SaveConfig();
            StatusMessage::Show("Activated: " + currentKey(state).name);
            return true;
        }

        // E: switch to Tab 1 and edit this key
        if (event == Event::Character('e') || event == Event::Character('E')) {
            state.ai_config_tab = 0;
            state.ai_config_editing = true;
            state.ai_config_field = 0;
            return true;
        }

        // Ctrl+N: add new key
        if (event == Event::Character('\x0e')) {
            AIKeyConfig new_key;
            if (!state.ai_config_keys.empty()) {
                auto& ref = state.ai_config_keys[state.ai_config_selected];
                new_key.name = ref.name + " (copy)";
                new_key.backend = ref.backend;
                new_key.endpoint = ref.endpoint;
                new_key.model = ref.model;
            }
            int insert_pos = std::min(state.ai_config_selected + 1,
                static_cast<int>(state.ai_config_keys.size()));
            state.ai_config_keys.insert(
                state.ai_config_keys.begin() + insert_pos, new_key);
            state.ai_config_selected = insert_pos;
            // Also persist immediately so active_key stays valid
            auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
            ai_cfg.keys = state.ai_config_keys;
            ai_cfg.active_key = state.ai_config_active_key;
            ConfigManager::GetInstance()->SaveConfig();
            return true;
        }

        // Ctrl+D: delete key
        if (event == Event::Character('\x04')) {
            if (state.ai_config_keys.size() <= 1) return true;
            int idx = state.ai_config_selected;
            state.ai_config_keys.erase(
                state.ai_config_keys.begin() + idx);
            if (state.ai_config_active_key == idx)
                state.ai_config_active_key = 0;
            else if (state.ai_config_active_key > idx)
                state.ai_config_active_key--;
            state.ai_config_selected = std::min(idx,
                static_cast<int>(state.ai_config_keys.size()) - 1);
            auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
            ai_cfg.keys = state.ai_config_keys;
            ai_cfg.active_key = state.ai_config_active_key;
            ConfigManager::GetInstance()->SaveConfig();
            return true;
        }

        return true;
    }

    return true;
}

}} // namespace FTB::UI
