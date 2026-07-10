#include "ai/ActionExecutor.hpp"
#include "core/MainUI.hpp"
#include "browser/FileManager.hpp"
#include "browser/ClipboardManager.hpp"
#include "utils/FilesystemUtil.hpp"

#include <filesystem>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <regex>
#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

namespace FTB {

// ====================================================================
// Shared helpers
// ====================================================================

std::string ActionExecutor::globToRegex(const std::string& glob) {
    std::string regex;
    regex.reserve(glob.size() + 3);
    regex += '^';
    for (char c : glob) {
        switch (c) {
            case '*': regex += ".*"; break;
            case '?': regex += "."; break;
            case '.': case '+': case '^': case '$':
            case '{': case '}': case '|': case '(':
            case ')': case '[': case ']': case '\\':
                regex += '\\'; regex += c; break;
            default: regex += c; break;
        }
    }
    regex += '$';
    return regex;
}

std::string ActionExecutor::resolvePath(const std::string& raw, const std::string& cwd) {
    if (raw.empty()) return raw;
    if (raw == "~") {
        const char* home = std::getenv("HOME");
        return home ? home : "/tmp";
    }
    if (!fs::path(raw).is_absolute()) {
        return (fs::path(cwd) / raw).string();
    }
    return raw;
}

std::string ActionExecutor::formatTime(const fs::file_time_type& ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm* tm = std::localtime(&tt);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

// ====================================================================
// Constructor / core methods
// ====================================================================

ActionExecutor::ActionExecutor(MainState& state) : state_(state) {}

void ActionExecutor::setSandboxRoot(const std::string& root) {
    sandbox_root_ = root;
}

bool ActionExecutor::isPathSafe(const std::string& path) const {
    if (sandbox_root_.empty()) return true;
    try {
        auto canonical = fs::weakly_canonical(path);
        auto root = fs::weakly_canonical(sandbox_root_);
        auto rel = fs::relative(canonical, root);
        std::string rel_str = rel.string();
        if (!rel_str.empty() && rel_str[0] == '.') return false;
        return true;
    } catch (...) {
        return false;
    }
}

// ====================================================================
// Build handler dispatch map
// ====================================================================

std::unordered_map<std::string, ActionExecutor::HandlerFn> ActionExecutor::buildHandlerMap() {
    std::unordered_map<std::string, HandlerFn> m;
    // File IO
    m["create_file"]       = &ActionExecutor::handleCreateFile;
    m["write_file"]        = &ActionExecutor::handleWriteFile;
    m["delete_file"]       = &ActionExecutor::handleDeleteFile;
    m["delete_directory"]  = &ActionExecutor::handleDeleteDirectory;
    m["rename"]            = &ActionExecutor::handleRename;
    m["copy"]              = &ActionExecutor::handleCopy;
    m["move"]              = &ActionExecutor::handleMove;
    m["create_directory"]  = &ActionExecutor::handleCreateDirectory;
    m["read_file"]         = &ActionExecutor::handleReadFile;
    // Browse & Search
    m["navigate"]          = &ActionExecutor::handleNavigate;
    m["open_file"]         = &ActionExecutor::handleOpenFile;
    m["execute"]           = &ActionExecutor::handleExecuteCmd;
    m["list_directory"]    = &ActionExecutor::handleListDirectory;
    m["get_file_info"]     = &ActionExecutor::handleGetFileInfo;
    m["search_files"]      = &ActionExecutor::handleSearchFiles;
    m["grep_content"]      = &ActionExecutor::handleGrepContent;
    // Navigation (history)
    m["go_back"]           = &ActionExecutor::handleGoBack;
    m["go_forward"]        = &ActionExecutor::handleGoForward;
    m["go_home"]           = &ActionExecutor::handleGoHome;
    // Clipboard
    m["copy_to_clipboard"]    = &ActionExecutor::handleCopyToClipboard;
    m["cut_to_clipboard"]     = &ActionExecutor::handleCutToClipboard;
    m["paste_from_clipboard"] = &ActionExecutor::handlePasteFromClipboard;
    // Selection & Batch
    m["select_files"]      = &ActionExecutor::handleSelectFiles;
    m["batch_rename"]      = &ActionExecutor::handleBatchRename;
    m["batch_delete"]      = &ActionExecutor::handleBatchDelete;
    // Advanced
    m["create_symlink"]    = &ActionExecutor::handleCreateSymlink;
    m["diff_files"]        = &ActionExecutor::handleDiffFiles;
    m["compress"]          = &ActionExecutor::handleCompress;
    m["extract"]           = &ActionExecutor::handleExtract;
    return m;
}

// ====================================================================
// Dispatch: execute
// ====================================================================

ExecutionResult ActionExecutor::execute(const ToolCall& call) {
    ExecutionResult result;

    try {
        static const auto handler_map = buildHandlerMap();
        auto it = handler_map.find(call.name);
        if (it != handler_map.end()) {
            result = (this->*it->second)(call);
        } else {
            throw std::runtime_error("Unknown action: " + call.name);
        }

        static const std::unordered_set<std::string> refresh_actions = {
            "create_file", "write_file", "delete_file", "delete_directory",
            "rename", "create_directory", "move", "copy",
            "batch_delete", "batch_rename", "paste_from_clipboard",
            "create_symlink"
        };
        if (refresh_actions.find(call.name) != refresh_actions.end()) {
            RefreshDirectoryContents(state_);
        }
    }
    catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.log_type = "error";
    }

    return result;
}

// ====================================================================
// registerDefaultTools
// ====================================================================

void ActionExecutor::registerDefaultTools(ToolRegistry& registry) {
    auto make_schema = [](const std::vector<std::string>& required,
                          const std::vector<std::pair<std::string,std::string>>& props) {
        json properties = json::object();
        for (const auto& p : props) {
            properties[p.first] = {{"type", p.second}};
        }
        json schema = {{"type", "object"}, {"properties", properties}};
        if (!required.empty()) schema["required"] = required;
        return schema;
    };

    auto reg = [&](const std::string& name, const std::string& desc,
                   const std::vector<std::string>& required,
                   const std::vector<std::pair<std::string,std::string>>& props,
                   bool danger) {
        ToolDef t;
        t.name = name;
        t.description = desc;
        t.parameters_schema = make_schema(required, props);
        t.executor = nullptr;
        t.dangerous = danger;
        t.permission = danger ? PermissionLevel::Prompt : PermissionLevel::Allowed;
        if (!registry.registerTool(std::move(t))) {
            // Tool name collision - should not happen with static registration
        }
    };

    // Original actions
    reg("create_file", "Create a new file with content",
        {"path", "content"}, {{"path", "string"}, {"content", "string"}}, false);
    reg("write_file", "Write content to a file",
        {"path", "content"}, {{"path", "string"}, {"content", "string"}}, false);
    reg("delete_file", "Delete a file",
        {"path"}, {{"path", "string"}}, true);
    reg("delete_directory", "Delete a directory",
        {"path"}, {{"path", "string"}}, true);
    reg("rename", "Rename a file or directory",
        {"path", "new_name"}, {{"path", "string"}, {"new_name", "string"}}, false);
    reg("move", "Move a file or directory to a new location (removes the source)",
        {"path", "destination"}, {{"path", "string"}, {"destination", "string"}}, false);
    reg("copy", "Copy a file or directory to a destination (keeps the source intact)",
        {"path", "destination"}, {{"path", "string"}, {"destination", "string"}}, false);
    reg("create_directory", "Create a directory",
        {"path"}, {{"path", "string"}}, false);
    reg("read_file", "Read file content with optional line range",
        {"path"}, {{"path", "string"}, {"start_line", "integer"}, {"end_line", "integer"}}, false);
    reg("navigate", "Navigate to a directory",
        {"path"}, {{"path", "string"}}, false);
    reg("open_file", "Open a file in editor",
        {"path"}, {{"path", "string"}}, false);
    reg("execute", "Execute a shell command",
        {"command"}, {{"command", "string"}}, true);
    reg("list_directory", "List directory contents with optional metadata",
        {}, {{"path", "string"}, {"show_metadata", "boolean"}, {"max_items", "integer"}}, false);

    // P0: New core actions
    reg("get_file_info", "Get file/directory metadata (type, size, permissions, mtime)",
        {"path"}, {{"path", "string"}}, false);
    reg("search_files", "Search files by name pattern (glob) recursively",
        {"pattern"}, {{"pattern", "string"}, {"path", "string"}, {"max_results", "integer"}}, false);
    reg("grep_content", "Search file contents by regex pattern",
        {"pattern"}, {{"pattern", "string"}, {"path", "string"}, {"include", "string"}, {"max_results", "integer"}}, false);

    // P1: Navigation and clipboard
    reg("go_back", "Navigate to previous directory in history", {}, {}, false);
    reg("go_forward", "Navigate to next directory in forward history", {}, {}, false);
    reg("go_home", "Navigate to home directory", {}, {}, false);
    reg("copy_to_clipboard", "Copy files to clipboard",
        {}, {{"all", "boolean"}, {"pattern", "string"}}, false);
    reg("cut_to_clipboard", "Cut files to clipboard",
        {}, {{"all", "boolean"}, {"pattern", "string"}}, false);
    reg("paste_from_clipboard", "Paste files from clipboard",
        {}, {{"force_overwrite", "boolean"}}, false);
    reg("select_files", "Select files by pattern or all",
        {}, {{"all", "boolean"}, {"pattern", "string"}}, false);

    // P2: Batch operations
    reg("batch_rename", "Batch rename files using regex",
        {"pattern", "replacement"}, {{"pattern", "string"}, {"replacement", "string"}}, false);
    reg("batch_delete", "Delete multiple files",
        {"paths"}, {{"paths", "array"}}, true);

    // P3: Advanced features
    reg("create_symlink", "Create a symbolic link",
        {"target", "link_path"}, {{"target", "string"}, {"link_path", "string"}, {"relative", "boolean"}}, false);
    reg("diff_files", "Compare two files and show differences",
        {"file1", "file2"}, {{"file1", "string"}, {"file2", "string"}}, false);
    reg("compress", "Compress files into archive",
        {"paths", "archive_name"}, {{"paths", "array"}, {"archive_name", "string"}, {"format", "string"}}, false);
    reg("extract", "Extract archive to destination",
        {"archive"}, {{"archive", "string"}, {"destination", "string"}}, false);
}

} // namespace FTB
