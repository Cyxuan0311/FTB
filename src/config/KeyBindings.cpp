#include "config/KeyBindings.hpp"
#include <algorithm>
#include <iostream>

namespace FTB {

const std::map<std::string, KeyBindings::PanelCommand> KeyBindings::command_map_ = KeyBindings::InitCommandMap();

std::map<std::string, KeyBindings::PanelCommand> KeyBindings::InitCommandMap() {
    std::map<std::string, KeyBindings::PanelCommand> m;
    m["calendar"]   = PanelCommand::Calendar;
    m["cal"]        = PanelCommand::Calendar;
    m["theme"]      = PanelCommand::Theme;
    m["th"]         = PanelCommand::Theme;
    m["layout"]     = PanelCommand::Layout;
    m["lo"]         = PanelCommand::Layout;
    m["jump"]       = PanelCommand::JumpDirectory;
    m["j"]          = PanelCommand::JumpDirectory;
    m["rename"]     = PanelCommand::Rename;
    m["rn"]         = PanelCommand::Rename;
    m["newfile"]    = PanelCommand::NewFile;
    m["nf"]         = PanelCommand::NewFile;
    m["newfolder"]  = PanelCommand::NewFolder;
    m["nd"]         = PanelCommand::NewFolder;
    m["preview"]    = PanelCommand::FilePreview;
    m["p"]          = PanelCommand::FilePreview;
    m["details"]    = PanelCommand::FolderDetails;
    m["d"]          = PanelCommand::FolderDetails;
    m["vim"]        = PanelCommand::VimEdit;
    m["v"]          = PanelCommand::VimEdit;
    m["editor"]     = PanelCommand::VimEdit;
    m["e"]          = PanelCommand::VimEdit;
    m["help"]       = PanelCommand::Help;
    m["h"]          = PanelCommand::Help;
    m["search"]     = PanelCommand::Search;
    m["s"]          = PanelCommand::Search;
    m["sort"]       = PanelCommand::Sort;
    m["so"]         = PanelCommand::Sort;
    m["plugin"]     = PanelCommand::Plugin;
    m["pl"]         = PanelCommand::Plugin;
    m["uistyle"]    = PanelCommand::UIStyle;
    m["ui"]         = PanelCommand::UIStyle;
    m["statusstyle"] = PanelCommand::StatusBarStyle;
    m["ss"]         = PanelCommand::StatusBarStyle;
    m["fdfind"]     = PanelCommand::FuzzyFinder;
    m["fd"]         = PanelCommand::FuzzyFinder;
    m["clipboard"]  = PanelCommand::ShowClipboard;
    m["cb"]         = PanelCommand::ShowClipboard;
    m["clr"]        = PanelCommand::ClearClipboard;
    m["cc"]         = PanelCommand::ClearClipboard;
    m["mdsource"]   = PanelCommand::MDToggleSource;
    m["mds"]        = PanelCommand::MDToggleSource;
    m["xlsx"]       = PanelCommand::XLSToggleSource;
    m["xls"]        = PanelCommand::XLSToggleSource;
    m["media"]      = PanelCommand::MediaToggleSource;
    m["video"]      = PanelCommand::MediaToggleSource;
    m["mp4"]        = PanelCommand::MediaToggleSource;
    m["gif"]        = PanelCommand::MediaToggleSource;
    m["timg"]       = PanelCommand::MediaPlay;
    m["play"]       = PanelCommand::MediaPlay;
    m["pdf"]        = PanelCommand::PdfToggleSource;
    m["hygg"]       = PanelCommand::PdfToggleSource;
    m["doc"]        = PanelCommand::DocToggleSource;
    m["docx"]       = PanelCommand::DocToggleSource;
    m["pandoc"]     = PanelCommand::DocToggleSource;
    m["aud"]        = PanelCommand::AudioToggleEnabled;
    m["audio"]      = PanelCommand::AudioToggleEnabled;
    m["eyed3"]      = PanelCommand::AudioToggleEnabled;
    m["hex"]        = PanelCommand::HexToggleEnabled;
    m["xxd"]        = PanelCommand::HexToggleEnabled;
    m["gh"]         = PanelCommand::GoHome;
    m["gd"]         = PanelCommand::GoDownloads;
    m["gc"]         = PanelCommand::GoConfig;
    m["newtab"]     = PanelCommand::NewTab;
    m["nt"]         = PanelCommand::NewTab;
    m["closetab"]   = PanelCommand::CloseTab;
    m["ct"]         = PanelCommand::CloseTab;
    m["nexttab"]    = PanelCommand::NextTab;
    m["nx"]         = PanelCommand::NextTab;
    m["prevtab"]    = PanelCommand::PrevTab;
    m["pv"]         = PanelCommand::PrevTab;
    m["open"]       = PanelCommand::OpenPicker;
    m["op"]         = PanelCommand::OpenPicker;
    m["openwith"]   = PanelCommand::OpenManual;
    m["ow"]         = PanelCommand::OpenManual;
    m["opencfg"]    = PanelCommand::OpenConfig;
    m["oc"]         = PanelCommand::OpenConfig;
    m["tasks"]      = PanelCommand::Tasks;
    m["task"]       = PanelCommand::Tasks;
    m["ts"]         = PanelCommand::Tasks;
    m["batchrename"] = PanelCommand::BatchRename;
    m["br"]         = PanelCommand::BatchRename;
    m["z"]          = PanelCommand::QuitWithCwd;
    m["exit"]       = PanelCommand::QuitWithCwd;
    m["quit"]       = PanelCommand::QuitWithCwd;
    m["pcmd"]       = PanelCommand::PluginCommand;
    m["plugincmd"]  = PanelCommand::PluginCommand;
    m["pc"]         = PanelCommand::PluginCommand;
    m["toggleprotocol"] = PanelCommand::ToggleProtocol;
    m["protocol"]       = PanelCommand::ToggleProtocol;
    m["imgproto"]       = PanelCommand::ToggleProtocol;
#ifdef FTB_ENABLE_SSH
    m["ssh"]        = PanelCommand::SSH;
#endif
#ifdef FTB_ENABLE_AI
    m["ai"]         = PanelCommand::AI;
    m["ask"]        = PanelCommand::AI;
    m["aicfg"]      = PanelCommand::AIConfig;
    m["ai-config"]  = PanelCommand::AIConfig;
#endif
    return m;
}

const std::map<std::string, ftxui::Event>& KeyBindings::GetKeyEventMap() {
    static const std::map<std::string, ftxui::Event> m = [] {
        std::map<std::string, ftxui::Event> map;
        map["CtrlA"] = ftxui::Event::CtrlA;
        map["CtrlB"] = ftxui::Event::CtrlB;
        map["CtrlC"] = ftxui::Event::CtrlC;
        map["CtrlD"] = ftxui::Event::CtrlD;
        map["CtrlE"] = ftxui::Event::CtrlE;
        map["CtrlF"] = ftxui::Event::CtrlF;
        map["CtrlG"] = ftxui::Event::CtrlG;
        map["CtrlH"] = ftxui::Event::CtrlH;
        map["CtrlI"] = ftxui::Event::CtrlI;
        map["CtrlJ"] = ftxui::Event::CtrlJ;
        map["CtrlK"] = ftxui::Event::CtrlK;
        map["CtrlL"] = ftxui::Event::CtrlL;
        map["CtrlM"] = ftxui::Event::CtrlM;
        map["CtrlN"] = ftxui::Event::CtrlN;
        map["CtrlO"] = ftxui::Event::CtrlO;
        map["CtrlP"] = ftxui::Event::CtrlP;
        map["CtrlQ"] = ftxui::Event::CtrlQ;
        map["CtrlR"] = ftxui::Event::CtrlR;
        map["CtrlS"] = ftxui::Event::CtrlS;
        map["CtrlT"] = ftxui::Event::CtrlT;
        map["CtrlU"] = ftxui::Event::CtrlU;
        map["CtrlV"] = ftxui::Event::CtrlV;
        map["CtrlW"] = ftxui::Event::CtrlW;
        map["CtrlX"] = ftxui::Event::CtrlX;
        map["CtrlY"] = ftxui::Event::CtrlY;
        map["CtrlZ"] = ftxui::Event::CtrlZ;

        map["AltA"] = ftxui::Event::AltA;
        map["AltB"] = ftxui::Event::AltB;
        map["AltC"] = ftxui::Event::AltC;
        map["AltD"] = ftxui::Event::AltD;
        map["AltE"] = ftxui::Event::AltE;
        map["AltF"] = ftxui::Event::AltF;
        map["AltG"] = ftxui::Event::AltG;
        map["AltH"] = ftxui::Event::AltH;
        map["AltI"] = ftxui::Event::AltI;
        map["AltJ"] = ftxui::Event::AltJ;
        map["AltK"] = ftxui::Event::AltK;
        map["AltL"] = ftxui::Event::AltL;
        map["AltM"] = ftxui::Event::AltM;
        map["AltN"] = ftxui::Event::AltN;
        map["AltO"] = ftxui::Event::AltO;
        map["AltP"] = ftxui::Event::AltP;
        map["AltQ"] = ftxui::Event::AltQ;
        map["AltR"] = ftxui::Event::AltR;
        map["AltS"] = ftxui::Event::AltS;
        map["AltT"] = ftxui::Event::AltT;
        map["AltU"] = ftxui::Event::AltU;
        map["AltV"] = ftxui::Event::AltV;
        map["AltW"] = ftxui::Event::AltW;
        map["AltX"] = ftxui::Event::AltX;
        map["AltY"] = ftxui::Event::AltY;
        map["AltZ"] = ftxui::Event::AltZ;

        map["F1"]  = ftxui::Event::F1;
        map["F2"]  = ftxui::Event::F2;
        map["F3"]  = ftxui::Event::F3;
        map["F4"]  = ftxui::Event::F4;
        map["F5"]  = ftxui::Event::F5;
        map["F6"]  = ftxui::Event::F6;
        map["F7"]  = ftxui::Event::F7;
        map["F8"]  = ftxui::Event::F8;
        map["F9"]  = ftxui::Event::F9;
        map["F10"] = ftxui::Event::F10;
        map["F11"] = ftxui::Event::F11;
        map["F12"] = ftxui::Event::F12;
        return map;
    }();
    return m;
}

KeyBindings& KeyBindings::GetInstance() {
    static KeyBindings instance;
    return instance;
}

bool KeyBindings::HandleEvent(const ftxui::Event& event) {
    if (event == prefix_event_) {
        EnterPrefixMode();
        return true;
    }

    if (prefix_mode_) {
        bool result = HandlePrefixInput(event);
        return result;
    }

    return false;
}

bool KeyBindings::HandlePrefixInput(const ftxui::Event& event) {
    if (event == ftxui::Event::Escape) {
        ExitPrefixMode();
        return true;
    }

    if (event == ftxui::Event::Return) {
        PanelCommand cmd = ExecuteCommand();
        if (cmd != PanelCommand::None) {
            auto it = callbacks_.find(cmd);
            if (it != callbacks_.end()) {
                it->second();
            }
        }
        ExitPrefixMode();
        return true;
    }

    if (event == ftxui::Event::ArrowLeft) {
        if (command_cursor_ > 0) {
            command_cursor_--;
        }
        return true;
    }

    if (event == ftxui::Event::ArrowRight) {
        if (command_cursor_ < command_input_.size()) {
            command_cursor_++;
        }
        return true;
    }

    if (event == ftxui::Event::Home) {
        command_cursor_ = 0;
        return true;
    }

    if (event == ftxui::Event::End) {
        command_cursor_ = command_input_.size();
        return true;
    }

    if (event == ftxui::Event::Tab) {
        if (!command_input_.empty()) {
            std::string lower_input = command_input_;
            std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

            std::vector<std::string> matches;
            for (const auto& [name, cmd] : command_map_) {
                if (name.size() >= lower_input.size() &&
                    name.substr(0, lower_input.size()) == lower_input) {
                    matches.push_back(name);
                }
            }

            if (matches.empty()) {
                auto sp = lower_input.find(' ');
                if (sp != std::string::npos) {
                    std::string prefix = lower_input.substr(0, sp);
                    auto map_it = command_map_.find(prefix);
                    if (map_it != command_map_.end() && map_it->second == PanelCommand::PluginCommand) {
                        std::string payload = command_input_.substr(sp + 1);
                        if (plugin_completer_) {
                            auto plugin_matches = plugin_completer_(payload);
                            if (!plugin_matches.empty()) {
                                if (plugin_matches.size() == 1) {
                                    command_input_ = prefix + " " + plugin_matches[0];
                                } else {
                                    std::string lcp = plugin_matches[0];
                                    for (size_t i = 1; i < plugin_matches.size(); ++i) {
                                        size_t j = 0;
                                        while (j < lcp.size() && j < plugin_matches[i].size() && lcp[j] == plugin_matches[i][j]) j++;
                                        lcp = lcp.substr(0, j);
                                    }
                                    command_input_ = prefix + " " + lcp;
                                }
                                command_cursor_ = command_input_.size();
                                return true;
                            }
                        }
                    }
                }
            }

            if (!matches.empty()) {
                if (matches.size() == 1) {
                    command_input_ = matches[0];
                } else {
                    std::string lcp = matches[0];
                    for (size_t i = 1; i < matches.size(); ++i) {
                        size_t j = 0;
                        while (j < lcp.size() && j < matches[i].size() && lcp[j] == matches[i][j]) j++;
                        lcp = lcp.substr(0, j);
                    }
                    command_input_ = lcp;
                }
            }
        }
        command_cursor_ = command_input_.size();
        return true;
    }

    if (event == ftxui::Event::Backspace) {
        if (!command_input_.empty() && command_cursor_ > 0) {
            command_input_.erase(command_cursor_ - 1, 1);
            command_cursor_--;
        } else if (command_input_.empty()) {
            ExitPrefixMode();
        }
        return true;
    }

    if (event == ftxui::Event::Delete) {
        if (command_cursor_ < command_input_.size()) {
            command_input_.erase(command_cursor_, 1);
        }
        return true;
    }

    if (event.is_character()) {
        char c = event.character()[0];
        if (c != ':') {
            command_input_.insert(command_cursor_, 1, c);
            command_cursor_++;
        }
        return true;
    }

    return true;
}

void KeyBindings::SetPrefixKey(const std::string& key_name) {
    const auto& event_map = GetKeyEventMap();
    auto it = event_map.find(key_name);
    if (it != event_map.end()) {
        prefix_event_ = it->second;
        prefix_key_name_ = key_name;
    } else {
        prefix_event_ = ftxui::Event::CtrlB;
        prefix_key_name_ = "CtrlB";
    }
}

static const std::map<std::string, std::string> InitConflictMap() {
    std::map<std::string, std::string> m;
    m["CtrlR"] = "Reload config";
    m["CtrlC"] = "Quit FTB";
    m["CtrlD"] = "Delete confirm";
    m["CtrlM"] = "Same as Enter (terminal-level)";
    m["CtrlI"] = "Same as Tab (terminal-level)";
    m["CtrlH"] = "Same as Backspace (terminal-level)";

    m["AltJ"]  = "Preview scroll down";
    m["AltK"]  = "Preview scroll up";
    m["AltH"]  = "Preview scroll left";
    m["AltL"]  = "Preview scroll right";
    return m;
}

std::vector<PrefixKeyInfo> KeyBindings::GetAvailablePrefixKeys() const {
    static const auto conflict_map = InitConflictMap();
    std::vector<PrefixKeyInfo> result;
    for (const auto& [name, event] : GetKeyEventMap()) {
        PrefixKeyInfo info;
        info.name = name;
        info.is_current = (name == prefix_key_name_);
        auto cit = conflict_map.find(name);
        if (cit != conflict_map.end()) {
            info.is_safe = false;
            info.conflict_note = cit->second;
        } else {
            info.is_safe = true;
        }
        if (name.size() >= 4 && name.substr(0, 3) == "Alt") {
            info.display_name = "Alt+" + name.substr(3);
        } else if (name.size() >= 4 && name.substr(0, 4) == "Ctrl") {
            info.display_name = "Ctrl+" + name.substr(4);
        } else {
            info.display_name = name;
        }
        result.push_back(info);
    }
    return result;
}

void KeyBindings::EnterPrefixMode() {
    prefix_mode_ = true;
    command_input_.clear();
    command_cursor_ = 0;
    current_command_ = PanelCommand::None;
    command_payload_.clear();
}

void KeyBindings::ExitPrefixMode() {
    prefix_mode_ = false;
    command_input_.clear();
    command_cursor_ = 0;
    current_command_ = PanelCommand::None;
    command_payload_.clear();
}

KeyBindings::PanelCommand KeyBindings::ExecuteCommand() {
    std::string lower_input = command_input_;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    auto it = command_map_.find(lower_input);
    if (it != command_map_.end()) {
        current_command_ = it->second;
        command_payload_.clear();
        return current_command_;
    }

    auto space_pos = lower_input.find(' ');
    if (space_pos != std::string::npos) {
        std::string prefix = lower_input.substr(0, space_pos);
        it = command_map_.find(prefix);
        if (it != command_map_.end()) {
            current_command_ = it->second;
            command_payload_ = command_input_.substr(space_pos + 1);
            return current_command_;
        }
    }

    current_command_ = PanelCommand::None;
    command_payload_.clear();
    return PanelCommand::None;
}

std::string KeyBindings::GetCommandHint() const {
    if (!prefix_mode_) return "";

    if (!command_input_.empty()) {
        std::string lower_input = command_input_;
        std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

        std::vector<std::string> matches;
        for (const auto& [name, cmd] : command_map_) {
            if (name.size() >= lower_input.size() &&
                name.substr(0, lower_input.size()) == lower_input) {
                matches.push_back(name);
            }
        }

        if (matches.empty()) {
            auto sp = lower_input.find(' ');
            if (sp != std::string::npos) {
                std::string prefix = lower_input.substr(0, sp);
                auto map_it = command_map_.find(prefix);
                if (map_it != command_map_.end() && map_it->second == PanelCommand::PluginCommand) {
                    std::string payload = command_input_.substr(sp + 1);
                    if (plugin_completer_) {
                        auto plugin_matches = plugin_completer_(payload);
                        if (plugin_matches.size() == 1) {
                            return plugin_matches[0].substr(payload.size());
                        } else if (plugin_matches.size() > 1) {
                            std::string hint;
                            for (size_t i = 0; i < plugin_matches.size(); ++i) {
                                if (i > 0) hint += " ";
                                hint += plugin_matches[i];
                            }
                            return " [" + hint + "]";
                        }
                    }
                }
            }
        }

        if (matches.size() == 1) {
            return matches[0].substr(lower_input.size());
        } else if (matches.size() > 1) {
            std::string hint;
            for (size_t i = 0; i < matches.size(); ++i) {
                if (i > 0) hint += " ";
                hint += matches[i];
            }
            return " [" + hint + "]";
        }
    }

    return "";
}

std::vector<std::pair<std::string, std::string>> KeyBindings::GetCommandList() const {
    std::vector<std::pair<std::string, std::string>> list;
    list.push_back({"calendar / cal",    "Calendar panel"});
    list.push_back({"theme / th",        "Theme switcher"});
    list.push_back({"jump / j",          "Jump to directory"});
    list.push_back({"fdfind / fd",       "Fuzzy find files"});
    list.push_back({"rename / rn",       "Rename item"});
    list.push_back({"batchrename / br",  "Batch rename with regex"});
    list.push_back({"newfile / nf",      "Create new file"});
    list.push_back({"newfolder / nd",    "Create new folder"});
    list.push_back({"preview / p",       "File preview"});
    list.push_back({"details / d",       "Folder details"});
    list.push_back({"vim / v / editor / e", "Open editor"});
    list.push_back({"search / s",        "Search files"});
    list.push_back({"help / h",          "Help panel"});
    list.push_back({"layout / lo",       "Layout settings"});
    list.push_back({"uistyle / ui",      "UI style settings"});
    list.push_back({"statusstyle / ss",  "Status bar style"});
    list.push_back({"sort / so",         "Sort mode settings"});
    list.push_back({"plugin / pl",       "Plugin manager"});
    list.push_back({"tasks / task / ts",    "Show task manager"});
    list.push_back({"clipboard / cb",       "Show clipboard details"});
    list.push_back({"clr / cc",             "Clear clipboard"});
    list.push_back({"mdsource / mds",            "Toggle MD preview source (glow)"});
    list.push_back({"xlsx / xls",                "Toggle XLSX preview (xleak)"});
    list.push_back({"media / video / mp4 / gif", "Toggle media preview (timg)"});
    list.push_back({"timg / play",               "Play media fullscreen (timg)"});
    list.push_back({"aud / audio / eyed3",       "Toggle audio preview (eyeD3)"});
    list.push_back({"pdf / hygg",                "Toggle PDF preview (hygg)"});
    list.push_back({"doc / docx / pandoc",       "Toggle DOC/DOCX preview (pandoc, catdoc)"});
    list.push_back({"hex / xxd",                 "Toggle hex preview (xxd)"});
    list.push_back({"gh",                        "Go to home directory"});
    list.push_back({"gd",                   "Go to downloads directory"});
    list.push_back({"gc",                   "Go to config directory"});
    list.push_back({"newtab / nt",          "Create new tab"});
    list.push_back({"closetab / ct",        "Close current tab"});
    list.push_back({"nexttab / nx",         "Next tab"});
    list.push_back({"prevtab / pv",         "Previous tab"});
    list.push_back({"open / op",           "Open with picker"});
    list.push_back({"openwith / ow",       "Manual open with"});
    list.push_back({"opencfg / oc",        "Configure openers"});
    list.push_back({"z / exit / quit",     "Quit and change shell directory"});
    list.push_back({"pcmd / pc / plugincmd", "Execute plugin command"});
    list.push_back({"toggleprotocol / protocol / imgproto", "Toggle terminal image protocol (Kitty/iTerm2/Sixel)"});
#ifdef FTB_ENABLE_SSH
    list.push_back({"ssh",               "SSH connection"});
#endif
#ifdef FTB_ENABLE_AI
    list.push_back({"ai / ask",          "AI assistant panel"});
    list.push_back({"aicfg / ai-config", "AI configuration"});
#endif
    return list;
}

void KeyBindings::RegisterCallback(PanelCommand cmd, CommandCallback callback) {
    callbacks_[cmd] = std::move(callback);
}

} // namespace FTB
