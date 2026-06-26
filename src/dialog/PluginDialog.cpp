#include "../../include/dialog/PluginDialog.hpp"
#include "../../include/ops/PluginManager.hpp"

namespace FTB::UI {

using namespace ftxui;

Element RenderPluginPanel(MainState& state, int tw, int th) {
    int pw = std::min(64, tw - 4);
    int ph = std::min(26, th - 4);

    auto* pm = PluginManager::GetInstance();
    auto plugins = pm->GetAvailablePlugins();

    Elements els;

    // ── Title ──
    els.push_back(text(" Plugin Manager") | color(TC("title")) | bold);
    els.push_back(separator());

    if (plugins.empty()) {
        els.push_back(text(""));
        els.push_back(text("  No plugins found.") | color(TC("dim")));
        els.push_back(text(""));
        els.push_back(text("  Install plugins to:") | color(TC("dim")));
        els.push_back(text("    ~/.config/ftb/plugins/<name>.ftb/") | color(TC("syn_string")));
        els.push_back(text(""));
        els.push_back(text("  Each plugin needs:") | color(TC("dim")));
        els.push_back(hbox({
            text("    main.ts       ") | color(TC("syn_identifier")),
            text(" - Entry point") | color(TC("dim"))
        }));
        els.push_back(hbox({
            text("    package.json  ") | color(TC("syn_identifier")),
            text(" - Metadata & permissions") | color(TC("dim"))
        }));
    } else {
        // ── Plugin list ──
        for (int i = 0; i < static_cast<int>(plugins.size()); ++i) {
            const auto& name = plugins[i];
            bool is_selected = (i == state.panel_selected);
            bool is_enabled = pm->IsPluginEnabled(name);
            auto status = pm->GetPluginStatus(name);

            std::string state_str = status.value("state", "unknown");
            std::string version = status.value("version", "");

            // State indicator
            std::string indicator;
            Color indicator_color;
            if (state_str == "loaded") {
                indicator = "+";
                indicator_color = TC("syn_string");
            } else if (state_str == "error") {
                indicator = "x";
                indicator_color = TC("syn_preprocessor");
            } else if (state_str == "disabled") {
                indicator = "-";
                indicator_color = TC("syn_comment");
            } else {
                indicator = "o";
                indicator_color = TC("dim");
            }

            // Build row
            std::string prefix = is_selected ? " > " : "   ";
            std::string ver = version.empty() ? "" : " v" + version;
            std::string off_tag = (!is_enabled && state_str != "disabled") ? " [off]" : "";

            auto name_style = is_selected ? bgcolor(TC("selection_bg")) | color(TC("selection_fg")) | bold
                                          : color(TC("main_fg"));

            els.push_back(hbox({
                text(prefix) | color(TC("indicator")),
                text(indicator) | color(indicator_color),
                text(" "),
                text(name) | name_style,
                text(ver) | color(TC("dim")),
                text(off_tag) | color(TC("syn_comment")),
            }));
        }

        els.push_back(separator());

        // ── Detail section for selected plugin ──
        if (state.panel_selected >= 0 && state.panel_selected < static_cast<int>(plugins.size())) {
            const auto& sel_name = plugins[state.panel_selected];
            auto detail = pm->GetPluginStatus(sel_name);

            // Left: basic info
            Elements detail_left;
            detail_left.push_back(hbox({
                text(" Name:    ") | color(TC("dim")),
                text(detail.value("name", "")) | color(TC("title")) | bold
            }));
            detail_left.push_back(hbox({
                text(" Type:    ") | color(TC("dim")),
                text(detail.value("type", "")) | color(TC("syn_type"))
            }));
            detail_left.push_back(hbox({
                text(" State:   ") | color(TC("dim")),
                text(detail.value("state", "")) | color(detail.value("state", "") == "loaded" ? TC("syn_string") : TC("main_fg"))
            }));
            detail_left.push_back(hbox({
                text(" Enabled: ") | color(TC("dim")),
                text(detail.value("enabled", false) ? "Yes" : "No") | color(detail.value("enabled", false) ? TC("syn_string") : TC("syn_comment"))
            }));

            std::string desc = detail.value("description", "");
            if (!desc.empty()) {
                detail_left.push_back(hbox({
                    text(" Desc:    ") | color(TC("dim")),
                    text(desc) | color(TC("syn_comment"))
                }));
            }

            if (!detail.value("error", "").empty()) {
                detail_left.push_back(hbox({
                    text(" Error:   ") | color(TC("dim")),
                    text(detail.value("error", "")) | color(TC("syn_preprocessor"))
                }));
            }

            // Right: permissions
            Elements detail_right;
            if (detail.contains("permissions")) {
                auto perms = detail["permissions"];
                detail_right.push_back(text(" Permissions") | color(TC("dim")) | bold);

                const std::vector<std::pair<std::string, std::string>> perm_items = {
                    {"fs_read", "fs.read"}, {"fs_write", "fs.write"}, {"fs_list", "fs.list"},
                    {"net_fetch", "net.fetch"}, {"env_read", "env.get"}, {"clipboard", "clipboard"},
                    {"subprocess", "exec"}
                };

                Elements perm_line;
                for (const auto& [key, label] : perm_items) {
                    if (perms.value(key, false)) {
                        perm_line.push_back(text(" [" + label + "]") | color(TC("syn_keyword")));
                    }
                }
                if (perm_line.empty()) {
                    detail_right.push_back(text("  (none)") | color(TC("dim")));
                } else {
                    detail_right.push_back(hbox(perm_line));
                }
            }

            els.push_back(hbox({
                vbox(detail_left) | flex,
                separator() | color(TC("main_border")),
                vbox(detail_right),
            }));
        }
    }

    // ── Message bar ──
    if (!state.panel_message.empty()) {
        els.push_back(separator());
        els.push_back(text(" " + state.panel_message) | color(TC("syn_string")));
    }

    // ── Footer ──
    els.push_back(text(""));
    els.push_back(text(" Enter=Run  d=Toggle  r=Reload  Esc=Close") | color(TC("dim")) | dim);

    return vbox(els) | bgcolor(TC("main_bg")) | GetPanelBorder() |
           size(WIDTH, EQUAL, pw) | size(HEIGHT, EQUAL, ph) | center;
}

// ─── Event handling ──────────────────────────────────────────────────

bool HandlePluginEvent(MainState& state, const ftxui::Event& event) {
    auto* pm = PluginManager::GetInstance();
    auto plugins = pm->GetAvailablePlugins();

    if (event == Event::Escape || event == Event::Character('q')) {
        state.active_panel = ActivePanel::None;
        state.panel_message.clear();
        return true;
    }

    // Navigation
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.panel_selected > 0) state.panel_selected--;
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.panel_selected < static_cast<int>(plugins.size()) - 1) state.panel_selected++;
        return true;
    }

    // Enter: execute plugin
    if (event == Event::Return) {
        if (state.panel_selected >= 0 && state.panel_selected < static_cast<int>(plugins.size())) {
            const auto& name = plugins[state.panel_selected];
            if (!pm->IsPluginEnabled(name)) {
                state.panel_message = "Plugin is disabled. Press 'd' to enable.";
                return true;
            }
            PluginContext ctx;
            ctx.current_path = state.currentPath;
            if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
                ctx.selected_file = state.filteredContents[state.selected];
                ctx.selected_file_path = state.currentPath + "/" + ctx.selected_file;
            }
            auto result = pm->ExecutePlugin(name, ctx);
            if (!result.message.empty()) {
                state.panel_message = result.message;
            } else if (!result.success) {
                state.panel_message = "Error: " + result.error;
            } else {
                state.panel_message = "Plugin '" + name + "' executed successfully";
            }
        }
        return true;
    }

    // d: toggle enable/disable
    if (event == Event::Character('d')) {
        if (state.panel_selected >= 0 && state.panel_selected < static_cast<int>(plugins.size())) {
            const auto& name = plugins[state.panel_selected];
            if (pm->IsPluginEnabled(name)) {
                pm->DisablePlugin(name);
                state.panel_message = "Plugin '" + name + "' disabled";
            } else {
                pm->EnablePlugin(name);
                state.panel_message = "Plugin '" + name + "' enabled";
            }
        }
        return true;
    }

    // r: reload plugin
    if (event == Event::Character('r')) {
        if (state.panel_selected >= 0 && state.panel_selected < static_cast<int>(plugins.size())) {
            const auto& name = plugins[state.panel_selected];
            pm->ReloadPlugin(name);
            state.panel_message = "Plugin '" + name + "' reloaded";
        }
        return true;
    }

    return true;
}

}  // namespace FTB::UI
