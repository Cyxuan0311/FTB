#ifndef OBJECT_POOL_HPP
#define OBJECT_POOL_HPP

#include <queue>
#include <memory>
#include <mutex>
#include <functional>

// 前向声明
namespace FTB { namespace Editor { class NanoEditor; } }

namespace FTB {

/**
 * @brief 对象池模板类，用于重用频繁创建的对象
 * @tparam T 对象类型
 */
template<typename T>
class ObjectPool {
private:
    std::queue<std::unique_ptr<T>> available_objects_;
    mutable std::mutex mutex_;
    std::function<std::unique_ptr<T>()> factory_;
    size_t max_size_;
    size_t current_size_;

public:
    explicit ObjectPool(std::function<std::unique_ptr<T>()> factory, size_t max_size = 100)
        : factory_(std::move(factory)), max_size_(max_size), current_size_(0) {}

    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!available_objects_.empty()) {
            auto obj = std::move(available_objects_.front());
            available_objects_.pop();
            return obj;
        }
        if (max_size_ == 0 || current_size_ < max_size_) {
            current_size_++;
            return factory_();
        }
        return nullptr;
    }

    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        std::lock_guard<std::mutex> lock(mutex_);
        available_objects_.push(std::move(obj));
    }

    size_t available_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_objects_.size();
    }

    size_t current_size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_size_;
    }
};

/**
 * @brief NanoEditor对象池
 */
class NanoEditorPool {
private:
    static std::unique_ptr<ObjectPool<Editor::NanoEditor>> instance_;
    static std::mutex instance_mutex_;

public:
    static ObjectPool<Editor::NanoEditor>& GetInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::make_unique<ObjectPool<Editor::NanoEditor>>(
                []() { return std::make_unique<Editor::NanoEditor>(); },
                50
            );
        }
        return *instance_;
    }
};

} // namespace FTB

#endif // OBJECT_POOL_HPP
