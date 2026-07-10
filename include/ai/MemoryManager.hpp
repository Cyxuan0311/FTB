#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include "ai/AIClient.hpp"

namespace FTB {

struct ContextMetrics {
    int total_tokens = 0;
    int user_message_count = 0;
    int assistant_message_count = 0;
    int tool_error_count = 0;
};

class MemoryManager {
public:
    void addMessage(Message::Role role, const std::string& content, const std::string& name = "");
    const std::vector<Message>& getHistory() const;
    void setMaxTurns(size_t n);
    void clear();
    void setHistory(const std::vector<Message>& messages);
    size_t messageCount() const;

    void saveToFile(const std::string& path);
    void loadFromFile(const std::string& path);

    ContextMetrics getMetrics() const { return metrics_; }
    void setContextLimit(int max_ctx, int max_resp);
    int estimateTokens(const std::string& text) const;

private:
    std::vector<Message> history_;
    size_t max_turns_ = 20;
    int max_context_tokens_ = 8192;
    int max_response_tokens_ = 2048;
    ContextMetrics metrics_;

    void truncateByTokens();
};

} // namespace FTB
