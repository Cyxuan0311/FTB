#pragma once

#include <string>
#include <vector>
#include <ftxui/component/screen_interactive.hpp>
#include "FTB/MainUI.hpp"

namespace FTB {

// 处理文件操作事件 (剪贴板、删除、选择等)
// 返回 true 表示事件已消费
bool HandleFileOperationEvent(MainState& state, const ftxui::Event& event);

}  // namespace FTB
