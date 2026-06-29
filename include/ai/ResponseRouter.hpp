#pragma once

#include <string>
#include <vector>
#include "ai/ToolRegistry.hpp"

namespace FTB {

struct AIResponse {
    std::string thought;
    std::vector<ToolCall> tool_calls;
    bool is_text_only = true;
    bool parse_failed = false;
    std::string error_message;
};

class ResponseRouter {
public:
    static AIResponse parse(const std::string& raw_response);
    static std::string buildErrorFeedback(const std::string& error,
                                          const std::string& raw_response);
};

} // namespace FTB
