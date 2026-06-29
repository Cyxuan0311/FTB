#include "ai/ResponseRouter.hpp"
#include <nlohmann/json.hpp>
#include <regex>

namespace FTB {

AIResponse ResponseRouter::parse(const std::string& raw_response) {
    AIResponse result;
    std::string text = raw_response;

    // 提取 ```json ... ``` 代码块
    auto code_start = text.find("```json");
    if (code_start != std::string::npos) {
        auto code_end = text.find("```", code_start + 7);
        if (code_end != std::string::npos) {
            text = text.substr(code_start + 7, code_end - code_start - 7);
        }
    } else {
        code_start = text.find("```");
        if (code_start != std::string::npos) {
            auto code_end = text.find("```", code_start + 3);
            if (code_end != std::string::npos) {
                text = text.substr(code_start + 3, code_end - code_start - 3);
            }
        }
    }

    auto first_non_space = text.find_first_not_of(" \t\n\r");
    if (first_non_space == std::string::npos) {
        result.thought = raw_response;
        result.is_text_only = true;
        return result;
    }
    text = text.substr(first_non_space);

    try {
        auto root = json::parse(text);

        if (root.contains("thought") && root["thought"].is_string()) {
            result.thought = root["thought"].get<std::string>();
        }

        if (root.contains("steps") && root["steps"].is_array()) {
            for (const auto& step : root["steps"]) {
                if (!step.contains("action")) continue;
                ToolCall tc;
                tc.name = step["action"].get<std::string>();
                tc.params = step.value("params", json::object());
                tc.description = step.value("description", tc.name);
                result.tool_calls.push_back(std::move(tc));
            }
            result.is_text_only = result.tool_calls.empty();
        } else {
            result.is_text_only = true;
        }
    } catch (...) {
        // JSON 解析失败，尝试 fallback：解析 <tool_call>action{...} 格式
        result.parse_failed = true;
        auto re = std::regex(R"(<tool_call>(\w+)\s*(\{.*?\})\s*)");
        std::smatch m;
        std::string::const_iterator search_start(raw_response.cbegin());
        while (std::regex_search(search_start, raw_response.cend(), m, re)) {
            if (m.size() >= 3) {
                ToolCall tc;
                tc.name = m[1].str();
                try {
                    tc.params = json::parse(m[2].str());
                } catch (...) {
                    tc.params = json::object();
                }
                tc.description = tc.name;
                result.tool_calls.push_back(std::move(tc));
            }
            search_start = m.suffix().first;
        }

        if (result.tool_calls.empty()) {
            result.thought = raw_response;
            result.is_text_only = true;
        } else {
            // 从原始响应中提取 thought（去掉 <tool_call> 部分）
            result.thought = std::regex_replace(raw_response, re, "");
            result.thought.erase(result.thought.find_last_not_of(" \t\n\r") + 1);
            result.is_text_only = false;
        }
    }

    return result;
}

std::string ResponseRouter::buildErrorFeedback(const std::string& error,
                                                const std::string& raw_response) {
    return "Error processing your response: " + error
         + "\nOriginal response was: " + raw_response.substr(0, 200);
}

} // namespace FTB
