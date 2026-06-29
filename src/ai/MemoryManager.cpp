#include "ai/MemoryManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace FTB {

using json = nlohmann::json;

void MemoryManager::addMessage(Message::Role role, const std::string& content, const std::string& name) {
    Message m;
    m.role = role;
    m.content = content;
    m.name = name;
    history_.push_back(std::move(m));
    while (history_.size() > max_turns_ * 2) {
        history_.erase(history_.begin());
    }
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
}

void MemoryManager::setHistory(const std::vector<Message>& messages) {
    history_ = messages;
    while (history_.size() > max_turns_ * 2) {
        history_.erase(history_.begin());
    }
}

size_t MemoryManager::messageCount() const {
    return history_.size();
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
        history_.clear();
        for (const auto& item : arr) {
            Message m;
            m.role = static_cast<Message::Role>(item["role"].get<int>());
            m.content = item["content"].get<std::string>();
            history_.push_back(std::move(m));
        }
    } catch (...) {}
}

} // namespace FTB
