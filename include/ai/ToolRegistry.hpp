#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace FTB {

using json = nlohmann::json;

struct ToolCall {
    std::string name;
    json params;
    std::string description;
};

enum class PermissionLevel {
    Allowed,
    Prompt,
    Denied
};

struct ToolDef {
    std::string name;
    std::string description;
    json parameters_schema;
    std::function<json(const json& params)> executor;
    bool dangerous = false;
    PermissionLevel permission = PermissionLevel::Allowed;
};

class ToolRegistry {
public:
    // Returns true on success, false if name already registered
    bool registerTool(ToolDef tool);
    ToolDef* findTool(const std::string& name);
    json getToolSchemas() const;
    const std::vector<ToolDef>& allTools() const;
    void registerDefaultTools();

private:
    std::unordered_map<std::string, int> tool_indices_;
    std::vector<ToolDef> tools_;
};

} // namespace FTB
