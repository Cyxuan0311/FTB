#include "dialog/OpenerConfigDialog.hpp"
#include "ops/OpenerManager.hpp"
#include "config/ConfigManager.hpp"
#include "utils/StatusMessage.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>

namespace FTB { namespace UI {

using namespace ftxui;

static Element RenderField(const std::string& label, const std::string& value,
                           bool active, bool is_orphan = false) {
    Elements row;
    std::string marker = active ? " > " : "   ";
    row.push_back(text(marker) | color(TC("indicator")));
    row.push_back(text(label) | color(TC("title")) | bold | size(WIDTH, EQUAL, 12));
    if (label == "Mode") {
        row.push_back(text(is_orphan ? "GUI (Background)" : "TUI (Block)")
            | (active ? color(TC("syn_keyword")) | bold : color(TC("main_fg"))));
    } else {
        row.push_back(text(value.empty() ? "(empty)" : value)
            | (active ? color(TC("syn_keyword")) | bold : color(TC("main_fg"))));
    }
    return hbox(std::move(row));
}

Element RenderOpenerConfigPanel(MainState& state, int /*tw*/, int /*th*/) {
    Elements rows;

    rows.push_back(
        hbox({
            text(" Configure Openers") | color(TC("title")) | bold,
            filler()
        })
    );
    rows.push_back(separator() | color(TC("main_border")));

    rows.push_back(text(""));
    rows.push_back(text("  New Opener:") | color(TC("syn_keyword")) | bold);
    rows.push_back(text(""));

    rows.push_back(RenderField("Name", state.opener_config_name,
        state.opener_config_field == 0));
    rows.push_back(RenderField("Command", state.opener_config_run,
        state.opener_config_field == 1));
    rows.push_back(RenderField("Description", state.opener_config_desc,
        state.opener_config_field == 2));
    rows.push_back(RenderField("Mode", "",
        state.opener_config_field == 3, state.opener_config_is_orphan));

    rows.push_back(text(""));
    rows.push_back(text("  New Rule:") | color(TC("syn_keyword")) | bold);
    rows.push_back(text(""));
    rows.push_back(RenderField("File Pattern", state.opener_config_rule_name,
        state.opener_config_field == 4));
    rows.push_back(RenderField("Use Opener", state.opener_config_rule_use,
        state.opener_config_field == 5));

    rows.push_back(text(""));
    rows.push_back(text("  Current Openers:") | color(TC("dim")));
    auto names = OpenerManager::Instance().GetOpenerNames();
    for (const auto& name : names) {
        auto opener = OpenerManager::Instance().GetOpener(name);
        if (opener) {
            std::string mode = opener->orphan ? "GUI" : "TUI";
            rows.push_back(text("    " + name + " [" + mode + "] " + opener->run) | color(TC("dim")));
        }
    }

    rows.push_back(separator() | color(TC("main_border")));
    rows.push_back(
        hbox({
            text("  ") | color(TC("dim")),
            text("\u2191\u2193 Fields  ") | color(TC("dim")),
            text("Tab Next  ") | color(TC("dim")),
            text("Enter Save  ") | color(TC("syn_keyword")) | bold,
            text("Esc Cancel") | color(TC("dim")),
            filler()
        })
    );

    Element panel = vbox(std::move(rows))
        | bgcolor(TC("main_bg"))
        | GetPanelBorder()
        | size(WIDTH, GREATER_THAN, 55);

    return panel | center;
}

static void SaveOpenerConfig(const MainState& state) {
    try {
        std::string config_path = std::string(getenv("HOME") ? getenv("HOME") : "/tmp")
                                  + "/.config/ftb/ftb.json";

        nlohmann::json root;
        if (std::filesystem::exists(config_path)) {
            std::ifstream file(config_path);
            if (file.is_open()) {
                root = nlohmann::json::parse(file);
            }
        }

        if (!root.contains("opener")) {
            root["opener"] = nlohmann::json::object();
        }
        if (!root["opener"].contains("openers")) {
            root["opener"]["openers"] = nlohmann::json::object();
        }
        if (!root["opener"].contains("rules")) {
            root["opener"]["rules"] = nlohmann::json::array();
        }

        // Add opener
        if (!state.opener_config_name.empty() && !state.opener_config_run.empty()) {
            nlohmann::json op;
            op["run"] = state.opener_config_run;
            op["block"] = !state.opener_config_is_orphan;
            op["orphan"] = state.opener_config_is_orphan;
            op["desc"] = state.opener_config_desc.empty()
                ? state.opener_config_name : state.opener_config_desc;

            root["opener"]["openers"][state.opener_config_name] = nlohmann::json::array();
            root["opener"]["openers"][state.opener_config_name].push_back(op);
        }

        // Add rule
        if (!state.opener_config_rule_name.empty() && !state.opener_config_rule_use.empty()) {
            nlohmann::json rule;
            rule["name"] = state.opener_config_rule_name;

            std::vector<std::string> use_list;
            std::istringstream ss(state.opener_config_rule_use);
            std::string token;
            while (std::getline(ss, token, ',')) {
                size_t start = token.find_first_not_of(" \t");
                size_t end = token.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    use_list.push_back(token.substr(start, end - start + 1));
                }
            }
            rule["use"] = use_list;

            root["opener"]["rules"].push_back(rule);
        }

        std::ofstream out(config_path);
        if (out.is_open()) {
            out << root.dump(2) << std::endl;
        }

        // Reload opener config
        OpenerConfig opener_config;
        if (root["opener"].contains("openers")) {
            for (auto& [name, arr] : root["opener"]["openers"].items()) {
                std::vector<Opener> openers_list;
                if (arr.is_array()) {
                    for (const auto& item : arr) {
                        Opener op;
                        if (item.contains("run"))     op.run = item["run"].get<std::string>();
                        if (item.contains("block"))   op.block = item["block"].get<bool>();
                        if (item.contains("orphan"))  op.orphan = item["orphan"].get<bool>();
                        if (item.contains("desc"))    op.desc = item["desc"].get<std::string>();
                        openers_list.push_back(op);
                    }
                }
                opener_config.openers[name] = openers_list;
            }
        }
        if (root["opener"].contains("rules")) {
            for (const auto& rule : root["opener"]["rules"]) {
                OpenRule r;
                if (rule.contains("name")) r.name = rule["name"].get<std::string>();
                if (rule.contains("use") && rule["use"].is_array()) {
                    for (const auto& u : rule["use"]) {
                        r.use.push_back(u.get<std::string>());
                    }
                }
                opener_config.rules.push_back(r);
            }
        }
        OpenerManager::Instance().LoadConfig(opener_config);

    } catch (const std::exception& e) {
        std::cerr << "Failed to save opener config: " << e.what() << std::endl;
    }
}

bool HandleOpenerConfigEvent(MainState& state, const Event& event) {
    if (event == Event::Escape) {
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (state.opener_config_field > 0) {
            state.opener_config_field--;
        }
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (state.opener_config_field < 5) {
            state.opener_config_field++;
        }
        return true;
    }

    if (event == Event::Tab) {
        if (state.opener_config_field == 3) {
            state.opener_config_is_orphan = !state.opener_config_is_orphan;
        }
        return true;
    }

    if (event == Event::Return) {
        SaveOpenerConfig(state);
        StatusMessage::Show("Opener config saved to ~/.config/ftb/ftb.json");
        state.active_panel = ActivePanel::None;
        return true;
    }

    if (event == Event::Backspace) {
        switch (state.opener_config_field) {
            case 0: if (!state.opener_config_name.empty()) state.opener_config_name.pop_back(); break;
            case 1: if (!state.opener_config_run.empty()) state.opener_config_run.pop_back(); break;
            case 2: if (!state.opener_config_desc.empty()) state.opener_config_desc.pop_back(); break;
            case 4: if (!state.opener_config_rule_name.empty()) state.opener_config_rule_name.pop_back(); break;
            case 5: if (!state.opener_config_rule_use.empty()) state.opener_config_rule_use.pop_back(); break;
        }
        return true;
    }

    if (event.is_character()) {
        char c = event.character()[0];
        if (c != ':') {
            switch (state.opener_config_field) {
                case 0: state.opener_config_name += c; break;
                case 1: state.opener_config_run += c; break;
                case 2: state.opener_config_desc += c; break;
                case 4: state.opener_config_rule_name += c; break;
                case 5: state.opener_config_rule_use += c; break;
            }
        }
        return true;
    }

    return true;
}

}} // namespace FTB::UI
