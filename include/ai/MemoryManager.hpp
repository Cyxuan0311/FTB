#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include "ai/AIClient.hpp"

namespace FTB {

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

private:
    std::vector<Message> history_;
    size_t max_turns_ = 20;
};

} // namespace FTB
