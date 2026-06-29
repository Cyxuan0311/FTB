#include "ai/SessionManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace FTB {

SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

static std::string generateSessionId() {
    static int counter = 0;
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return "s_" + std::to_string(now) + "_" + std::to_string(counter++);
}

void SessionManager::load(const std::string& path) {
    file_path_ = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        auto id = generateSessionId();
        sessions_.push_back({id, "Session 1", {}, 0, 0});
        active_id_ = id;
        return;
    }

    try {
        auto root = json::parse(f);
        active_id_ = root.value("active_session", "");
        for (const auto& item : root["sessions"]) {
            AISession s;
            s.id = item["id"];
            s.name = item.value("name", "Unnamed");
            s.created_at = item.value("created_at", (int64_t)0);
            s.updated_at = item.value("updated_at", (int64_t)0);
            for (const auto& m : item["messages"]) {
                Message msg;
                msg.role = static_cast<Message::Role>(m["role"].get<int>());
                msg.content = m["content"].get<std::string>();
                s.messages.push_back(std::move(msg));
            }
            sessions_.push_back(std::move(s));
        }
    } catch (const std::exception&) {}

    if (sessions_.empty()) {
        auto id = generateSessionId();
        sessions_.push_back({id, "Session 1", {}, 0, 0});
        active_id_ = id;
    }

    bool found = false;
    for (const auto& s : sessions_) {
        if (s.id == active_id_) { found = true; break; }
    }
    if (!found) active_id_ = sessions_[0].id;
}

void SessionManager::save(const std::string& path) {
    json root;
    root["active_session"] = active_id_;
    json arr = json::array();
    for (const auto& s : sessions_) {
        json msgs = json::array();
        for (const auto& m : s.messages) {
            msgs.push_back({{"role", static_cast<int>(m.role)}, {"content", m.content}});
        }
        arr.push_back({
            {"id", s.id},
            {"name", s.name},
            {"created_at", s.created_at},
            {"updated_at", s.updated_at},
            {"messages", std::move(msgs)}
        });
    }
    root["sessions"] = std::move(arr);

    std::ofstream f(path);
    if (f.is_open()) {
        f << root.dump(2);
    }
}

const AISession* SessionManager::getSession(const std::string& id) const {
    for (const auto& s : sessions_) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

void SessionManager::setActiveSession(const std::string& id) {
    for (const auto& s : sessions_) {
        if (s.id == id) { active_id_ = id; return; }
    }
}

std::string SessionManager::createSession(const std::string& name) {
    auto id = generateSessionId();
    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::string sname = name.empty()
        ? "Session " + std::to_string(sessions_.size() + 1)
        : name;
    sessions_.push_back({id, sname, {}, now, now});
    active_id_ = id;
    return id;
}

void SessionManager::deleteSession(const std::string& id) {
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it->id == id) {
            sessions_.erase(it);
            break;
        }
    }
    if (sessions_.empty()) {
        auto nid = generateSessionId();
        sessions_.push_back({nid, "Session 1", {}, 0, 0});
        active_id_ = nid;
    } else if (active_id_ == id) {
        active_id_ = sessions_.back().id;
    }
}

void SessionManager::renameSession(const std::string& id, const std::string& name) {
    for (auto& s : sessions_) {
        if (s.id == id) { s.name = name; return; }
    }
}

void SessionManager::syncCurrentSession(const std::vector<Message>& messages) {
    for (auto& s : sessions_) {
        if (s.id == active_id_) {
            s.messages = messages;
            s.updated_at = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            return;
        }
    }
}

} // namespace FTB
