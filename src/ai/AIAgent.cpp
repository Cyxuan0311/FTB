#include "ai/AIAgent.hpp"
#include "ai/SessionManager.hpp"
#include "core/MainUI.hpp"
#include "config/ConfigManager.hpp"
#include "renderer/detail_element.hpp"

namespace FTB {

AIAgent::AIAgent(MainState& state)
    : state_(state), executor_(state)
{
    executor_.registerDefaultTools(tools_);
}

AIAgent::~AIAgent() {
    cancel();
}

void AIAgent::sendRequest(const std::string& user_input, bool add_user_message) {
    if (add_user_message) {
        needs_auto_continue_ = false;
        auto_continue_count_ = 0;
        consecutive_tool_errors_.clear();
        skipped_tools_.clear();
    }
    processing_ = true;
    streaming_ = true;
    stream_buffer_.clear();
    pending_response_.clear();

    if (add_user_message) {
        bool was_empty = memory_.getHistory().empty();
        memory_.addMessage(Message::User, user_input);
        // opencode 风格：用第一条用户消息自动命名 session
        if (was_empty) {
            auto& sm = SessionManager::getInstance();
            std::string new_name = user_input;
            if (new_name.size() > 40) {
                auto space = new_name.find_last_of(' ', 40);
                if (space > 20) new_name = new_name.substr(0, space);
                else new_name = new_name.substr(0, 40);
                new_name += "...";
            }
            sm.renameSession(sm.activeSessionId(), new_name);
        }
    }

    auto& cfg = ConfigManager::GetInstance()->GetConfig();
    auto& ai_cfg = cfg.ai;
    const auto& active_key = ai_cfg.keys.empty()
        ? AIKeyConfig{} : ai_cfg.keys[ai_cfg.active_key % ai_cfg.keys.size()];

    AIClient::Config client_cfg;
    std::string backend_lower = active_key.backend;
    for (auto& c : backend_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    client_cfg.backend = backend_lower == "openai"
        ? AIClient::Backend::OpenAI : AIClient::Backend::Ollama;
    client_cfg.endpoint = active_key.endpoint;
    client_cfg.model = active_key.model.empty() ? active_key.name : active_key.model;
    client_cfg.api_key = active_key.api_key;
    client_cfg.tools = tools_.getToolSchemas();
    client_.setConfig(client_cfg);

    memory_.setContextLimit(ai_cfg.max_context_tokens, ai_cfg.max_response_tokens);

    Environment env;
    env.cwd = state_.currentPath;
    env.os_type = ContextBuilder::detectOS();
    env.terminal_width = 80;
    env.terminal_height = 24;

    std::string system_prompt = ContextBuilder::buildSystemPrompt(
        env, memory_.getHistory(), tools_.getToolSchemas().dump(),
        ai_cfg.system_prompt);

    auto messages = ContextBuilder::buildMessages(
        system_prompt, memory_.getHistory(), user_input);

    client_.sendMessage(
        messages,
        [this](const std::string& delta) { handleStreamChunk(delta); },
        [this](const std::string& full) { handleResponseDone(full); },
        [this](const std::string& error) { handleError(error); }
    );
}

void AIAgent::cancel() {
    client_.cancel();
    processing_ = false;
    streaming_ = false;
    needs_auto_continue_ = false;
    auto_continue_count_ = 0;
}

void AIAgent::onRenderTick() {
    if (needs_auto_continue_ && !processing_) {
        continueProcessing();
        return;
    }
    if (!processing_) return;

    if (!pending_response_.empty()
        && static_cast<int>(stream_buffer_.size()) <= stream_visible_len()) {
        processPendingResponse();
    }
}

void AIAgent::continueProcessing() {
    needs_auto_continue_ = false;
    auto_continue_count_++;

    std::string msg = "Continue the previous task. The steps above have been executed. "
        "Review the results carefully. If any step FAILED (e.g. error, empty result), "
        "you MUST retry with a corrected approach. The task is only complete when "
        "the original request has been fully accomplished. "
        "When truly done, respond with "
        "{\"thought\": \"Task completed.\", \"steps\": []}.";

    sendRequest(msg, false);
}

bool AIAgent::isProcessing() const { return processing_; }
bool AIAgent::isStreaming() const { return streaming_; }
const std::string& AIAgent::getStreamBuffer() const { return stream_buffer_; }

std::string AIAgent::getPendingResponse() const {
    return pending_response_;
}

void AIAgent::handleStreamChunk(const std::string& delta) {
    if (delta.empty()) return;
    stream_buffer_ += delta;
    if (state_.screen) {
        state_.screen->Post(ftxui::Event::Custom);
    }
}

void AIAgent::handleResponseDone(const std::string& full_response) {
    pending_response_ = full_response;
    streaming_ = false;
    if (state_.screen) {
        state_.screen->Post(ftxui::Event::Custom);
    }
}

void AIAgent::handleError(const std::string& error) {
    state_.ai.entries.push_back({AILogEntry::Error, "AI request failed: " + error});
    processing_ = false;
    streaming_ = false;
    if (state_.screen) {
        state_.screen->Post(ftxui::Event::Custom);
    }
}

void AIAgent::processPendingResponse() {
    std::string response = pending_response_;
    pending_response_.clear();

    auto ai_resp = ResponseRouter::parse(response);

    if (!ai_resp.thought.empty()) {
        state_.ai.entries.push_back({AILogEntry::Assistant, ai_resp.thought});
        memory_.addMessage(Message::Assistant, ai_resp.thought);
    }

    // Separate tools: allowed execute immediately, prompt tools queue for confirmation
    std::vector<ToolCall> allowed_calls, prompt_calls;
    for (const auto& tc : ai_resp.tool_calls) {
        auto* def = tools_.findTool(tc.name);
        if (def && def->permission == PermissionLevel::Prompt) {
            prompt_calls.push_back(tc);
        } else {
            allowed_calls.push_back(tc);
        }
    }

    int total = static_cast<int>(ai_resp.tool_calls.size());
    int idx = 0;

    for (const auto& tc : allowed_calls) {
        idx++;
        if (!processing_) {
            state_.ai.entries.push_back({AILogEntry::Error, "Execution cancelled"});
            break;
        }

        // Doom loop: skip tool if too many consecutive errors
        if (consecutive_tool_errors_[tc.name] >= max_consecutive_errors_) {
            state_.ai.entries.push_back({AILogEntry::System,
                "[skip] '" + tc.name + "' failed " +
                std::to_string(max_consecutive_errors_) +
                " consecutive times, skipping"});
            skipped_tools_.push_back(tc.name);
            continue;
        }

        state_.ai.entries.push_back({AILogEntry::Step,
            "[" + std::to_string(idx) + "/" + std::to_string(total) + "] " + tc.description});

        auto result = executor_.execute(tc);

        if (result.success) {
            consecutive_tool_errors_[tc.name] = 0;
        } else {
            consecutive_tool_errors_[tc.name]++;
        }

        if (!result.message.empty()) {
            memory_.addMessage(Message::Tool, result.message, tc.name);
        }

        if (result.log_type == "success") {
            state_.ai.entries.push_back({AILogEntry::Success, result.message});
        } else if (result.log_type == "error") {
            state_.ai.entries.push_back({AILogEntry::Error, result.message});
        } else {
            state_.ai.entries.push_back({AILogEntry::System, result.message});
        }
    }

    // Queue prompt tools for user confirmation (with doom loop check)
    {
        std::vector<ToolCall> confirmable;
        for (auto& tc : prompt_calls) {
            if (consecutive_tool_errors_[tc.name] >= max_consecutive_errors_) {
                state_.ai.entries.push_back({AILogEntry::System,
                    "[skip] '" + tc.name + "' failed " +
                    std::to_string(max_consecutive_errors_) +
                    " consecutive times, skipping"});
                skipped_tools_.push_back(tc.name);
            } else {
                confirmable.push_back(std::move(tc));
            }
        }
        prompt_calls = std::move(confirmable);
    }

    if (!prompt_calls.empty()) {
        pending_tool_calls_ = std::move(prompt_calls);
        pending_tool_idx_ = 0;
        state_.ai.waiting_confirmation = true;
        state_.ai.pending_tool_call = pending_tool_calls_[0];
        state_.ai.entries.push_back({AILogEntry::System,
            "[pending] Tool '" + pending_tool_calls_[0].name + "' needs confirmation"});
        processing_ = false;
        if (state_.screen) {
            state_.screen->Post(ftxui::Event::Custom);
        }
        return;
    }

    // Inject system feedback about skipped tools into history
    if (!skipped_tools_.empty()) {
        std::string skip_msg = "Note: the following tools were skipped due to repeated failures:";
        for (const auto& t : skipped_tools_) skip_msg += " " + t;
        skip_msg += ". Do not retry them unless explicitly asked.";
        memory_.addMessage(Message::Tool, skip_msg, "_system");
        skipped_tools_.clear();
    }

    // No prompt tools — normal completion
    if (!ai_resp.tool_calls.empty() && auto_continue_count_ < max_auto_continue_) {
        needs_auto_continue_ = true;
    }

    processing_ = false;
    if (state_.screen) {
        state_.screen->Post(ftxui::Event::Custom);
    }
}

void AIAgent::confirmCurrentTool() {
    if (pending_tool_idx_ >= static_cast<int>(pending_tool_calls_.size())) return;

    auto& tc = pending_tool_calls_[pending_tool_idx_];

    // Doom loop check for confirmed tools too
    if (consecutive_tool_errors_[tc.name] >= max_consecutive_errors_) {
        state_.ai.entries.push_back({AILogEntry::System,
            "[skip] '" + tc.name + "' failed " +
            std::to_string(max_consecutive_errors_) +
            " consecutive times, skipping"});
        skipped_tools_.push_back(tc.name);
    } else {
        state_.ai.entries.push_back({AILogEntry::Step, tc.description});
        auto result = executor_.execute(tc);
        if (result.success) {
            consecutive_tool_errors_[tc.name] = 0;
        } else {
            consecutive_tool_errors_[tc.name]++;
        }
        if (!result.message.empty()) {
            memory_.addMessage(Message::Tool, result.message, tc.name);
        }
        if (result.log_type == "success") {
            state_.ai.entries.push_back({AILogEntry::Success, result.message});
        } else if (result.log_type == "error") {
            state_.ai.entries.push_back({AILogEntry::Error, result.message});
        } else {
            state_.ai.entries.push_back({AILogEntry::System, result.message});
        }
    }

    advanceAfterConfirm();
}

void AIAgent::skipCurrentTool() {
    if (pending_tool_idx_ >= static_cast<int>(pending_tool_calls_.size())) return;

    state_.ai.entries.push_back({AILogEntry::Error,
        "Skipped by user: " + pending_tool_calls_[pending_tool_idx_].name});

    advanceAfterConfirm();
}

void AIAgent::alwaysAllowCurrentTool() {
    if (pending_tool_idx_ >= static_cast<int>(pending_tool_calls_.size())) return;

    auto* def = tools_.findTool(pending_tool_calls_[pending_tool_idx_].name);
    if (def) {
        def->permission = PermissionLevel::Allowed;
    }
    confirmCurrentTool();
}

void AIAgent::advanceAfterConfirm() {
    pending_tool_idx_++;
    if (pending_tool_idx_ < static_cast<int>(pending_tool_calls_.size())) {
        state_.ai.pending_tool_call = pending_tool_calls_[pending_tool_idx_];
        state_.ai.entries.push_back({AILogEntry::System,
            "[pending] Tool '" + pending_tool_calls_[pending_tool_idx_].name + "' needs confirmation"});
    } else {
        // Inject skip feedback before finishing
        if (!skipped_tools_.empty()) {
            std::string skip_msg = "Note: the following tools were skipped due to repeated failures:";
            for (const auto& t : skipped_tools_) skip_msg += " " + t;
            skip_msg += ". Do not retry them unless explicitly asked.";
            memory_.addMessage(Message::Tool, skip_msg, "_system");
            skipped_tools_.clear();
        }

        state_.ai.waiting_confirmation = false;
        pending_tool_calls_.clear();
        if (auto_continue_count_ < max_auto_continue_) {
            needs_auto_continue_ = true;
        }
    }
    if (state_.screen) {
        state_.screen->Post(ftxui::Event::Custom);
    }
}

void AIAgent::loadSession(const std::vector<Message>& messages) {
    memory_.clear();
    state_.ai.entries.clear();
    for (const auto& m : messages) {
        memory_.addMessage(m.role, m.content, m.name);
        if (m.role == Message::User) {
            state_.ai.entries.push_back({AILogEntry::User, m.content});
        } else if (m.role == Message::Assistant) {
            state_.ai.entries.push_back({AILogEntry::Assistant, m.content});
        } else if (m.role == Message::Tool) {
            state_.ai.entries.push_back({AILogEntry::Step,
                "[" + m.name + "] " + m.content.substr(0, 60)});
            state_.ai.entries.push_back({AILogEntry::System, m.content});
        }
    }
    state_.ai.log_scroll = 0;
    state_.ai.auto_scroll = true;
}

void AIAgent::syncToSessionManager() {
    SessionManager::getInstance().syncCurrentSession(memory_.getHistory());
}

int AIAgent::stream_visible_len() const {
    return static_cast<int>(stream_buffer_.size());
}

} // namespace FTB
