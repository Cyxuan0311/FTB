#ifndef OBJECT_POOL_HPP
#define OBJECT_POOL_HPP

#include <queue>
#include <memory>
#include <mutex>
#include <functional>

// 前向声明
class VimLikeEditor;

namespace FTB {

/**
 * @brief 对象池模板类，用于重用频繁创建的对象
 * @tparam T 对象类型
 */
template<typename T>
class ObjectPool {
private:
    std::queue<std::unique_ptr<T>> available_objects_;
    std::mutex mutex_;
    std::function<std::unique_ptr<T>()> factory_;
    size_t max_size_;
    size_t current_size_;

public:
    /**
     * @brief 构造函数
     * @param factory 对象工厂函数
     * @param max_size 最大池大小，0表示无限制
     */
    explicit ObjectPool(std::function<std::unique_ptr<T>()> factory, size_t max_size = 100)
        : factory_(std::move(factory)), max_size_(max_size), current_size_(0) {}

    /**
     * @brief 从池中获取对象
     * @return 对象智能指针
     */
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!available_objects_.empty()) {
            auto obj = std::move(available_objects_.front());
            available_objects_.pop();
            return obj;
        }
        
        // 如果池中没有可用对象，创建新对象
        if (max_size_ == 0 || current_size_ < max_size_) {
            current_size_++;
            return factory_();
        }
        
        // 如果达到最大大小，返回nullptr
        return nullptr;
    }
    
    /**
     * @brief 将对象返回到池中
     * @param obj 要返回的对象
     */
    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 重置对象状态（如果需要）
        resetObject(*obj);
        
        available_objects_.push(std::move(obj));
    }
    
    /**
     * @brief 获取池中可用对象数量
     * @return 可用对象数量
     */
    size_t available_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_objects_.size();
    }
    
    /**
     * @brief 获取当前池大小
     * @return 当前池大小
     */
    size_t current_size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_size_;
    }

private:
    /**
     * @brief 重置对象状态（子类可以重写）
     * @param obj 要重置的对象
     */
    void resetObject(T& obj) {
        // 默认实现：什么都不做
        // 子类可以重写此方法来重置对象状态
        (void)obj;  // 避免未使用参数警告
    }
};

/**
 * @brief VimLikeEditor对象池
 */
class VimEditorPool {
private:
    static std::unique_ptr<ObjectPool<VimLikeEditor>> instance_;
    static std::mutex instance_mutex_;

public:
    /**
     * @brief 获取单例实例
     * @return 对象池引用
     */
    static ObjectPool<VimLikeEditor>& getInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::make_unique<ObjectPool<VimLikeEditor>>(
                []() { return std::make_unique<VimLikeEditor>(); },
                50  // 最大50个编辑器实例
            );
        }
        return *instance_;
    }
};

} // namespace FTB

#endif // OBJECT_POOL_HPP
