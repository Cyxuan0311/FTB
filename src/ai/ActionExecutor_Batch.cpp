#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "browser/FileManager.hpp"
#include "utils/PerfLogger.hpp"

#include <filesystem>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleBatchRename(const ToolCall& call) {
    ExecutionResult result;
    auto pattern = call.params["pattern"].get<std::string>();
    auto replacement = call.params["replacement"].get<std::string>();

    auto entries = FileManager::getDirectoryEntries(state_.currentPath);

    try {
        std::regex re(pattern);
        int count = 0;
        std::stringstream ss;
        ss << "Renamed:\n";
        for (auto& e : entries) {
            std::string new_name = std::regex_replace(e.name, re, replacement);
            if (new_name != e.name) {
                auto src = (fs::path(state_.currentPath) / e.name).string();
                auto dst = (fs::path(state_.currentPath) / new_name).string();
                std::error_code ec;
                fs::rename(src, dst, ec);
                if (!ec) {
                    ss << "  " << e.name << " -> " << new_name << "\n";
                    ++count;
                }
            }
        }
        if (count > 0) {
            RefreshDirectoryContents(state_);
            result.success = true;
            result.message = ss.str();
        } else {
            result.message = "No files matched pattern";
        }
    } catch (const std::regex_error& e) {
        result.message = std::string("Invalid regex pattern: ") + e.what();
    }
    return result;
}

ExecutionResult ActionExecutor::handleBatchDelete(const ToolCall& call) {
    ExecutionResult result;
    if (!call.params.contains("paths") || !call.params["paths"].is_array()) {
        result.message = "paths must be an array";
        return result;
    }

    auto paths = call.params["paths"].get<std::vector<std::string>>();
    int count = 0;
    for (const auto& p : paths) {
        auto full = resolvePath(p, state_.currentPath);
        if (!isPathSafe(full)) continue;
        std::error_code ec;
        if (fs::is_directory(full)) {
            fs::remove_all(full, ec);
        } else {
            fs::remove(full, ec);
        }
        if (!ec) ++count;
    }
    if (count > 0) {
        RefreshDirectoryContents(state_);
        result.success = true;
        result.message = "Deleted " + std::to_string(count) + " item(s)";
    } else {
        result.message = "No items were deleted";
    }
    return result;
}

} // namespace FTB
