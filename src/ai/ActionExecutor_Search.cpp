#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "utils/PerfLogger.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <iomanip>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleGetFileInfo(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    try {
        std::error_code ec;
        auto info = fs::symlink_status(path, ec);
        if (ec) { result.message = "Cannot access " + path; return result; }

        std::stringstream ss;
        ss << "Path: " << path << "\n";
        ss << "Type: ";
        if (fs::is_regular_file(info)) ss << "file";
        else if (fs::is_directory(info)) ss << "directory";
        else if (fs::is_symlink(info)) ss << "symlink";
        else ss << "other";
        ss << "\n";

        if (fs::is_regular_file(path)) {
            ss << "Size: " << fs::file_size(path, ec) << " bytes\n";
        }

        auto ftime = fs::last_write_time(path, ec);
        if (!ec) ss << "Modified: " << formatTime(ftime) << "\n";

        auto perms = info.permissions();
        ss << "Permissions: "
           << ((perms & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
           << ((perms & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
           << ((perms & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
           << ((perms & fs::perms::group_read) != fs::perms::none ? "r" : "-")
           << ((perms & fs::perms::group_write) != fs::perms::none ? "w" : "-")
           << ((perms & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
           << ((perms & fs::perms::others_read) != fs::perms::none ? "r" : "-")
           << ((perms & fs::perms::others_write) != fs::perms::none ? "w" : "-")
           << ((perms & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
           << "\n";

        result.success = true;
        result.message = ss.str();
    } catch (const std::exception& e) {
        result.message = std::string("get_file_info failed: ") + e.what();
    }
    return result;
}

ExecutionResult ActionExecutor::handleSearchFiles(const ToolCall& call) {
    ExecutionResult result;
    auto pattern = call.params["pattern"].get<std::string>();
    std::string base_path;
    if (call.params.contains("path")) {
        base_path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    } else {
        base_path = state_.currentPath;
    }
    if (!isPathSafe(base_path)) { result.message = "Access denied"; return result; }

    int max_results = call.params.value("max_results", 50);

    try {
        std::regex re(globToRegex(pattern));
        std::stringstream ss;
        ss << "Files matching '" << pattern << "' in " << base_path << ":\n";
        int count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(base_path, fs::directory_options::skip_permission_denied)) {
            if (count >= max_results) break;
            if (std::regex_match(entry.path().filename().string(), re)) {
                ss << entry.path().string() << "\n";
                ++count;
            }
        }
        if (count == 0) ss << "(no matches)\n";
        result.success = true;
        result.message = ss.str();
    } catch (const std::regex_error&) {
        // Fallback: use substring matching
        std::stringstream ss;
        ss << "Files matching '" << pattern << "' in " << base_path << ":\n";
        int count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(base_path, fs::directory_options::skip_permission_denied)) {
            if (count >= max_results) break;
            auto fname = entry.path().filename().string();
            if (fname.find(pattern) != std::string::npos) {
                ss << entry.path().string() << "\n";
                ++count;
            }
        }
        if (count == 0) ss << "(no matches)\n";
        result.success = true;
        result.message = ss.str();
    } catch (const std::exception& e) {
        result.message = std::string("Search failed: ") + e.what();
    }
    return result;
}

ExecutionResult ActionExecutor::handleGrepContent(const ToolCall& call) {
    ExecutionResult result;
    auto pattern = call.params["pattern"].get<std::string>();
    std::string base_path;
    if (call.params.contains("path")) {
        base_path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    } else {
        base_path = state_.currentPath;
    }
    if (!isPathSafe(base_path)) { result.message = "Access denied"; return result; }

    std::string include_ext = call.params.value("include", "");
    int max_results = call.params.value("max_results", 30);

    try {
        std::regex re(pattern);
        std::stringstream ss;
        ss << "Content matching '" << pattern << "' in " << base_path << ":\n";
        int count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(base_path, fs::directory_options::skip_permission_denied)) {
            if (count >= max_results) break;
            if (!fs::is_regular_file(entry.path())) continue;
            if (!include_ext.empty() && entry.path().extension().string() != include_ext) continue;

            std::ifstream f(entry.path());
            if (!f) continue;
            std::string line;
            int line_no = 1;
            while (std::getline(f, line)) {
                if (std::regex_search(line, re)) {
                    ss << entry.path().string() << ":" << line_no << ": " << line << "\n";
                    ++count;
                    if (count >= max_results) break;
                }
                ++line_no;
            }
        }
        if (count == 0) ss << "(no matches)\n";
        result.success = true;
        result.message = ss.str();
    } catch (const std::exception& e) {
        result.message = std::string("Grep failed: ") + e.what();
    }
    return result;
}

} // namespace FTB
