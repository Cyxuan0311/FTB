#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "ai/ToolRegistry.hpp"

namespace FTB {

struct MainState;

struct ExecutionResult {
    bool success = false;
    std::string message;
    std::string log_type;  // "success", "error", "system"
};

class ActionExecutor {
public:
    explicit ActionExecutor(MainState& state);

    ExecutionResult execute(const ToolCall& call);
    bool isPathSafe(const std::string& path) const;
    void setSandboxRoot(const std::string& root);
    void registerDefaultTools(ToolRegistry& registry);

private:
    // === shared helpers ===
    static std::string resolvePath(const std::string& path, const std::string& cwd);
    static std::string globToRegex(const std::string& glob);
    static std::string formatTime(const std::filesystem::file_time_type& ft);

    using HandlerFn = ExecutionResult (ActionExecutor::*)(const ToolCall&);
    static std::unordered_map<std::string, HandlerFn> buildHandlerMap();

    // === File IO ===
    ExecutionResult handleCreateFile(const ToolCall& call);
    ExecutionResult handleDeleteFile(const ToolCall& call);
    ExecutionResult handleDeleteDirectory(const ToolCall& call);
    ExecutionResult handleRename(const ToolCall& call);
    ExecutionResult handleCopy(const ToolCall& call);
    ExecutionResult handleMove(const ToolCall& call);
    ExecutionResult handleCreateDirectory(const ToolCall& call);
    ExecutionResult handleWriteFile(const ToolCall& call);
    ExecutionResult handleReadFile(const ToolCall& call);

    // === Browse & Search ===
    ExecutionResult handleNavigate(const ToolCall& call);
    ExecutionResult handleOpenFile(const ToolCall& call);
    ExecutionResult handleExecuteCmd(const ToolCall& call);
    ExecutionResult handleListDirectory(const ToolCall& call);
    ExecutionResult handleGetFileInfo(const ToolCall& call);
    ExecutionResult handleSearchFiles(const ToolCall& call);
    ExecutionResult handleGrepContent(const ToolCall& call);

    // === Navigation (history) ===
    ExecutionResult handleGoBack(const ToolCall& call);
    ExecutionResult handleGoForward(const ToolCall& call);
    ExecutionResult handleGoHome(const ToolCall& call);

    // === Clipboard ===
    ExecutionResult handleCopyToClipboard(const ToolCall& call);
    ExecutionResult handleCutToClipboard(const ToolCall& call);
    ExecutionResult handlePasteFromClipboard(const ToolCall& call);

    // === Selection & Batch ===
    ExecutionResult handleSelectFiles(const ToolCall& call);
    ExecutionResult handleBatchRename(const ToolCall& call);
    ExecutionResult handleBatchDelete(const ToolCall& call);

    // === Advanced ===
    ExecutionResult handleCreateSymlink(const ToolCall& call);
    ExecutionResult handleDiffFiles(const ToolCall& call);
    ExecutionResult handleCompress(const ToolCall& call);
    ExecutionResult handleExtract(const ToolCall& call);

    MainState& state_;
    std::string sandbox_root_;
};

} // namespace FTB
