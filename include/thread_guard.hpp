#pragma once
#include <thread>

class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t);
    ~ThreadGuard();
    
    // 删除拷贝构造和赋值操作
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

private:
    std::thread& thread_;
};