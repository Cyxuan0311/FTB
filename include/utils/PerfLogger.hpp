#pragma once

#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>

namespace FTB {

class PerfLogger {
public:
    static PerfLogger& GetInstance() {
        static PerfLogger instance;
        return instance;
    }

    // 启用/禁用日志 (由 -l 命令行参数控制)
    static void Enable() {
        GetInstance().enabled_.store(true);
    }

    static void Disable() {
        GetInstance().enabled_.store(false);
    }

    static bool IsEnabled() {
        return GetInstance().enabled_.load();
    }

    void Log(const std::string& tag, const std::string& message) {
        if (!enabled_.load()) return;

        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("ftb_perf.log", std::ios::out | std::ios::trunc);
            start_time_ = ms;
        }

        auto elapsed = ms - start_time_;
        file_ << "[" << elapsed << "ms] [" << tag << "] " << message << "\n";
        file_.flush();
    }

    // 计时作用域辅助类
    class Timer {
    public:
        Timer(const std::string& tag, const std::string& name)
            : tag_(tag), name_(name), start_(std::chrono::steady_clock::now()) {}

        ~Timer() {
            if (!GetInstance().enabled_.load()) return;

            auto end = std::chrono::steady_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
            std::ostringstream oss;
            oss << name_ << " -> " << us << " us";
            if (us > 1000) {
                oss << " (" << (us / 1000.0) << " ms)";
            }
            GetInstance().Log(tag_, oss.str());
        }

    private:
        std::string tag_;
        std::string name_;
        std::chrono::steady_clock::time_point start_;
    };

private:
    PerfLogger() = default;
    std::ofstream file_;
    std::mutex mutex_;
    std::atomic<bool> enabled_{false};
    long long start_time_ = 0;
};

} // namespace FTB

// 快捷宏 (enabled when -l flag is set)
#define PERF_LOG(tag, msg) FTB::PerfLogger::GetInstance().Log(tag, msg)
#define PERF_TIMER(tag, name) FTB::PerfLogger::Timer timer_##__LINE__(tag, name)
#define PERF_SCOPE(tag) FTB::PerfLogger::Timer timer_##__LINE__(tag, __func__)
