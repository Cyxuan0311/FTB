#include "browser/TaskSystem.hpp"
#include <algorithm>
#include <thread>

TaskSystem& TaskSystem::getInstance() {
    static TaskSystem instance;
    return instance;
}

TaskSystem::~TaskSystem() {
    stop();
}

void TaskSystem::start() {
    if (running_.load()) return;
    should_stop_.store(false);
    running_.store(true);

    size_t n = std::max<size_t>(1, max_workers_);
    workers_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        workers_.emplace_back(&TaskSystem::worker_loop, this);
    }
}

void TaskSystem::stop() {
    if (!running_.load()) return;
    should_stop_.store(true);
    cv_.notify_all();

    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
    workers_.clear();
    running_.store(false);
}

std::string TaskSystem::submit(TaskRequest request) {
    if (!running_.load()) {
        start();
    }

    auto req = std::make_shared<TaskRequest>(std::move(request));
    std::string task_id;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_id = std::to_string(next_id_++);

        RunningTask rt;
        rt.title = req->title;
        rt.type = req->type;
        rt.state = TaskState::Pending;
        rt.progress = std::make_shared<Progress>();
        rt.cancel = std::make_shared<std::atomic<bool>>(false);
        rt.pause = std::make_shared<std::atomic<bool>>(false);
        rt.start_time = std::chrono::steady_clock::now();
        running_tasks_[task_id] = std::move(rt);

        switch (req->priority) {
        case Priority::High:
            high_queue_.push_back({task_id, std::move(req)});
            break;
        case Priority::Low:
            low_queue_.push_back({task_id, std::move(req)});
            break;
        default:
            normal_queue_.push_back({task_id, std::move(req)});
            break;
        }
    }
    cv_.notify_one();
    return task_id;
}

bool TaskSystem::cancel(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto it = running_tasks_.find(task_id);
    if (it == running_tasks_.end()) return false;
    it->second.cancel->store(true);
    it->second.state = TaskState::Cancelled;
    cv_.notify_all();
    return true;
}

bool TaskSystem::pause(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto it = running_tasks_.find(task_id);
    if (it == running_tasks_.end()) return false;
    it->second.pause->store(true);
    it->second.state = TaskState::Paused;
    return true;
}

bool TaskSystem::resume(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto it = running_tasks_.find(task_id);
    if (it == running_tasks_.end()) return false;
    it->second.pause->store(false);
    it->second.state = TaskState::Running;
    cv_.notify_all();
    return true;
}

std::vector<TaskSnapshot> TaskSystem::get_snapshot() const {
    auto now = std::chrono::steady_clock::now();
    std::vector<TaskSnapshot> result;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (const auto& [id, rt] : running_tasks_) {
            double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
                now - rt.start_time).count();
            double speed = (elapsed > 0.001)
                ? static_cast<double>(rt.progress->bytes_processed.load()) / elapsed
                : 0.0;

            result.push_back({
                id,
                rt.title,
                rt.state,
                rt.type,
                rt.progress->bytes_processed.load(),
                rt.progress->total_bytes.load(),
                rt.progress->files_processed.load(),
                rt.progress->total_files.load(),
                speed,
                rt.progress->current_file,
                rt.progress->current_target
            });
        }
    }

    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        result.insert(result.end(), recent_history_.begin(), recent_history_.end());
    }

    return result;
}

size_t TaskSystem::active_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    size_t count = 0;
    for (const auto& [id, rt] : running_tasks_) {
        if (rt.state == TaskState::Running || rt.state == TaskState::Paused) {
            ++count;
        }
    }
    return count;
}

size_t TaskSystem::queue_depth() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return high_queue_.size() + normal_queue_.size() + low_queue_.size();
}

void TaskSystem::set_max_workers(size_t n) {
    max_workers_ = std::max<size_t>(1, n);
}

void TaskSystem::set_speed_limit(uintmax_t bytes_per_sec) {
    speed_limit_.store(bytes_per_sec);
}

uintmax_t TaskSystem::get_speed_limit() const {
    return speed_limit_.load();
}

std::shared_ptr<TaskSystem::QueuedEntry> TaskSystem::pop_next() {
    auto pop_one = [](auto& q) -> std::shared_ptr<QueuedEntry> {
        if (q.empty()) return nullptr;
        auto entry = std::make_shared<QueuedEntry>(std::move(q.front()));
        q.pop_front();
        return entry;
    };

    auto e = pop_one(high_queue_);
    if (e) return e;
    e = pop_one(normal_queue_);
    if (e) return e;
    return pop_one(low_queue_);
}

void TaskSystem::worker_loop() {
    while (!should_stop_.load()) {
        std::shared_ptr<QueuedEntry> entry;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] {
                return should_stop_.load()
                    || !high_queue_.empty()
                    || !normal_queue_.empty()
                    || !low_queue_.empty();
            });

            if (should_stop_.load()) break;
            entry = pop_next();
        }

        if (!entry || !entry->request) continue;

        const std::string& task_id = entry->task_id;
        auto& request = entry->request;
        auto work_start = std::chrono::steady_clock::now();

        // Look up running task data
        std::shared_ptr<Progress> progress;
        std::shared_ptr<std::atomic<bool>> cancel_flag;
        std::shared_ptr<std::atomic<bool>> pause_flag;

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            auto it = running_tasks_.find(task_id);
            if (it != running_tasks_.end()) {
                progress = it->second.progress;
                cancel_flag = it->second.cancel;
                pause_flag = it->second.pause;
                it->second.state = TaskState::Running;
                it->second.start_time = work_start;
            }
        }

        if (!progress) continue;

        // Check cancel before starting
        if (cancel_flag->load()) {
            if (request->cleanup) request->cleanup();
            store_history(task_id, request->title, request->type,
                          TaskState::Cancelled, *progress, work_start);
            remove_running(task_id);
            if (request->callback) request->callback(task_id, TaskState::Cancelled);
            continue;
        }

        // Check pause before starting — busy-wait with condition variable
        if (pause_flag->load()) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                auto it = running_tasks_.find(task_id);
                if (it != running_tasks_.end()) {
                    it->second.state = TaskState::Paused;
                }
            }

            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this, pause_flag, cancel_flag] {
                return should_stop_.load()
                    || !pause_flag->load()
                    || cancel_flag->load();
            });

            if (should_stop_.load() || cancel_flag->load()) {
                if (cancel_flag->load() && request->cleanup) request->cleanup();
                store_history(task_id, request->title, request->type,
                              cancel_flag->load() ? TaskState::Cancelled : TaskState::Pending,
                              *progress, work_start);
                remove_running(task_id);
                if (request->callback)
                    request->callback(task_id, cancel_flag->load() ? TaskState::Cancelled : TaskState::Pending);
                continue;
            }

            auto it = running_tasks_.find(task_id);
            if (it != running_tasks_.end()) {
                it->second.state = TaskState::Running;
            }
        }

        // Execute the work function
        TaskContext ctx{*progress, *cancel_flag, *pause_flag};
        bool success = false;
        try {
            success = request->work(ctx);
        } catch (const std::exception&) {
            success = false;
        }

        // Determine final state
        if (cancel_flag->load()) {
            store_history(task_id, request->title, request->type,
                          TaskState::Cancelled, *progress, work_start);
            remove_running(task_id);
            if (request->cleanup) request->cleanup();
            if (request->callback) request->callback(task_id, TaskState::Cancelled);
        } else if (!success) {
            store_history(task_id, request->title, request->type,
                          TaskState::Failed, *progress, work_start);
            remove_running(task_id);
            if (request->cleanup) request->cleanup();
            if (request->callback) request->callback(task_id, TaskState::Failed);
        } else {
            store_history(task_id, request->title, request->type,
                          TaskState::Completed, *progress, work_start);
            remove_running(task_id);
            if (request->callback) request->callback(task_id, TaskState::Completed);
        }
    }
}

void TaskSystem::store_history(const std::string& task_id, const std::string& title,
                                TaskType type, TaskState state,
                                Progress& progress,
                                std::chrono::steady_clock::time_point start) const {
    double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - start).count();
    double speed = (elapsed > 0.001)
        ? static_cast<double>(progress.bytes_processed.load()) / elapsed
        : 0.0;

    TaskSnapshot snap{
        task_id, title, state, type,
        progress.bytes_processed.load(), progress.total_bytes.load(),
        progress.files_processed.load(), progress.total_files.load(),
        speed,
        progress.current_file, progress.current_target
    };

    std::lock_guard<std::mutex> lock(history_mutex_);
    recent_history_.push_back(std::move(snap));
    if (recent_history_.size() > MAX_HISTORY) {
        recent_history_.pop_front();
    }
}

void TaskSystem::remove_running(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    running_tasks_.erase(task_id);
}
