#include "ai/ToolRegistry.hpp"

namespace FTB {

bool ToolRegistry::registerTool(ToolDef tool) {
    if (tool_indices_.count(tool.name)) {
        return false;
    }
    tool_indices_[tool.name] = static_cast<int>(tools_.size());
    tools_.push_back(std::move(tool));
    return true;
}

ToolDef* ToolRegistry::findTool(const std::string& name) {
    auto it = tool_indices_.find(name);
    if (it != tool_indices_.end()) {
        return &tools_[it->second];
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
