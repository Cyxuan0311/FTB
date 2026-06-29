#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <nlohmann/json.hpp>

namespace FTB {

struct Message {
    enum Role { System, User, Assistant, Tool };
    Role role;
    std::string content;
    std::string name;
};

class AIClient {
public:
    enum class Backend { Ollama, OpenAI };

    struct Config {
        Backend backend = Backend::Ollama;
        std::string endpoint = "http://localhost:11434";
        std::string model = "llama3.2";
        std::string api_key;
        nlohmann::json tools;
    };

    using StreamCallback = std::function<void(const std::string& delta)>;
    using DoneCallback = std::function<void(const std::string& full_response)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    AIClient();
    ~AIClient();

    void setConfig(const Config& cfg);
    const Config& getConfig() const { return config_; }

    void sendMessage(const std::vector<Message>& messages,
                     StreamCallback on_stream,
                     DoneCallback on_done,
                     ErrorCallback on_error);

    void cancel();
    bool isProcessing() const { return processing_; }

private:
    void sendOllama(const std::string& body,
                    StreamCallback on_stream,
                    DoneCallback on_done,
                    ErrorCallback on_error);

    void sendOpenAI(const std::string& body,
                    StreamCallback on_stream,
                    DoneCallback on_done,
                    ErrorCallback on_error);

    std::string buildOpenAIBody(const std::vector<Message>& messages);
    std::string buildOllamaBody(const std::vector<Message>& messages);

    Config config_;
    std::atomic<bool> processing_{false};
    std::atomic<bool> cancelled_{false};
    std::thread worker_;
};

} // namespace FTB
