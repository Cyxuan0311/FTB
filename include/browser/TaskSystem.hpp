#ifndef TASK_SYSTEM_HPP
#define TASK_SYSTEM_HPP

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <chrono>
#include <map>
#include <cstdint>

enum class TaskState { Pending, Running, Paused, Completed, Failed, Cancelled };
enum class TaskType { Copy, Move, Delete, Extract, Rename, BulkRename };
enum class Priority { Low, Normal, High };

struct Progress {
    std::atomic<uintmax_t> bytes_processed{0};
    std::atomic<uintmax_t> total_bytes{0};
    std::atomic<int> files_processed{0};
    std::atomic<int> total_files{0};
    std::string current_file;
    std::string current_target;
};

struct TaskSnapshot {
    std::string id;
    std::string title;
    TaskState state;
    TaskType type;
    uintmax_t bytes_processed;
    uintmax_t total_bytes;
    int files_processed;
    int total_files;
    double current_speed;
    std::string current_file;
    std::string current_target;
};

struct TaskContext {
    Progress& progress;
    const std::atomic<bool>& cancel;
    const std::atomic<bool>& pause;
};

using WorkFunction = std::function<bool(TaskContext& ctx)>;
using TaskCallback = std::function<void(const std::string& task_id, TaskState state)>;

struct TaskRequest {
    std::string title;
    TaskType type;
    Priority priority = Priority::Normal;
    WorkFunction work;
    std::function<void()> cleanup = nullptr;
    TaskCallback callback = nullptr;
};

class TaskSystem {
public:
    static TaskSystem& getInstance();

    std::string submit(TaskRequest request);
    bool cancel(const std::string& task_id);
    bool pause(const std::string& task_id);
    bool resume(const std::string& task_id);

    std::vector<TaskSnapshot> get_snapshot() const;
    size_t active_count() const;
    size_t queue_depth() const;

    void set_max_workers(size_t n);
    void set_speed_limit(uintmax_t bytes_per_sec);
    uintmax_t get_speed_limit() const;

    void start();
    void stop();

private:
    TaskSystem() = default;
    ~TaskSystem();
    TaskSystem(const TaskSystem&) = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    struct RunningTask {
        std::string title;
        TaskType type;
        TaskState state = TaskState::Running;
        std::shared_ptr<Progress> progress;
        std::shared_ptr<std::atomic<bool>> cancel;
        std::shared_ptr<std::atomic<bool>> pause;
        std::chrono::steady_clock::time_point start_time;
    };

    struct QueuedEntry {
        std::string task_id;
        std::shared_ptr<TaskRequest> request;
    };

    void worker_loop();
    std::shared_ptr<QueuedEntry> pop_next();
    void store_history(const std::string& task_id, const std::string& title,
                       TaskType type, TaskState state, Progress& progress,
                       std::chrono::steady_clock::time_point start) const;
    void remove_running(const std::string& task_id);

    std::vector<std::thread> workers_;
    std::deque<QueuedEntry> high_queue_;
    std::deque<QueuedEntry> normal_queue_;
    std::deque<QueuedEntry> low_queue_;

    std::map<std::string, RunningTask> running_tasks_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_stop_{false};

    size_t max_workers_{4};
    std::atomic<uintmax_t> speed_limit_{0};
    std::atomic<uint64_t> next_id_{1};

    mutable std::mutex history_mutex_;
    mutable std::deque<TaskSnapshot> recent_history_;
    static constexpr size_t MAX_HISTORY = 50;
};

#endif
