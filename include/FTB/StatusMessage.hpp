#pragma once

#include <string>
#include <chrono>

namespace FTB {

namespace detail {

struct StatusData {
    std::string text;
    std::chrono::steady_clock::time_point expiry;
};

inline StatusData& GetStatusData() {
    static StatusData d;
    return d;
}

} // namespace detail

class StatusMessage {
public:
    static void Show(const std::string& msg, int seconds = 3) {
        auto& d = detail::GetStatusData();
        d.text = msg;
        d.expiry = std::chrono::steady_clock::now()
                 + std::chrono::seconds(seconds);
    }

    static std::string GetCurrent() {
        auto& d = detail::GetStatusData();
        if (d.text.empty())
            return {};
        if (std::chrono::steady_clock::now() >= d.expiry) {
            d.text.clear();
            return {};
        }
        return d.text;
    }

    static bool HasActive() {
        return !GetCurrent().empty();
    }

    static void Clear() {
        detail::GetStatusData().text.clear();
    }
};

} // namespace FTB
