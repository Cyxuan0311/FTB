#ifndef THREAD_GUARD_HPP
#define THREAD_GUARD_HPP

#include <thread>

class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t);
    ~ThreadGuard();

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
    
private:
    std::thread& thread_;
};

#endif // THREAD_GUARD_HPP
