#include "ai/ToolRegistry.hpp"

namespace FTB {

void ToolRegistry::registerTool(ToolDef tool) {
    tools_.push_back(std::move(tool));
}

ToolDef* ToolRegistry::findTool(const std::string& name) {
    for (auto& t : tools_) {
        if (t.name == name) return &t;
    }
    return nullptr;
}

json ToolRegistry::getToolSchemas() const {
    json arr = json::array();
    for (const auto& t : tools_) {
        arr.push_back({
            {"type", "function"},
            {"function", {
                {"name", t.name},
                {"description", t.description},
                {"parameters", t.parameters_schema}
            }}
        });
    }
    return arr;
}

const std::vector<ToolDef>& ToolRegistry::allTools() const {
    return tools_;
}

void ToolRegistry::registerDefaultTools() {
    // 注册空的默认工具集，实际注册由 ActionExecutor 完成
}

} // namespace FTB
