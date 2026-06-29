#include "core/MainUI.hpp"
#ifdef FTB_ENABLE_AI
#include "ai/AIAgent.hpp"
#include "ai/SessionManager.hpp"
#endif

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <ftxui/dom/elements.hpp>

#include "utils/StatusMessage.hpp"
#include "preview/MarkdownPreview.hpp"
#include "preview/SpreadsheetPreview.hpp"
#include "preview/MediaPreview.hpp"
#include "preview/AudioPreview.hpp"
#include "preview/PdfPreview.hpp"
#include "preview/DocPreview.hpp"
#include "preview/HexPreview.hpp"
#include "browser/AsyncFileManager.hpp"
#include "config/ConfigManager.hpp"
#include "browser/ClipboardManager.hpp"
#include "browser/FileManager.hpp"
#include "ops/OpenerManager.hpp"
#include "dialog/OpenerPickerDialog.hpp"
#include "dialog/OpenerInputDialog.hpp"
#include "dialog/OpenerConfigDialog.hpp"
#include "editor/NanoEditor.hpp"
#include "config/ObjectPool.hpp"
#include "config/KeyBindings.hpp"
#include "config/ThemeManager.hpp"
#ifdef FTB_ENABLE_PLUGINS
#include "ops/PluginManager.hpp"
#endif

// UI 面板
#include "dialog/HelpDialog.hpp"
#include "dialog/ThemeDialog.hpp"
#include "dialog/LayoutPanel.hpp"
#include "dialog/RenameDialog.hpp"
#include "dialog/NewFileDialog.hpp"
#include "dialog/NewFolderDialog.hpp"
#include "dialog/FilePreviewDialog.hpp"
#include "dialog/FolderDetailsDialog.hpp"
#include "dialog/JumpDirectoryDialog.hpp"
#include "dialog/CalendarPanel.hpp"
#include "dialog/FuzzyFinderDialog.hpp"
#include "dialog/DeleteDialog.hpp"
#include "dialog/PluginDialog.hpp"
#include "dialog/UIStylePanel.hpp"
#include "dialog/SortDialog.hpp"
#include "dialog/NewTabDialog.hpp"
#include "dialog/ClipboardDialog.hpp"
#include "dialog/TaskPanelDialog.hpp"
#include "dialog/BatchRenameDialog.hpp"
#ifdef FTB_ENABLE_SSH
#include "dialog/SSHDialog.hpp"
#endif
#ifdef FTB_ENABLE_AI
#include "dialog/AIDialog.hpp"
#include "dialog/AIConfigDialog.hpp"
#endif

namespace fs = std::filesystem;

namespace FTB {

using namespace ftxui;

Element BuildPanelModal(MainState& state) {
    auto term_dim = Terminal::Size();
    int tw = term_dim.dimx;
    int th = term_dim.dimy;

    switch (state.active_panel) {
    case ActivePanel::Calendar:       return UI::RenderCalendarPanel(state, tw, th);
    case ActivePanel::Help:           return UI::RenderHelpPanel(state, tw, th);
    case ActivePanel::Rename:         return UI::RenderRenamePanel(state, tw, th);
    case ActivePanel::NewFile:        return UI::RenderNewFilePanel(state, tw, th);
    case ActivePanel::NewFolder:      return UI::RenderNewFolderPanel(state, tw, th);
    case ActivePanel::DeleteConfirm:  return UI::RenderDeleteConfirmPanel(state, tw, th);
    case ActivePanel::Theme:          return UI::RenderThemePanel(state, tw, th);
    case ActivePanel::Layout:         return UI::RenderLayoutPanel(state, tw, th);
    case ActivePanel::FilePreview:    return UI::RenderFilePreviewPanel(state, tw, th);
    case ActivePanel::FolderDetails:  return UI::RenderFolderDetailsPanel(state, tw, th);
    case ActivePanel::JumpDirectory:  return UI::RenderJumpDirectoryPanel(state, tw, th);
    case ActivePanel::FuzzyFinder:    return UI::RenderFuzzyFinderPanel(state, tw, th);
    case ActivePanel::UIStyle:        return UI::RenderUIStylePanel(state, tw, th);
    case ActivePanel::StatusBarStyle: return UI::RenderStatusBarStylePanel(state, tw, th);
    case ActivePanel::Sort:           return UI::RenderSortPanel(state, tw, th);
    case ActivePanel::NewTab:          return UI::RenderNewTabPanel(state, tw, th);
    case ActivePanel::ShowClipboard:   return UI::RenderClipboardPanel(state, tw, th);
    case ActivePanel::TaskPanel:       return UI::RenderTaskPanel(state, tw, th);
    case ActivePanel::OpenerPicker:    return UI::RenderOpenerPickerPanel(state, tw, th);
    case ActivePanel::OpenerInput:     return UI::RenderOpenerInputPanel(state, tw, th);
    case ActivePanel::OpenerConfig:    return UI::RenderOpenerConfigPanel(state, tw, th);
    case ActivePanel::BatchRename:     return UI::RenderBatchRenamePanel(state, tw, th);
#ifdef FTB_ENABLE_PLUGINS
    case ActivePanel::Plugin:         return UI::RenderPluginPanel(state, tw, th);
#endif
#ifdef FTB_ENABLE_SSH
    case ActivePanel::SSH:            return UI::RenderSSHPanel(state, tw, th);
#endif
#ifdef FTB_ENABLE_AI
    case ActivePanel::AI:             return UI::RenderAIPanel(state, tw, th);
    case ActivePanel::AIConfig:       return UI::RenderAIConfigPanel(state, tw, th);
#endif
    default:                          return text("");
    }
}

bool HandlePanelEvent(MainState& state, const Event& event) {
    if (state.active_panel == ActivePanel::None) return false;

    switch (state.active_panel) {
    case ActivePanel::UIStyle:         return UI::HandleUIStyleEvent(state, event);
    case ActivePanel::StatusBarStyle:  return UI::HandleStatusBarStyleEvent(state, event);
    case ActivePanel::Sort:            return UI::HandleSortEvent(state, event);
    case ActivePanel::NewTab:           return UI::HandleNewTabEvent(state, event);
    case ActivePanel::ShowClipboard:    return UI::HandleClipboardEvent(state, event);
    case ActivePanel::TaskPanel:        return UI::HandleTaskPanelEvent(state, event);
    case ActivePanel::OpenerPicker:    return UI::HandleOpenerPickerEvent(state, event);
    case ActivePanel::OpenerInput:     return UI::HandleOpenerInputEvent(state, event);
    case ActivePanel::OpenerConfig:    return UI::HandleOpenerConfigEvent(state, event);
    case ActivePanel::BatchRename:     return UI::HandleBatchRenameEvent(state, event);
    default: break;
    }

    if (event == Event::Escape) {
        if (state.active_panel == ActivePanel::Theme) {
            auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
            FTB::ThemeManager::GetInstance()->ApplyTheme(cfg.theme.name);
        }
        state.active_panel = ActivePanel::None;
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_selected = 0;
        state.panel_suggestion.clear();
        return true;
    }

    switch (state.active_panel) {
    case ActivePanel::Calendar:       return UI::HandleCalendarEvent(state, event);
    case ActivePanel::Help:           return UI::HandleHelpEvent(state, event);
    case ActivePanel::Theme:          return UI::HandleThemeEvent(state, event);
    case ActivePanel::Layout:         return UI::HandleLayoutEvent(state, event);
    case ActivePanel::Rename:         return UI::HandleRenameEvent(state, event);
    case ActivePanel::NewFile:        return UI::HandleNewFileEvent(state, event);
    case ActivePanel::NewFolder:      return UI::HandleNewFolderEvent(state, event);
    case ActivePanel::DeleteConfirm:  return UI::HandleDeleteConfirmEvent(state, event);
    case ActivePanel::FilePreview:    return UI::HandleFilePreviewEvent(state, event);
    case ActivePanel::FolderDetails:  return UI::HandleFolderDetailsEvent(state, event);
    case ActivePanel::JumpDirectory:  return UI::HandleJumpDirectoryEvent(state, event);
    case ActivePanel::FuzzyFinder:    return UI::HandleFuzzyFinderEvent(state, event);
#ifdef FTB_ENABLE_PLUGINS
    case ActivePanel::Plugin:         return UI::HandlePluginEvent(state, event);
#endif
#ifdef FTB_ENABLE_AI
    case ActivePanel::AIConfig:        return UI::HandleAIConfigEvent(state, event);
#endif
#ifdef FTB_ENABLE_SSH
    case ActivePanel::SSH:            return UI::HandleSSHEvent(state, event);
#endif
#ifdef FTB_ENABLE_AI
    case ActivePanel::AI:             return UI::HandleAIEvent(state, event);
#endif
    default:                          return true;
    }
}

void OpenEditorForFile(MainState& state, const std::string& filePath) {
    if (state.vimEditor != nullptr) {
        FTB::NanoEditorPool::GetInstance().release(std::move(state.vimEditor));
    }
    state.vimEditor = FTB::NanoEditorPool::GetInstance().acquire();
    FTB::GlobalAsyncFileManager::getInstance().asyncReadFileContent(
        filePath, 1, 1000,
        [&state, filePath](std::string fileContent) {
            std::vector<std::string> lines;
            std::istringstream iss(fileContent);
            std::string line;
            while (std::getline(iss, line))
                lines.push_back(line);
            state.vimEditor->SetContent(lines);
            state.vimEditor->SetFilename(filePath);
            state.vimEditor->Enter();
            state.vimEditor->SetOnExit([&state, filePath](const std::vector<std::string>& content) {
                std::string contentStr;
                for (size_t i = 0; i < content.size(); ++i) {
                    contentStr += content[i];
                    if (i < content.size() - 1) contentStr += "\n";
                }
                FileManager::writeFileContent(filePath, contentStr);
                state.vim_mode_active = false;
            });
            state.vim_mode_active = true;
        });
}

namespace {
    const FileManager::DirEntryInfo* FindSelectedEntry(const MainState& state) {
        if (state.selected < 0 || state.selected >= static_cast<int>(state.filteredContents.size()))
            return nullptr;
        const std::string& sel_name = state.filteredContents[state.selected];
        for (const auto& e : state.cached_current_entries) {
            if (e.name == sel_name) return &e;
        }
        return nullptr;
    }
}

void HandlePanelCommand(MainState& state, FTB::KeyBindings::PanelCommand cmd) {
    switch (cmd) {
    case FTB::KeyBindings::PanelCommand::Calendar:
        state.active_panel = ActivePanel::Calendar; break;
    case FTB::KeyBindings::PanelCommand::Help:
        state.active_panel = ActivePanel::Help; break;
    case FTB::KeyBindings::PanelCommand::Theme:
        state.panel_input.clear();
        state.panel_selected = 0;
        state.theme_scroll = 0;
        state.active_panel = ActivePanel::Theme; break;
    case FTB::KeyBindings::PanelCommand::Layout:
        state.active_panel = ActivePanel::Layout; break;
    case FTB::KeyBindings::PanelCommand::Rename:
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            state.panel_input.clear();
            state.panel_message.clear();
            state.active_panel = ActivePanel::Rename;
        }
        break;
    case FTB::KeyBindings::PanelCommand::NewFile:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::NewFile; break;
    case FTB::KeyBindings::PanelCommand::NewFolder:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::NewFolder; break;
    case FTB::KeyBindings::PanelCommand::FilePreview:
        state.active_panel = ActivePanel::FilePreview; break;
    case FTB::KeyBindings::PanelCommand::FolderDetails:
        state.active_panel = ActivePanel::FolderDetails; break;
    case FTB::KeyBindings::PanelCommand::JumpDirectory:
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_selected = 0;
        state.panel_suggestion.clear();
        state.active_panel = ActivePanel::JumpDirectory; break;
    case FTB::KeyBindings::PanelCommand::FuzzyFinder:
        state.panel_input.clear();
        state.panel_selected = 0;
        state.active_panel = ActivePanel::FuzzyFinder; break;
    case FTB::KeyBindings::PanelCommand::Search:
        state.search_mode = true;
        state.searchQuery.clear();
        break;
    case FTB::KeyBindings::PanelCommand::ClearClipboard:
        ClipboardManager::getInstance().clear();
        StatusMessage::Show("Clipboard cleared");
        break;
    case FTB::KeyBindings::PanelCommand::ShowClipboard:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::ShowClipboard;
        break;
    case FTB::KeyBindings::PanelCommand::Tasks:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::TaskPanel;
        break;
    case FTB::KeyBindings::PanelCommand::BatchRename: {
        auto items = [&]() {
            std::vector<std::string> names;
            if (!state.batch_selected.empty()) {
                for (int idx : state.batch_selected) {
                    if (idx >= 0 && idx < static_cast<int>(state.filteredContents.size()))
                        names.push_back(state.filteredContents[idx]);
                }
            } else if (state.selected >= 0 &&
                       state.selected < static_cast<int>(state.filteredContents.size())) {
                names.push_back(state.filteredContents[state.selected]);
            }
            return names;
        }();
        if (items.empty()) {
            StatusMessage::Show("No files selected for batch rename");
        } else {
            state.batchrename_pattern.clear();
            state.batchrename_replacement.clear();
            state.batchrename_field = 0;
            state.batchrename_preview.clear();
            state.panel_message.clear();
            state.active_panel = ActivePanel::BatchRename;
        }
        break;
    }
    case FTB::KeyBindings::PanelCommand::MDToggleSource: {
        FTB::MarkdownPreview::ToggleSourceMode();
        std::string msg = FTB::MarkdownPreview::ShowSource()
            ? "Markdown preview: source mode"
            : "Markdown preview: rendered mode";
        if (auto* entry = FindSelectedEntry(state)) {
            if (!FTB::MarkdownPreview::IsMarkdownFile(entry->name)) {
                msg += " (not a markdown file)";
            }
        }
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::XLSToggleSource: {
        FTB::SpreadsheetPreview::ToggleSourceMode();
        std::string msg = FTB::SpreadsheetPreview::ShowSource()
            ? "XLSX preview: source mode"
            : "XLSX preview: rendered mode";
        if (auto* entry = FindSelectedEntry(state)) {
            if (!FTB::SpreadsheetPreview::IsSpreadsheetFile(entry->name)) {
                msg += " (not a spreadsheet file)";
            }
        }
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::MediaToggleSource: {
        FTB::MediaPreview::ToggleEnabled();
        std::string msg = FTB::MediaPreview::IsEnabled()
            ? "Media preview: enabled"
            : "Media preview: disabled";
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::MediaPlay: {
        if (auto* entry = FindSelectedEntry(state)) {
            std::string fullPath = (fs::path(state.currentPath) / entry->name).string();
            if (FTB::MediaPreview::IsMediaFile(entry->name)) {
                FTB::MediaPreview::PlayFullscreen(fullPath, state.screen);
            } else {
                StatusMessage::Show("Not a media file");
            }
        }
        break;
    }
    case FTB::KeyBindings::PanelCommand::PdfToggleSource: {
        FTB::PdfPreview::ToggleSourceMode();
        std::string msg = FTB::PdfPreview::ShowSource()
            ? "PDF preview: source mode"
            : "PDF preview: rendered mode";
        if (auto* entry = FindSelectedEntry(state)) {
            if (!FTB::PdfPreview::IsPdfFile(entry->name)) {
                msg += " (not a PDF file)";
            }
        }
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::DocToggleSource: {
        FTB::DocPreview::ToggleSourceMode();
        std::string msg = FTB::DocPreview::ShowSource()
            ? "DOC/DOCX preview: source mode"
            : "DOC/DOCX preview: rendered mode";
        if (auto* entry = FindSelectedEntry(state)) {
            if (!FTB::DocPreview::IsDocFile(entry->name)) {
                msg += " (not a DOC/DOCX file)";
            }
        }
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::AudioToggleEnabled: {
        FTB::AudioPreview::ToggleEnabled();
        std::string msg = FTB::AudioPreview::IsEnabled()
            ? "Audio preview: enabled"
            : "Audio preview: disabled";
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::HexToggleEnabled: {
        FTB::HexPreview::ToggleEnabled();
        std::string msg = FTB::HexPreview::IsEnabled()
            ? "Hex preview: enabled"
            : "Hex preview: disabled";
        StatusMessage::Show(msg);
        break;
    }
    case FTB::KeyBindings::PanelCommand::GoHome: {
        const char* home_env = std::getenv("HOME");
        std::string home = home_env ? home_env : "";
        if (!home.empty()) {
            state.directoryHistory.push(state.currentPath);
            state.currentPath = fs::canonical(home).string();
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.selected = 0;
            state.current_page = 0;
            state.batch_selected.clear();
            state.searchQuery.clear();
            StatusMessage::Show("Go to ~");
        }
        break;
    }
    case FTB::KeyBindings::PanelCommand::GoDownloads: {
        const char* home_env = std::getenv("HOME");
        std::string home = home_env ? home_env : "";
        if (!home.empty()) {
            std::string dl = (fs::path(home) / "Downloads").string();
            state.directoryHistory.push(state.currentPath);
            state.currentPath = fs::canonical(dl).string();
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.selected = 0;
            state.current_page = 0;
            state.batch_selected.clear();
            state.searchQuery.clear();
            StatusMessage::Show("Go to ~/Downloads");
        }
        break;
    }
    case FTB::KeyBindings::PanelCommand::GoConfig: {
        const char* home_env = std::getenv("HOME");
        std::string home = home_env ? home_env : "";
        if (!home.empty()) {
            std::string cfg = (fs::path(home) / ".config").string();
            state.directoryHistory.push(state.currentPath);
            state.currentPath = fs::canonical(cfg).string();
            state.cached_canonical_path.clear();
            state.cached_current_path_for_entries.clear();
            InvalidatePreviewCache();
            state.allContents = FileManager::getDirectoryContents(state.currentPath);
            state.filteredContents = state.allContents;
            state.selected = 0;
            state.current_page = 0;
            state.batch_selected.clear();
            state.searchQuery.clear();
            StatusMessage::Show("Go to ~/.config");
        }
        break;
    }
    case FTB::KeyBindings::PanelCommand::Sort:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::Sort; break;
    case FTB::KeyBindings::PanelCommand::NewTab:
        state.panel_input.clear();
        state.panel_message.clear();
        state.panel_suggestion.clear();
        state.active_panel = ActivePanel::NewTab; break;
    case FTB::KeyBindings::PanelCommand::CloseTab:
        if (state.tabManager.canClose()) {
            state.saveTabState();
            state.tabManager.closeTab(state.tabManager.activeIndex());
            state.loadTabState();
            StatusMessage::Show("Tab closed");
        } else {
            StatusMessage::Show("Cannot close the last tab");
        }
        break;
    case FTB::KeyBindings::PanelCommand::NextTab:
        state.saveTabState();
        state.tabManager.nextTab();
        state.loadTabState();
        break;
    case FTB::KeyBindings::PanelCommand::PrevTab:
        state.saveTabState();
        state.tabManager.prevTab();
        state.loadTabState();
        break;
    case FTB::KeyBindings::PanelCommand::UIStyle: {
        auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
        state.panel_orig_column_sep = cfg.ui.column_separator;
        state.panel_orig_panel_border = cfg.ui.panel_border;
        state.panel_orig_selection_style = cfg.ui.selection_style;
        state.panel_orig_tab_bar_style = cfg.ui.tab_bar_style;
        state.panel_selected = 0;
        state.active_panel = ActivePanel::UIStyle;
        break;
    }
    case FTB::KeyBindings::PanelCommand::StatusBarStyle: {
        auto& cfg = FTB::ConfigManager::GetInstance()->GetConfigMutable();
        state.panel_orig_statusbar_style = cfg.statusbar.style;
        state.panel_selected = 0;
        state.active_panel = ActivePanel::StatusBarStyle;
        break;
    }
#ifdef FTB_ENABLE_PLUGINS
    case FTB::KeyBindings::PanelCommand::Plugin:
        state.panel_selected = 0;
        state.active_panel = ActivePanel::Plugin; break;
#endif
    case FTB::KeyBindings::PanelCommand::VimEdit:
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            fs::path fullPath = fs::path(state.currentPath) / state.filteredContents[state.selected];
            if (!FileManager::isDirectory(fullPath.string())) {
                OpenEditorForFile(state, fullPath.string());
            }
        }
        break;
    case FTB::KeyBindings::PanelCommand::OpenPicker:
        if (state.selected >= 0 && state.selected < static_cast<int>(state.filteredContents.size())) {
            std::string filename = state.filteredContents[state.selected];
            if (!FileManager::isDirectory((fs::path(state.currentPath) / filename).string())) {
                state.matched_openers = FTB::OpenerManager::Instance().MatchOpeners(filename);
                state.opener_selected = 0;
                state.active_panel = ActivePanel::OpenerPicker;
            } else {
                StatusMessage::Show("Cannot open directory with opener");
            }
        }
        break;
    case FTB::KeyBindings::PanelCommand::OpenManual:
        state.panel_input.clear();
        state.active_panel = ActivePanel::OpenerInput;
        break;
    case FTB::KeyBindings::PanelCommand::OpenConfig:
        state.opener_config_name.clear();
        state.opener_config_run.clear();
        state.opener_config_desc.clear();
        state.opener_config_rule_name.clear();
        state.opener_config_rule_use.clear();
        state.opener_config_field = 0;
        state.opener_config_is_orphan = false;
        state.active_panel = ActivePanel::OpenerConfig;
        break;
#ifdef FTB_ENABLE_SSH
    case FTB::KeyBindings::PanelCommand::SSH:
        state.panel_input.clear();
        state.panel_message.clear();
        state.active_panel = ActivePanel::SSH; break;
#endif
#ifdef FTB_ENABLE_AI
    case FTB::KeyBindings::PanelCommand::AI:
        state.panel_input.clear();
        state.panel_message.clear();
        if (!state.ai_agent) {
            state.ai = std::move(AIPanelState{});
            state.ai_agent = std::make_unique<AIAgent>(state);
            auto& sm = SessionManager::getInstance();
            state.ai.current_session_id = sm.activeSessionId();
            if (auto* session = sm.getSession(sm.activeSessionId())) {
                if (!session->messages.empty()) {
                    state.ai_agent->loadSession(session->messages);
                }
            }
        }
        state.active_panel = ActivePanel::AI; break;
    case FTB::KeyBindings::PanelCommand::AIConfig: {
        auto& ai_cfg = ConfigManager::GetInstance()->GetConfigMutable().ai;
        state.ai_config_tab = 0;
        state.ai_config_selected = 0;
        state.ai_config_field = 0;
        state.ai_config_editing = false;
        state.ai_config_active_key = ai_cfg.active_key;
        state.ai_config_keys = ai_cfg.keys;
        state.active_panel = ActivePanel::AIConfig;
        break;
    }
#endif
    default:
        break;
    }
}

void RegisterPanelCommands(FTB::KeyBindings& keybindings, MainState& state) {
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Calendar, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Calendar); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Help, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Help); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Theme, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Theme); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Layout, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Layout); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Rename, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Rename); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NewFile, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NewFile); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NewFolder, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NewFolder); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FilePreview, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FilePreview); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FolderDetails, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FolderDetails); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::JumpDirectory, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::JumpDirectory); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::FuzzyFinder, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::FuzzyFinder); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Search, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Search); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::VimEdit, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::VimEdit); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Sort, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Sort); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::UIStyle, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::UIStyle); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::StatusBarStyle, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::StatusBarStyle); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::ClearClipboard, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::ClearClipboard); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::ShowClipboard, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::ShowClipboard); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Tasks, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Tasks); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::BatchRename, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::BatchRename); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::MDToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::MDToggleSource); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::XLSToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::XLSToggleSource); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::MediaToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::MediaToggleSource); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::MediaPlay, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::MediaPlay); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::PdfToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::PdfToggleSource); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::DocToggleSource, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::DocToggleSource); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::AudioToggleEnabled, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::AudioToggleEnabled); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::HexToggleEnabled, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::HexToggleEnabled); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::GoHome, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::GoHome); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::GoDownloads, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::GoDownloads); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::GoConfig, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::GoConfig); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NewTab, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NewTab); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::CloseTab, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::CloseTab); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::NextTab, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::NextTab); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::PrevTab, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::PrevTab); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::OpenPicker, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::OpenPicker); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::OpenManual, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::OpenManual); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::OpenConfig, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::OpenConfig); });
#ifdef FTB_ENABLE_PLUGINS
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::Plugin, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::Plugin); });
#endif
#ifdef FTB_ENABLE_SSH
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::SSH, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::SSH); });
#endif
#ifdef FTB_ENABLE_AI
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::AI, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::AI); });
    keybindings.RegisterCallback(FTB::KeyBindings::PanelCommand::AIConfig, [&]() { HandlePanelCommand(state, FTB::KeyBindings::PanelCommand::AIConfig); });
#endif
}

}
