#ifndef ASYNC_FILE_MANAGER_HPP
#define ASYNC_FILE_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

namespace FTB {

/**
 * @brief 异步文件管理器，用于非阻塞文件操作
 */
class AsyncFileManager {
private:
    std::thread worker_thread_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_stop_{false};

public:
    AsyncFileManager();
    ~AsyncFileManager();

    /**
     * @brief 启动异步文件管理器
     */
    void start();

    /**
     * @brief 停止异步文件管理器
     */
    void stop();

    /**
     * @brief 异步获取目录内容
     * @param path 目录路径
     * @param callback 回调函数，参数为目录内容列表
     */
    void asyncGetDirectoryContents(const std::string& path, 
                                   std::function<void(std::vector<std::string>)> callback);

    /**
     * @brief 异步读取文件内容
     * @param file_path 文件路径
     * @param start_line 起始行号
     * @param end_line 结束行号
     * @param callback 回调函数，参数为文件内容
     */
    void asyncReadFileContent(const std::string& file_path, 
                              int start_line, int end_line,
                              std::function<void(std::string)> callback);

    /**
     * @brief 异步写入文件内容
     * @param file_path 文件路径
     * @param content 文件内容
     * @param callback 回调函数，参数为是否成功
     */
    void asyncWriteFileContent(const std::string& file_path, 
                               const std::string& content,
                               std::function<void(bool)> callback);

    /**
     * @brief 异步删除文件或目录
     * @param path 文件或目录路径
     * @param callback 回调函数，参数为是否成功
     */
    void asyncDeleteFileOrDirectory(const std::string& path,
                                    std::function<void(bool)> callback);

    /**
     * @brief 异步创建文件
     * @param file_path 文件路径
     * @param callback 回调函数，参数为是否成功
     */
    void asyncCreateFile(const std::string& file_path,
                         std::function<void(bool)> callback);

    /**
     * @brief 异步创建目录
     * @param dir_path 目录路径
     * @param callback 回调函数，参数为是否成功
     */
    void asyncCreateDirectory(const std::string& dir_path,
                              std::function<void(bool)> callback);

    /**
     * @brief 获取队列中待处理任务数量
     * @return 待处理任务数量
     */
    size_t getPendingTaskCount() const;

private:
    /**
     * @brief 工作线程主循环
     */
    void workerLoop();

    /**
     * @brief 添加任务到队列
     * @param task 要执行的任务
     */
    void enqueueTask(std::function<void()> task);
};

/**
 * @brief 全局异步文件管理器单例
 */
class GlobalAsyncFileManager {
private:
    static std::unique_ptr<AsyncFileManager> instance_;
    static std::mutex instance_mutex_;

public:
    /**
     * @brief 获取单例实例
     * @return 异步文件管理器引用
     */
    static AsyncFileManager& getInstance();

    /**
     * @brief 初始化全局实例
     */
    static void initialize();

    /**
     * @brief 清理全局实例
     */
    static void cleanup();
};

} // namespace FTB

#endif // ASYNC_FILE_MANAGER_HPP


