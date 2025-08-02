#include "../include/FTB/ThreadGuard.hpp"

ThreadGuard::ThreadGuard(std::thread& t) : thread_(t) {}

ThreadGuard::~ThreadGuard() {
    if (thread_.joinable())
        thread_.join();
}
