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
#ifdef FTB_ENABLE_SSH
    m["ssh"]        = PanelCommand::SSH;
#endif
    return m;
}

KeyBindings& KeyBindings::GetInstance() {
    static KeyBindings instance;
    return instance;
}

bool KeyBindings::HandleEvent(const ftxui::Event& event) {
    // Ctrl+B: 进入前缀模式
    if (event == ftxui::Event::CtrlB) {
        EnterPrefixMode();
        return true;
    }

    // 如果已在前缀模式，处理命令输入
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

    // 左/右箭头：移动光标
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

    // Home / End：移动到开头/末尾
    if (event == ftxui::Event::Home) {
        command_cursor_ = 0;
        return true;
    }

    if (event == ftxui::Event::End) {
        command_cursor_ = command_input_.size();
        return true;
    }

    // Tab: 补全命令
    if (event == ftxui::Event::Tab) {
        if (!command_input_.empty()) {
            std::string lower_input = command_input_;
            std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

            // 查找所有匹配的命令
            std::vector<std::string> matches;
            for (const auto& [name, cmd] : command_map_) {
                if (name.size() >= lower_input.size() &&
                    name.substr(0, lower_input.size()) == lower_input) {
                    matches.push_back(name);
                }
            }

            // 如果有匹配，补全到最长公共前缀
            if (!matches.empty()) {
                if (matches.size() == 1) {
                    command_input_ = matches[0];
                } else {
                    // 找最长公共前缀
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

    // 普通字符输入 (忽略冒号，因为前缀已经隐含了冒号)
    if (event.is_character()) {
        char c = event.character()[0];
        if (c != ':') {
            command_input_.insert(command_cursor_, 1, c);
            command_cursor_++;
        }
        return true;
    }

    return true; // 在前缀模式下消费所有事件
}

void KeyBindings::EnterPrefixMode() {
    prefix_mode_ = true;
    command_input_.clear();
    command_cursor_ = 0;
    current_command_ = PanelCommand::None;
}

void KeyBindings::ExitPrefixMode() {
    prefix_mode_ = false;
    command_input_.clear();
    command_cursor_ = 0;
    current_command_ = PanelCommand::None;
}

KeyBindings::PanelCommand KeyBindings::ExecuteCommand() {
    // 转为小写
    std::string lower_input = command_input_;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    auto it = command_map_.find(lower_input);
    if (it != command_map_.end()) {
        current_command_ = it->second;
        return current_command_;
    }

    current_command_ = PanelCommand::None;
    return PanelCommand::None;
}

std::string KeyBindings::GetCommandHint() const {
    if (!prefix_mode_) return "";

    // 根据当前输入尝试补全
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
#ifdef FTB_ENABLE_SSH
    list.push_back({"ssh",               "SSH connection"});
#endif
    return list;
}

void KeyBindings::RegisterCallback(PanelCommand cmd, CommandCallback callback) {
    callbacks_[cmd] = std::move(callback);
}

} // namespace FTB
