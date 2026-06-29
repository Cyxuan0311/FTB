#pragma once

#include <string>
#include <vector>
#include "ai/AIClient.hpp"

namespace FTB {

struct Environment {
    std::string cwd;
    std::string os_type;
    int terminal_width = 80;
    int terminal_height = 24;
};

class ContextBuilder {
public:
    static std::string detectOS();
    static std::string buildSystemPrompt(
        const Environment& env,
        const std::vector<Message>& history,
        const std::string& tool_schemas_json,
        const std::string& custom_prompt);

    static std::vector<Message> buildMessages(
        const std::string& system_prompt,
        const std::vector<Message>& history,
        const std::string& user_input);
};

} // namespace FTB
