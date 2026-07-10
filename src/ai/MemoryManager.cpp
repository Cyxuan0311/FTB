#include "ai/MemoryManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace FTB {

using json = nlohmann::json;

int MemoryManager::estimateTokens(const std::string& text) const {
    if (text.empty()) return 0;
    int ascii = 0, non_ascii = 0;
    for (unsigned char c : text) {
        if (c < 128) ascii++;
        else non_ascii++;
    }
    return static_cast<int>(ascii / 3.5 + non_ascii / 1.5 + 1);
}

void MemoryManager::truncateByTokens() {
    int limit = max_context_tokens_ - max_response_tokens_;
    if (limit <= 0) limit = max_context_tokens_ / 2;

    int total = 0;
    for (const auto& m : history_) {
        total += estimateTokens(m.content) + 4;
    }
    metrics_.total_tokens = total;

    while (total > limit && history_.size() > 1) {
        int start = (history_.front().role == Message::System) ? 1 : 0;
        if (start >= static_cast<int>(history_.size())) break;
        int removed = estimateTokens(history_[start].content) + 4;
        history_.erase(history_.begin() + start);
        total -= removed;
        // Remove the following assistant/tool response too
        if (start < static_cast<int>(history_.size()) && history_[start].role != Message::System) {
            removed = estimateTokens(history_[start].content) + 4;
            history_.erase(history_.begin() + start);
            total -= removed;
        }
    }

    metrics_.total_tokens = total;
    metrics_.user_message_count = 0;
    metrics_.assistant_message_count = 0;
    for (const auto& m : history_) {
        if (m.role == Message::User) metrics_.user_message_count++;
        else if (m.role == Message::Assistant) metrics_.assistant_message_count++;
    }
}

void MemoryManager::addMessage(Message::Role role, const std::string& content, const std::string& name) {
    Message m;
    m.role = role;
    m.content = content;
    m.name = name;
    history_.push_back(std::move(m));

    if (role == Message::User) metrics_.user_message_count++;
    else if (role == Message::Assistant) metrics_.assistant_message_count++;

    truncateByTokens();
}

const std::vector<Message>& MemoryManager::getHistory() const {
    return history_;
}

void MemoryManager::setMaxTurns(size_t n) {
    max_turns_ = n;
    while (history_.size() > max_turns_ * 2) {
        history_.erase(history_.begin());
    }
}

void MemoryManager::clear() {
    history_.clear();
    metrics_ = ContextMetrics{};
}

void MemoryManager::setHistory(const std::vector<Message>& messages) {
    history_ = messages;
    metrics_.user_message_count = 0;
    metrics_.assistant_message_count = 0;
    metrics_.tool_error_count = 0;
    for (const auto& m : history_) {
        if (m.role == Message::User) metrics_.user_message_count++;
        else if (m.role == Message::Assistant) metrics_.assistant_message_count++;
    }
    truncateByTokens();
}

size_t MemoryManager::messageCount() const {
    return history_.size();
}

void MemoryManager::setContextLimit(int max_ctx, int max_resp) {
    max_context_tokens_ = max_ctx;
    max_response_tokens_ = max_resp;
    truncateByTokens();
}

void MemoryManager::saveToFile(const std::string& path) {
    json arr = json::array();
    for (const auto& m : history_) {
        arr.push_back({
            {"role", m.role},
            {"content", m.content}
        });
    }
    std::ofstream f(path);
    if (f.is_open()) {
        f << arr.dump(2);
    }
}

void MemoryManager::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;

    try {
        auto arr = json::parse(f);
        clear();
        for (const auto& item : arr) {
            Message m;
            m.role = static_cast<Message::Role>(item["role"].get<int>());
            m.content = item["content"].get<std::string>();
            history_.push_back(std::move(m));
        }
        for (const auto& m : history_) {
            if (m.role == Message::User) metrics_.user_message_count++;
            else if (m.role == Message::Assistant) metrics_.assistant_message_count++;
        }
    } catch (...) {}
}

} // namespace FTB
