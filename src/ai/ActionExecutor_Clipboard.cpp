#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "browser/FileManager.hpp"
#include "browser/ClipboardManager.hpp"
#include "utils/PerfLogger.hpp"

#include <filesystem>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleCopyToClipboard(const ToolCall& call) {
    ExecutionResult result;
    auto& cm = ClipboardManager::getInstance();
    auto entries = FileManager::getDirectoryEntries(state_.currentPath);

    std::vector<std::string> to_copy;
    if (call.params.contains("all") && call.params["all"].get<bool>()) {
        for (const auto& e : entries) to_copy.push_back(e.name);
    } else if (call.params.contains("pattern")) {
        try {
            std::regex re(globToRegex(call.params["pattern"].get<std::string>()));
            for (const auto& e : entries) {
                if (std::regex_match(e.name, re)) {
                    to_copy.push_back(e.name);
                }
            }
        } catch (...) {
            auto pat = call.params["pattern"].get<std::string>();
            for (const auto& e : entries) {
                if (e.name.find(pat) != std::string::npos) {
                    to_copy.push_back(e.name);
                }
            }
        }
    }
    if (to_copy.empty()) { result.message = "No files matched"; return result; }

    cm.setCutMode(false);
    for (const auto& name : to_copy) {
        auto full = (fs::path(state_.currentPath) / name).string();
        cm.addItem(full);
    }
    result.success = true;
    result.message = "Copied " + std::to_string(to_copy.size()) + " item(s) to clipboard";
    return result;
}

ExecutionResult ActionExecutor::handleCutToClipboard(const ToolCall& call) {
    ExecutionResult result;
    auto& cm = ClipboardManager::getInstance();
    auto entries = FileManager::getDirectoryEntries(state_.currentPath);

    std::vector<std::string> to_cut;
    if (call.params.contains("all") && call.params["all"].get<bool>()) {
        for (const auto& e : entries) to_cut.push_back(e.name);
    } else if (call.params.contains("pattern")) {
        try {
            std::regex re(globToRegex(call.params["pattern"].get<std::string>()));
            for (const auto& e : entries) {
                if (std::regex_match(e.name, re)) {
                    to_cut.push_back(e.name);
                }
            }
        } catch (...) {
            auto pat = call.params["pattern"].get<std::string>();
            for (const auto& e : entries) {
                if (e.name.find(pat) != std::string::npos) {
                    to_cut.push_back(e.name);
                }
            }
        }
    }
    if (to_cut.empty()) { result.message = "No files matched"; return result; }

    cm.setCutMode(true);
    for (const auto& name : to_cut) {
        auto full = (fs::path(state_.currentPath) / name).string();
        cm.addItem(full);
    }
    result.success = true;
    result.message = "Cut " + std::to_string(to_cut.size()) + " item(s) to clipboard";
    return result;
}

ExecutionResult ActionExecutor::handlePasteFromClipboard(const ToolCall& call) {
    ExecutionResult result;
    auto& cm = ClipboardManager::getInstance();
    bool force = call.params.value("force_overwrite", false);

    if (cm.getItems().empty()) { result.message = "Clipboard is empty"; return result; }

    auto msg = cm.paste(state_.currentPath, force);
    result.success = true;
    result.message = msg;
    RefreshDirectoryContents(state_);
    return result;
}

ExecutionResult ActionExecutor::handleSelectFiles(const ToolCall& call) {
    ExecutionResult result;

    if (call.params.contains("all") && call.params["all"].get<bool>()) {
        for (size_t i = 0; i < state_.allContents.size(); ++i) {
            state_.batch_selected.insert(static_cast<int>(i));
        }
        result.success = true;
        result.message = "Selected all " + std::to_string(state_.allContents.size()) + " items";
        return result;
    }

    if (call.params.contains("pattern")) {
        int count = 0;
        try {
            std::regex re(globToRegex(call.params["pattern"].get<std::string>()));
            for (size_t i = 0; i < state_.allContents.size(); ++i) {
                if (std::regex_match(state_.allContents[i], re)) {
                    state_.batch_selected.insert(static_cast<int>(i));
                    ++count;
                }
            }
        } catch (...) {
            auto pat = call.params["pattern"].get<std::string>();
            for (size_t i = 0; i < state_.allContents.size(); ++i) {
                if (state_.allContents[i].find(pat) != std::string::npos) {
                    state_.batch_selected.insert(static_cast<int>(i));
                    ++count;
                }
            }
        }
        if (count > 0) {
            result.success = true;
            result.message = "Selected " + std::to_string(count) + " item(s)";
        } else {
            result.message = "No files matched pattern";
        }
        return result;
    }

    result.message = "Specify 'all' or 'pattern'";
    return result;
}

} // namespace FTB
