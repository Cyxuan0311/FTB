#include "FTB/AsyncFileManager.hpp"
#include "FTB/FileManager.hpp"
#include <iostream>

namespace FTB {

// 静态成员定义
std::unique_ptr<AsyncFileManager> GlobalAsyncFileManager::instance_ = nullptr;
std::mutex GlobalAsyncFileManager::instance_mutex_;

AsyncFileManager::AsyncFileManager() = default;

AsyncFileManager::~AsyncFileManager() {
    stop();
}

void AsyncFileManager::start() {
    if (running_.load()) {
        return;
    }
    
    should_stop_.store(false);
    running_.store(true);
    worker_thread_ = std::thread(&AsyncFileManager::workerLoop, this);
}

void AsyncFileManager::stop() {
    if (!running_.load()) {
        return;
    }
    
    should_stop_.store(true);
    cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    running_.store(false);
}

void AsyncFileManager::asyncGetDirectoryContents(const std::string& path, 
                                                  std::function<void(std::vector<std::string>)> callback) {
    enqueueTask([path, callback]() {
        try {
            auto contents = FileManager::getDirectoryContents(path);
            callback(contents);
        } catch (const std::exception& e) {
            std::cerr << "异步获取目录内容失败: " << e.what() << std::endl;
            callback(std::vector<std::string>());
        }
    });
}

void AsyncFileManager::asyncReadFileContent(const std::string& file_path, 
                                             int start_line, int end_line,
                                             std::function<void(std::string)> callback) {
    enqueueTask([file_path, start_line, end_line, callback]() {
        try {
            auto content = FileManager::readFileContent(file_path, start_line, end_line);
            callback(content);
        } catch (const std::exception& e) {
            std::cerr << "异步读取文件内容失败: " << e.what() << std::endl;
            callback("");
        }
    });
}

void AsyncFileManager::asyncWriteFileContent(const std::string& file_path, 
                                              const std::string& content,
                                              std::function<void(bool)> callback) {
    enqueueTask([file_path, content, callback]() {
        try {
            bool success = FileManager::writeFileContent(file_path, content);
            callback(success);
        } catch (const std::exception& e) {
            std::cerr << "异步写入文件内容失败: " << e.what() << std::endl;
            callback(false);
        }
    });
}

void AsyncFileManager::asyncDeleteFileOrDirectory(const std::string& path,
                                                   std::function<void(bool)> callback) {
    enqueueTask([path, callback]() {
        try {
            bool success = FileManager::deleteFileOrDirectory(path);
            callback(success);
        } catch (const std::exception& e) {
            std::cerr << "异步删除文件或目录失败: " << e.what() << std::endl;
            callback(false);
        }
    });
}

void AsyncFileManager::asyncCreateFile(const std::string& file_path,
                                        std::function<void(bool)> callback) {
    enqueueTask([file_path, callback]() {
        try {
            bool success = FileManager::createFile(file_path);
            callback(success);
        } catch (const std::exception& e) {
            std::cerr << "异步创建文件失败: " << e.what() << std::endl;
            callback(false);
        }
    });
}

void AsyncFileManager::asyncCreateDirectory(const std::string& dir_path,
                                             std::function<void(bool)> callback) {
    enqueueTask([dir_path, callback]() {
        try {
            bool success = FileManager::createDirectory(dir_path);
            callback(success);
        } catch (const std::exception& e) {
            std::cerr << "异步创建目录失败: " << e.what() << std::endl;
            callback(false);
        }
    });
}

size_t AsyncFileManager::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
    return task_queue_.size();
}

void AsyncFileManager::workerLoop() {
    while (!should_stop_.load()) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { 
                return !task_queue_.empty() || should_stop_.load(); 
            });
            
            if (should_stop_.load()) {
                break;
            }
            
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "异步任务执行失败: " << e.what() << std::endl;
            }
        }
    }
}

void AsyncFileManager::enqueueTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(std::move(task));
    }
    cv_.notify_one();
}

// GlobalAsyncFileManager 实现
AsyncFileManager& GlobalAsyncFileManager::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<AsyncFileManager>();
        instance_->start();
    }
    return *instance_;
}

void GlobalAsyncFileManager::initialize() {
    getInstance();  // 这会自动创建并启动实例
}

void GlobalAsyncFileManager::cleanup() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_) {
        instance_->stop();
        instance_.reset();
    }
}

} // namespace FTB
