#include "ai/AIClient.hpp"
#include "httplib.h"
#include <nlohmann/json.hpp>

#if defined(CPPHTTPLIB_OPENSSL_SUPPORT) && !defined(CPPHTTPLIB_SSL_PREPROCESSOR)
#define FTB_HAVE_SSL
#endif
#ifdef FTB_HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include <sstream>
#include <map>

using json = nlohmann::json;

namespace FTB {

AIClient::AIClient() = default;

AIClient::~AIClient() {
    cancel();
    if (worker_.joinable()) worker_.join();
}

void AIClient::setConfig(const Config& cfg) { config_ = cfg; }

void AIClient::cancel() { cancelled_ = true; }

std::string AIClient::buildOpenAIBody(const std::vector<Message>& messages) {
    json msgs = json::array();
    for (const auto& m : messages) {
        std::string role;
        switch (m.role) {
            case Message::System:    role = "system";    break;
            case Message::User:      role = "user";      break;
            case Message::Tool:      role = "tool";      break;
            default:                 role = "assistant"; break;
        }
        json msg = {{"role", role}, {"content", m.content}};
        if (m.role == Message::Tool && !m.name.empty()) {
            msg["name"] = m.name;
        }
        msgs.push_back(std::move(msg));
    }
    json body = {
        {"model", config_.model},
        {"stream", true},
        {"messages", msgs}
    };
    if (!config_.tools.empty()) {
        body["tools"] = config_.tools;
        body["tool_choice"] = "auto";
    }
    return body.dump();
}

std::string AIClient::buildOllamaBody(const std::vector<Message>& messages) {
    json msgs = json::array();
    for (const auto& m : messages) {
        std::string role;
        switch (m.role) {
            case Message::System:    role = "system";    break;
            case Message::User:      role = "user";      break;
            case Message::Tool:      role = "tool";      break;
            default:                 role = "assistant"; break;
        }
        json msg = {{"role", role}, {"content", m.content}};
        if (m.role == Message::Tool && !m.name.empty()) {
            msg["name"] = m.name;
        }
        msgs.push_back(std::move(msg));
    }
    json body = {
        {"model", config_.model},
        {"stream", true},
        {"messages", msgs}
    };
    return body.dump();
}

void AIClient::sendMessage(const std::vector<Message>& messages,
                           StreamCallback on_stream,
                           DoneCallback on_done,
                           ErrorCallback on_error) {
    cancel();
    if (worker_.joinable()) worker_.join();

    cancelled_ = false;
    processing_ = true;

    std::string body = (config_.backend == Backend::Ollama)
        ? buildOllamaBody(messages)
        : buildOpenAIBody(messages);

    if (config_.backend == Backend::Ollama) {
        worker_ = std::thread(&AIClient::sendOllama, this, body,
                              std::move(on_stream), std::move(on_done), std::move(on_error));
    } else {
        worker_ = std::thread(&AIClient::sendOpenAI, this, body,
                              std::move(on_stream), std::move(on_done), std::move(on_error));
    }
}

void AIClient::sendOllama(const std::string& body,
                           StreamCallback on_stream,
                           DoneCallback on_done,
                           ErrorCallback on_error) {
    std::string full_content;

    try {
        httplib::Client cli(config_.endpoint);
        cli.set_read_timeout(120);
        cli.set_connection_timeout(10);

        auto res = cli.Post("/api/chat", body, "application/json");

        if (cancelled_) {
            if (on_error) on_error("Cancelled");
        } else if (res && res->status == 200) {
            std::istringstream stream(res->body);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.empty()) continue;
                try {
                    auto chunk = json::parse(line);
                    if (chunk.contains("message") && chunk["message"].contains("content")) {
                        std::string delta = chunk["message"]["content"].get<std::string>();
                        full_content += delta;
                        if (on_stream) on_stream(delta);
                    }
                    if (chunk.value("done", false)) break;
                } catch (const json::parse_error&) {}
            }
            if (on_done) on_done(full_content);
        } else {
            std::string err = res ? httplib::to_string(res.error()) : "No response";
            if (on_error) on_error("Connection failed: " + err);
        }
    } catch (const std::exception& e) {
        if (on_error) on_error(std::string("Error: ") + e.what());
    }

    processing_ = false;
}

void AIClient::sendOpenAI(const std::string& body,
                           StreamCallback on_stream,
                           DoneCallback on_done,
                           ErrorCallback on_error) {
    std::string full_content;
    struct ToolCallAccum {
        int index = 0;
        std::string id;
        std::string name;
        std::string arguments;
    };
    std::map<int, ToolCallAccum> tool_calls_accum;

    try {
        std::string host, path = "/v1/chat/completions";
        bool is_https = false;

        if (!config_.endpoint.empty()) {
            is_https = config_.endpoint.compare(0, 8, "https://") == 0;
            auto proto_end = config_.endpoint.find("//");
            if (proto_end != std::string::npos) {
                auto slash = config_.endpoint.find('/', proto_end + 2);
                if (slash != std::string::npos) {
                    host = config_.endpoint.substr(proto_end + 2, slash - proto_end - 2);
                    auto base = config_.endpoint.substr(slash);
                    while (!base.empty() && base.back() == '/') base.pop_back();
                    // If endpoint already ends with /chat/completions, use as-is
                    // Otherwise append /chat/completions
                    const std::string suffix = "/chat/completions";
                    if (base.size() >= suffix.size() &&
                        base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0) {
                        path = base;
                    } else {
                        path = base + suffix;
                    }
                } else {
                    host = config_.endpoint.substr(proto_end + 2);
                }
            } else {
                host = config_.endpoint;
            }
        }

        httplib::Headers headers = {
            {"Content-Type", "application/json"}
        };
        if (!config_.api_key.empty()) {
            headers.insert({"Authorization", "Bearer " + config_.api_key});
        }

        auto do_request = [&](auto& cli) -> httplib::Result {
            cli.set_read_timeout(120);
            cli.set_connection_timeout(10);
            return cli.Post(path.c_str(), headers, body, "application/json");
        };

        auto handle_result = [&](httplib::Result&& result) {
            if (!result) {
                if (on_error) on_error("No response from server");
                return;
            }
            if (cancelled_) {
                if (on_error) on_error("Cancelled");
                return;
            }
            if (result->status != 200) {
                std::string err = "HTTP " + std::to_string(result->status);
                try {
                    auto err_json = json::parse(result->body);
                    if (err_json.contains("error")) {
                        if (err_json["error"].is_string()) {
                            err = err_json["error"].get<std::string>();
                        } else if (err_json["error"].contains("message")) {
                            err = err_json["error"]["message"].get<std::string>();
                        }
                    }
                } catch (const json::parse_error&) {}
                if (on_error) on_error("API error: " + err);
                return;
            }

            std::istringstream stream(result->body);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.empty() || line == "data: [DONE]") continue;
                if (line.substr(0, 6) != "data: ") continue;
                try {
                    auto chunk = json::parse(line.substr(6));
                    if (chunk.contains("choices") && !chunk["choices"].empty()) {
                        auto& choice = chunk["choices"][0];
                        if (choice.contains("delta")) {
                            auto& delta = choice["delta"];
                            if (delta.contains("content") && delta["content"].is_string()) {
                                std::string c = delta["content"].get<std::string>();
                                full_content += c;
                                if (on_stream) on_stream(c);
                            }
                            if (delta.contains("tool_calls") && delta["tool_calls"].is_array()) {
                                for (auto& tc : delta["tool_calls"]) {
                                    int idx = tc.value("index", 0);
                                    auto& acc = tool_calls_accum[idx];
                                    acc.index = idx;
                                    if (tc.contains("id") && tc["id"].is_string())
                                        acc.id = tc["id"].get<std::string>();
                                    if (tc.contains("function")) {
                                        auto& func = tc["function"];
                                        if (func.contains("name") && func["name"].is_string())
                                            acc.name = func["name"].get<std::string>();
                                        if (func.contains("arguments") && func["arguments"].is_string())
                                            acc.arguments += func["arguments"].get<std::string>();
                                    }
                                }
                            }
                        }
                    }
                } catch (const json::parse_error&) {}
            }

            if (!tool_calls_accum.empty()) {
                json steps = json::array();
                for (auto& [idx, tc] : tool_calls_accum) {
                    json params;
                    try {
                        if (!tc.arguments.empty())
                            params = json::parse(tc.arguments);
                    } catch (const json::parse_error&) {}
                    steps.push_back({
                        {"action", tc.name},
                        {"params", params},
                        {"description", tc.name}
                    });
                }
                json resp_json = {
                    {"thought", full_content},
                    {"steps", steps}
                };
                std::string resp_str = resp_json.dump();
                if (on_done) on_done(resp_str);
            } else if (!full_content.empty()) {
                if (on_done) on_done(full_content);
            } else {
                try {
                    auto json_resp = json::parse(result->body);
                    if (json_resp.contains("choices") && !json_resp["choices"].empty()) {
                        auto& choice = json_resp["choices"][0];
                        if (choice.contains("message") && choice["message"].contains("content")) {
                            full_content = choice["message"]["content"].get<std::string>();
                        } else if (choice.contains("delta") && choice["delta"].contains("content")) {
                            full_content = choice["delta"]["content"].get<std::string>();
                        }
                    } else if (json_resp.contains("message") && json_resp["message"].contains("content")) {
                        full_content = json_resp["message"]["content"].get<std::string>();
                    }
                } catch (const json::parse_error&) {
                }
                if (!full_content.empty()) {
                    if (on_stream) on_stream(full_content);
                    if (on_done) on_done(full_content);
                } else {
                    if (on_error) on_error("Empty response from API");
                }
            }
        };

#ifdef FTB_HAVE_SSL
        if (is_https) {
            httplib::SSLClient cli(host);
            handle_result(do_request(cli));
        } else
#endif
        {
            httplib::Client cli(host);
            handle_result(do_request(cli));
        }
    } catch (const std::exception& e) {
        if (on_error) on_error(std::string("Error: ") + e.what());
    }

    processing_ = false;
}

} // namespace FTB
