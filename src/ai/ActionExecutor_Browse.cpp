#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "browser/FileManager.hpp"
#include "utils/PerfLogger.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleNavigate(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    try {
        auto canon = fs::weakly_canonical(path);
        if (!fs::is_directory(canon)) {
            result.message = "Not a directory";
            return result;
        }
        state_.directoryHistory.push(state_.currentPath);
        state_.currentPath = canon.string();
        RefreshDirectoryContents(state_);
        result.success = true;
        result.message = "Navigated to " + canon.string();
    } catch (const std::exception& e) {
        result.message = std::string("Navigate failed: ") + e.what();
    }
    return result;
}

ExecutionResult ActionExecutor::handleOpenFile(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    auto target_name = fs::path(path).filename().string();
    auto& all = state_.allContents;
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i] == target_name) {
            state_.selected = static_cast<int>(i);
            state_.active_panel = ActivePanel::None;
            result.success = true;
            result.message = "Opened " + target_name;
            return result;
        }
    }
    result.message = "File not found in current directory";
    return result;
}

ExecutionResult ActionExecutor::handleExecuteCmd(const ToolCall& call) {
    ExecutionResult result;
    auto cmd = call.params["command"].get<std::string>();

    std::array<char, 4096> buffer;
    std::string cmd_safe;
    if (state_.currentPath.find(' ') != std::string::npos) {
        cmd_safe = "cd \"" + state_.currentPath + "\" && " + cmd;
    } else {
        cmd_safe = "cd " + state_.currentPath + " && " + cmd;
    }
    cmd_safe += " 2>&1";

    FILE* pipe = popen(cmd_safe.c_str(), "r");
    if (!pipe) {
        result.message = "Failed to execute command";
        return result;
    }

    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    int rc = pclose(pipe);

    result.success = true;
    if (output.size() > 32000) {
        output = output.substr(0, 32000) + "\n...[truncated]";
    }
    if (rc != 0) {
        result.message = "Command exited with code " + std::to_string(rc) + ":\n" + output;
    } else {
        result.message = output;
    }
    return result;
}

ExecutionResult ActionExecutor::handleListDirectory(const ToolCall& call) {
    ExecutionResult result;
    std::string path_str;
    if (call.params.contains("path")) {
        path_str = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    } else {
        path_str = state_.currentPath;
    }
    if (!isPathSafe(path_str)) { result.message = "Access denied"; return result; }

    bool show_metadata = call.params.value("show_metadata", false);
    int max_items = call.params.value("max_items", 100);

    try {
        auto entries = FileManager::getDirectoryEntries(path_str);
        std::stringstream ss;
        ss << "Contents of " << path_str << ":\n";
        int count = 0;
        for (const auto& e : entries) {
            if (count >= max_items) {
                ss << "... and " << (entries.size() - count) << " more\n";
                break;
            }
            if (show_metadata) {
                ss << (e.is_dir ? 'd' : '-')
                   << " " << std::setw(8) << e.file_size
                   << " " << e.mod_time
                   << " " << e.name << "\n";
            } else {
                ss << "  " << e.name << "\n";
            }
            ++count;
        }
        result.success = true;
        result.message = ss.str();
    } catch (const std::exception& e) {
        result.message = std::string("List failed: ") + e.what();
    }
    return result;
}

ExecutionResult ActionExecutor::handleGoBack(const ToolCall&) {
    ExecutionResult result;
    if (!state_.directoryHistory.empty()) {
        state_.forward_history.push(state_.currentPath);
        state_.currentPath = state_.directoryHistory.pop();
        RefreshDirectoryContents(state_);
        result.success = true;
        result.message = "Navigated back to " + state_.currentPath;
    } else {
        result.message = "No previous directory in history";
    }
    return result;
}

ExecutionResult ActionExecutor::handleGoForward(const ToolCall&) {
    ExecutionResult result;
    if (!state_.forward_history.empty()) {
        state_.directoryHistory.push(state_.currentPath);
        state_.currentPath = state_.forward_history.pop();
        RefreshDirectoryContents(state_);
        result.success = true;
        result.message = "Navigated forward to " + state_.currentPath;
    } else {
        result.message = "No forward directory in history";
    }
    return result;
}

ExecutionResult ActionExecutor::handleGoHome(const ToolCall&) {
    ExecutionResult result;
    const char* home = std::getenv("HOME");
    if (!home) { result.message = "HOME not set"; return result; }

    state_.directoryHistory.push(state_.currentPath);
    state_.currentPath = home;
    RefreshDirectoryContents(state_);
    result.success = true;
    result.message = "Navigated home";
    return result;
}

} // namespace FTB
