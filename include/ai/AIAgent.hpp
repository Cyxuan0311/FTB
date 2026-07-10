#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "ai/AIClient.hpp"
#include "ai/ContextBuilder.hpp"
#include "ai/ToolRegistry.hpp"
#include "ai/ResponseRouter.hpp"
#include "ai/ActionExecutor.hpp"
#include "ai/MemoryManager.hpp"

namespace FTB {

struct MainState;
struct AILogEntry;

class AIAgent {
public:
    explicit AIAgent(MainState& state);
    ~AIAgent();

    void sendRequest(const std::string& user_input, bool add_user_message = true);
    void cancel();
    void onRenderTick();
    void loadSession(const std::vector<Message>& messages);
    void syncToSessionManager();

    bool isProcessing() const;
    bool isStreaming() const;
    const std::string& getStreamBuffer() const;
    std::string getPendingResponse() const;

    AIClient& client() { return client_; }
    ToolRegistry& tools() { return tools_; }
    MemoryManager& memory() { return memory_; }

    // Tool confirmation
    void confirmCurrentTool();
    void skipCurrentTool();
    void alwaysAllowCurrentTool();

    bool hasPendingConfirmations() const { return pending_tool_idx_ < static_cast<int>(pending_tool_calls_.size()); }

private:
    void handleStreamChunk(const std::string& delta);
    void handleResponseDone(const std::string& full_response);
    void handleError(const std::string& error);
    void processPendingResponse();
    void continueProcessing();
    void advanceAfterConfirm();
    int stream_visible_len() const;

    MainState& state_;
    AIClient client_;
    ToolRegistry tools_;
    ActionExecutor executor_;
    MemoryManager memory_;

    std::string stream_buffer_;
    std::string pending_response_;
    bool processing_ = false;
    bool streaming_ = false;

    bool needs_auto_continue_ = false;
    int auto_continue_count_ = 0;
    static constexpr int max_auto_continue_ = 5;

    // Tool permission confirmation
    std::vector<ToolCall> pending_tool_calls_;
    int pending_tool_idx_ = 0;

    // Doom loop detection
    std::unordered_map<std::string, int> consecutive_tool_errors_;
    std::vector<std::string> skipped_tools_;
    static constexpr int max_consecutive_errors_ = 3;
};

} // namespace FTB
