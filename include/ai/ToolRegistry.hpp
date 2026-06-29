#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace FTB {

using json = nlohmann::json;

struct ToolCall {
    std::string name;
    json params;
    std::string description;
};

struct ToolDef {
    std::string name;
    std::string description;
    json parameters_schema;
    std::function<json(const json& params)> executor;
    bool dangerous = false;
};

class ToolRegistry {
public:
    void registerTool(ToolDef tool);
    ToolDef* findTool(const std::string& name);
    json getToolSchemas() const;
    const std::vector<ToolDef>& allTools() const;
    void registerDefaultTools();

private:
    std::vector<ToolDef> tools_;
};

} // namespace FTB
