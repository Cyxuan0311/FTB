#include "config/ObjectPool.hpp"
#include "editor/NanoEditor.hpp"

namespace FTB {

// 静态成员定义
std::unique_ptr<ObjectPool<Editor::NanoEditor>> NanoEditorPool::instance_ = nullptr;
std::mutex NanoEditorPool::instance_mutex_;

} // namespace FTB
