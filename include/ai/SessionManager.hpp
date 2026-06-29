#pragma once

#include <string>
#include <vector>
#include "ai/AIClient.hpp"

namespace FTB {

struct AISession {
    std::string id;
    std::string name;
    std::vector<Message> messages;
    int64_t created_at = 0;
    int64_t updated_at = 0;
};

class SessionManager {
public:
    static SessionManager& getInstance();

    void load(const std::string& path);
    void save(const std::string& path);

    const std::vector<AISession>& sessions() const { return sessions_; }

    const AISession* getSession(const std::string& id) const;
    std::string activeSessionId() const { return active_id_; }
    void setActiveSession(const std::string& id);

    std::string createSession(const std::string& name = "");
    void deleteSession(const std::string& id);
    void renameSession(const std::string& id, const std::string& name);

    void syncCurrentSession(const std::vector<Message>& messages);
    std::string filePath() const { return file_path_; }

private:
    SessionManager() = default;
    std::vector<AISession> sessions_;
    std::string active_id_;
    std::string file_path_;
};

} // namespace FTB
