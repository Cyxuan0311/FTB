#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <list>
#include <mutex>
#include <chrono>
#include <functional>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace FTB {

/**
 * @brief LRU缓存模板类，支持TTL和持久化
 * @tparam Key 键类型
 * @tparam Value 值类型
 * @tparam Hash 哈希函数类型
 */
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache {
public:
    using key_type = Key;
    using value_type = Value;
    using size_type = size_t;
    using time_point = std::chrono::system_clock::time_point;
    
    /**
     * @brief 缓存项结构
     */
    struct CacheItem {
        Value value;
        time_point last_access;
        time_point created_time;
        std::chrono::seconds ttl;
        
        CacheItem(const Value& val, std::chrono::seconds item_ttl = std::chrono::seconds(300))
            : value(val), last_access(std::chrono::system_clock::now()), 
              created_time(std::chrono::system_clock::now()), ttl(item_ttl) {}
              
        bool is_expired() const {
            return std::chrono::system_clock::now() - created_time > ttl;
        }
        
        void update_access() {
            last_access = std::chrono::system_clock::now();
        }
    };

private:
    using item_list = std::list<std::pair<Key, CacheItem>>;
    using item_map = std::unordered_map<Key, typename item_list::iterator, Hash>;
    
    mutable std::shared_mutex mutex_;  // 读写锁，提高并发性能
    item_list items_;                  // LRU链表
    item_map item_map_;               // 快速查找映射
    size_type max_size_;              // 最大缓存大小
    std::chrono::seconds default_ttl_; // 默认TTL
    bool enable_persistence_;         // 是否启用持久化
    std::string persistence_file_;    // 持久化文件路径
    
    // 序列化/反序列化函数
    std::function<std::string(const Key&)> key_serializer_;
    std::function<std::string(const Value&)> value_serializer_;
    std::function<Key(const std::string&)> key_deserializer_;
    std::function<Value(const std::string&)> value_deserializer_;

public:
    /**
     * @brief 构造函数
     * @param max_size 最大缓存大小
     * @param default_ttl 默认TTL时间
     * @param enable_persistence 是否启用持久化
     * @param persistence_file 持久化文件路径
     */
    explicit LRUCache(size_type max_size = 1000, 
                     std::chrono::seconds default_ttl = std::chrono::seconds(300),
                     bool enable_persistence = false,
                     const std::string& persistence_file = "")
        : max_size_(max_size), default_ttl_(default_ttl), 
          enable_persistence_(enable_persistence), persistence_file_(persistence_file) {
        
        if (enable_persistence_ && !persistence_file_.empty()) {
            load_from_disk();
        }
    }
    
    /**
     * @brief 设置序列化函数
     */
    void set_serializers(
        std::function<std::string(const Key&)> key_ser,
        std::function<std::string(const Value&)> value_ser,
        std::function<Key(const std::string&)> key_deser,
        std::function<Value(const std::string&)> value_deser) {
        key_serializer_ = key_ser;
        value_serializer_ = value_ser;
        key_deserializer_ = key_deser;
        value_deserializer_ = value_deser;
    }
    
    /**
     * @brief 获取缓存值
     * @param key 键
     * @return 可选值，如果不存在或过期则返回空
     */
    std::optional<Value> get(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        auto it = item_map_.find(key);
        if (it == item_map_.end()) {
            return std::nullopt;
        }
        
        auto& item = it->second->second;
        if (item.is_expired()) {
            // 过期项将在下次清理时删除
            return std::nullopt;
        }
        
        item.update_access();
        return item.value;
    }
    
    /**
     * @brief 设置缓存值
     * @param key 键
     * @param value 值
     * @param ttl 可选的TTL时间
     */
    void put(const Key& key, const Value& value, 
             std::optional<std::chrono::seconds> ttl = std::nullopt) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
            // 更新现有项
            it->second->second.value = value;
            it->second->second.update_access();
            it->second->second.ttl = ttl.value_or(default_ttl_);
            move_to_front(it->second);
        } else {
            // 添加新项
            if (items_.size() >= max_size_) {
                evict_lru();
            }
            
            auto ttl_time = ttl.value_or(default_ttl_);
            items_.emplace_front(key, CacheItem(value, ttl_time));
            item_map_[key] = items_.begin();
        }
        
        if (enable_persistence_) {
            save_to_disk_async();
        }
    }
    
    /**
     * @brief 删除缓存项
     * @param key 键
     * @return 是否成功删除
     */
    bool erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        auto it = item_map_.find(key);
        if (it == item_map_.end()) {
            return false;
        }
        
        items_.erase(it->second);
        item_map_.erase(it);
        
        if (enable_persistence_) {
            save_to_disk_async();
        }
        
        return true;
    }
    
    /**
     * @brief 检查键是否存在且未过期
     * @param key 键
     * @return 是否存在
     */
    bool contains(const Key& key) const {
        return get(key).has_value();
    }
    
    /**
     * @brief 获取缓存大小
     * @return 当前缓存项数量
     */
    size_type size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return items_.size();
    }
    
    /**
     * @brief 检查缓存是否为空
     * @return 是否为空
     */
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return items_.empty();
    }
    
    /**
     * @brief 清空缓存
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        items_.clear();
        item_map_.clear();
        
        if (enable_persistence_) {
            save_to_disk_async();
        }
    }
    
    /**
     * @brief 清理过期项
     * @return 清理的项数量
     */
    size_type cleanup_expired() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        size_type cleaned = 0;
        auto it = items_.begin();
        while (it != items_.end()) {
            if (it->second.is_expired()) {
                item_map_.erase(it->first);
                it = items_.erase(it);
                cleaned++;
            } else {
                ++it;
            }
        }
        
        if (cleaned > 0 && enable_persistence_) {
            save_to_disk_async();
        }
        
        return cleaned;
    }
    
    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStats {
        size_type size;
        size_type max_size;
        size_type expired_count;
        double hit_ratio;
        time_point last_cleanup;
    };
    
    CacheStats get_stats() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        CacheStats stats{};
        stats.size = items_.size();
        stats.max_size = max_size_;
        
        size_type expired = 0;
        for (const auto& item : items_) {
            if (item.second.is_expired()) {
                expired++;
            }
        }
        stats.expired_count = expired;
        
        // 这里可以添加命中率统计逻辑
        stats.hit_ratio = 0.0; // 简化实现
        
        return stats;
    }

private:
    /**
     * @brief 将项移动到链表前端
     */
    void move_to_front(typename item_list::iterator it) {
        if (it != items_.begin()) {
            items_.splice(items_.begin(), items_, it);
        }
    }
    
    /**
     * @brief 驱逐最久未使用的项
     */
    void evict_lru() {
        if (!items_.empty()) {
            auto last = std::prev(items_.end());
            item_map_.erase(last->first);
            items_.erase(last);
        }
    }
    
    /**
     * @brief 异步保存到磁盘
     */
    void save_to_disk_async() {
        if (!enable_persistence_ || persistence_file_.empty()) {
            return;
        }
        
        // 在后台线程中保存，避免阻塞
        std::thread([this]() {
            save_to_disk();
        }).detach();
    }
    
    /**
     * @brief 保存缓存到磁盘
     */
    void save_to_disk() {
        if (!enable_persistence_ || persistence_file_.empty() || 
            !key_serializer_ || !value_serializer_) {
            return;
        }
        
        try {
            std::ofstream file(persistence_file_);
            if (!file.is_open()) {
                return;
            }
            
            std::shared_lock<std::shared_mutex> lock(mutex_);
            
            file << items_.size() << "\n";
            for (const auto& item : items_) {
                if (!item.second.is_expired()) {
                    file << key_serializer_(item.first) << "\n";
                    file << value_serializer_(item.second.value) << "\n";
                    file << std::chrono::duration_cast<std::chrono::seconds>(
                        item.second.created_time.time_since_epoch()).count() << "\n";
                    file << item.second.ttl.count() << "\n";
                }
            }
        } catch (const std::exception& e) {
            // 静默处理保存错误
        }
    }
    
    /**
     * @brief 从磁盘加载缓存
     */
    void load_from_disk() {
        if (!enable_persistence_ || persistence_file_.empty() || 
            !key_deserializer_ || !value_deserializer_) {
            return;
        }
        
        if (!std::filesystem::exists(persistence_file_)) {
            return;
        }
        
        try {
            std::ifstream file(persistence_file_);
            if (!file.is_open()) {
                return;
            }
            
            std::unique_lock<std::shared_mutex> lock(mutex_);
            
            size_type count;
            file >> count;
            
            for (size_type i = 0; i < count; ++i) {
                std::string key_str, value_str;
                long long created_seconds, ttl_seconds;
                
                if (!(file >> key_str >> value_str >> created_seconds >> ttl_seconds)) {
                    break;
                }
                
                Key key = key_deserializer_(key_str);
                Value value = value_deserializer_(value_str);
                
                time_point created_time(std::chrono::seconds(created_seconds));
                std::chrono::seconds ttl(ttl_seconds);
                
                // 检查是否过期
                if (std::chrono::system_clock::now() - created_time < ttl) {
                    items_.emplace_front(key, CacheItem(value, ttl));
                    items_.front().second.created_time = created_time;
                    item_map_[key] = items_.begin();
                }
            }
        } catch (const std::exception& e) {
            // 静默处理加载错误
        }
    }
};

} // namespace FTB

#endif // LRU_CACHE_HPP
