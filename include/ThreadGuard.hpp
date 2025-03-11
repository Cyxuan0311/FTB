#ifndef THREAD_GUARD_HPP
#define THREAD_GUARD_HPP

#include <thread>

class ThreadGuard {
public:
    explicit ThreadGuard(std::thread& t);
    ~ThreadGuard();
private:
    std::thread& thread_;
};

#endif
