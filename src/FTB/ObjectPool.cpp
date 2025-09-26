#include "FTB/ObjectPool.hpp"
#include "FTB/Vim/Vim_Like.hpp"

namespace FTB {

// 静态成员定义
std::unique_ptr<ObjectPool<VimLikeEditor>> VimEditorPool::instance_ = nullptr;
std::mutex VimEditorPool::instance_mutex_;

} // namespace FTB


