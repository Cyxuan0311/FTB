#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "utils/FilesystemUtil.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace FTB {

ExecutionResult ActionExecutor::handleCreateFile(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    std::ofstream f(path);
    if (!f) { result.message = "Failed to create file"; return result; }
    if (call.params.contains("content")) {
        f << call.params["content"].get<std::string>();
    }
    f.close();
    result.success = true;
    result.message = "Created " + fs::path(path).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleWriteFile(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    std::ofstream f(path);
    if (!f) { result.message = "Failed to write file"; return result; }
    f << call.params["content"].get<std::string>();
    f.close();
    result.success = true;
    result.message = "Written to " + fs::path(path).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleDeleteFile(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    if (!fs::remove(path)) { result.message = "Failed to delete file"; return result; }
    result.success = true;
    result.message = "Deleted " + fs::path(path).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleDeleteDirectory(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    std::error_code ec;
    fs::remove_all(path, ec);
    if (ec) { result.message = "Failed to delete directory: " + ec.message(); return result; }
    result.success = true;
    result.message = "Deleted directory " + fs::path(path).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleRename(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    auto new_name = resolvePath(call.params["new_name"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path) || !isPathSafe(new_name)) { result.message = "Access denied"; return result; }

    std::error_code ec;
    fs::rename(path, new_name, ec);
    if (ec) {
        if (!renameWithFallback(path, new_name)) {
            result.message = "Failed to rename: " + ec.message();
            return result;
        }
    }
    result.success = true;
    result.message = "Renamed " + fs::path(path).filename().string() + " to " + fs::path(new_name).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleCopy(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    auto dest = resolvePath(call.params["destination"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path) || !isPathSafe(dest)) { result.message = "Access denied"; return result; }

    if (fs::is_directory(dest)) {
        dest = (fs::path(dest) / fs::path(path).filename()).string();
    }

    std::error_code ec;
    if (fs::is_directory(path)) {
        fs::copy(path, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    } else {
        fs::copy_file(path, dest, fs::copy_options::overwrite_existing, ec);
    }
    if (ec) { result.message = "Failed to copy: " + ec.message(); return result; }
    result.success = true;
    result.message = "Copied to " + fs::path(dest).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleMove(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    auto dest = resolvePath(call.params["destination"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path) || !isPathSafe(dest)) { result.message = "Access denied"; return result; }

    if (fs::is_directory(dest)) {
        dest = (fs::path(dest) / fs::path(path).filename()).string();
    }

    std::error_code ec;
    fs::rename(path, dest, ec);
    if (ec) {
        if (!renameWithFallback(path, dest)) {
            result.message = "Failed to move: " + ec.message();
            return result;
        }
    }
    result.success = true;
    result.message = "Moved to " + fs::path(dest).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleCreateDirectory(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) { result.message = "Failed to create directory: " + ec.message(); return result; }
    result.success = true;
    result.message = "Created directory " + fs::path(path).filename().string();
    return result;
}

ExecutionResult ActionExecutor::handleReadFile(const ToolCall& call) {
    ExecutionResult result;
    auto path = resolvePath(call.params["path"].get<std::string>(), state_.currentPath);
    if (!isPathSafe(path)) { result.message = "Access denied"; return result; }

    std::ifstream f(path);
    if (!f) { result.message = "Failed to open file"; return result; }

    int start_line = 0, end_line = -1;
    if (call.params.contains("start_line")) start_line = call.params["start_line"].get<int>();
    if (call.params.contains("end_line")) end_line = call.params["end_line"].get<int>();

    std::stringstream ss;
    ss << f.rdbuf();
    auto content = ss.str();

    if (start_line > 0 || end_line > 0) {
        std::istringstream stream(content);
        std::string line;
        std::stringstream filtered;
        int line_no = 1;
        while (std::getline(stream, line)) {
            if (line_no >= start_line && (end_line < 0 || line_no <= end_line)) {
                filtered << line << "\n";
            }
            ++line_no;
        }
        content = filtered.str();
    }

    result.success = true;
    result.message = content;
    result.log_type = "file_content";
    return result;
}

} // namespace FTB
