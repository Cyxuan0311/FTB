#include "../../include/protocols/ImageOutputManager.hpp"

#include "../../include/protocols/KittyProtocol.hpp"
#include "../../include/protocols/ITerm2Protocol.hpp"
#include "../../include/protocols/SixelProtocol.hpp"
#include "../../include/utils/PerfLogger.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

namespace FTB {

// ---- static members ----

std::unique_ptr<TerminalImageProtocol> ImageOutputManager::s_protocol;

std::string ImageOutputManager::s_cached_path;
std::string ImageOutputManager::s_cached_data;
int ImageOutputManager::s_term_rows = 0;
int ImageOutputManager::s_term_cols = 0;
int ImageOutputManager::s_image_rows = 0;
int ImageOutputManager::s_max_image_rows = 0;
int ImageOutputManager::s_panel_width = 0;

int ImageOutputManager::s_render_w = 0;
int ImageOutputManager::s_render_h = 0;

std::string ImageOutputManager::s_pending_path;
std::string ImageOutputManager::s_pending_data;
int ImageOutputManager::s_pending_rows = 0;
int ImageOutputManager::s_pending_col = 0;
int ImageOutputManager::s_pending_image_rows = 0;
int ImageOutputManager::s_pending_max_rows = 0;
int ImageOutputManager::s_pending_panel_width = 0;
int ImageOutputManager::s_pending_render_w = 0;
int ImageOutputManager::s_pending_render_h = 0;
std::chrono::steady_clock::time_point ImageOutputManager::s_pending_time;

std::string ImageOutputManager::s_encoding_path;
uint64_t ImageOutputManager::s_encode_version = 0;
std::string ImageOutputManager::s_failed_path;

// ---- mutexes ----

static std::mutex s_manager_mutex;
static std::mutex s_encode_mutex;

// ---- Protocol management ----

void ImageOutputManager::DetectProtocol() {
    auto proto_kitty = std::make_unique<KittyProtocol>();
    if (proto_kitty->IsSupported()) {
        PERF_LOG("proto", "Active protocol: kitty");
        s_protocol = std::move(proto_kitty);
        return;
    }
    auto proto_iterm2 = std::make_unique<ITerm2Protocol>();
    if (proto_iterm2->IsSupported()) {
        PERF_LOG("proto", "Active protocol: iterm2");
        s_protocol = std::move(proto_iterm2);
        return;
    }
    auto proto_sixel = std::make_unique<SixelProtocol>();
    if (proto_sixel->IsSupported()) {
        PERF_LOG("proto", "Active protocol: sixel");
        s_protocol = std::move(proto_sixel);
        return;
    }
    PERF_LOG("proto", "No supported image protocol detected, using fallback");
    s_protocol = nullptr;
}

TerminalImageProtocol* ImageOutputManager::ActiveProtocol() {
    return s_protocol.get();
}

std::string ImageOutputManager::ProtocolName() {
    return s_protocol ? s_protocol->Name() : "(none)";
}

// ---- Cache ----

bool ImageOutputManager::HasCached(const std::string& path) {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return (path == s_cached_path && !s_cached_data.empty());
}

void ImageOutputManager::Cache(const std::string& path,
                                const std::string& data,
                                int term_rows,
                                int render_w,
                                int render_h) {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    s_cached_path = path;
    s_cached_data = data;
    s_image_rows = term_rows;
    s_render_w = render_w;
    s_render_h = render_h;
    PERF_LOG("img_cache", "Cache path=" + path +
             " size=" + std::to_string(data.size()) +
             " rows=" + std::to_string(term_rows) +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h));
}

bool ImageOutputManager::GetCached(const std::string& path,
                                    std::string& out_data,
                                    int& out_rows,
                                    int* out_render_w,
                                    int* out_render_h) {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    if (path != s_cached_path) {
        PERF_LOG("img_cache", "GetCached MISS path=" + path + " cached=" + s_cached_path);
        return false;
    }
    if (s_cached_data.empty()) {
        PERF_LOG("img_cache", "GetCached EMPTY path=" + path);
        return false;
    }
    out_data = s_cached_data;
    out_rows = s_image_rows;
    if (out_render_w) *out_render_w = s_render_w;
    if (out_render_h) *out_render_h = s_render_h;
    PERF_LOG("img_cache", "GetCached HIT path=" + path +
             " size=" + std::to_string(s_cached_data.size()) +
             " rows=" + std::to_string(s_image_rows) +
             " render=" + std::to_string(s_render_w) + "x" + std::to_string(s_render_h));
    return true;
}

// ---- Position / State ----

int ImageOutputManager::CurrentRow() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return s_term_rows;
}

int ImageOutputManager::CurrentCol() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return s_term_cols;
}

int ImageOutputManager::CurrentImageRows() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return s_image_rows;
}

int ImageOutputManager::CurrentMaxRows() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return s_max_image_rows;
}

bool ImageOutputManager::IsActive() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return !s_cached_path.empty() && !s_cached_data.empty();
}

// ---- Clear ----

void ImageOutputManager::ClearCurrent() {
    std::string path_at_clear;
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        path_at_clear = s_cached_path;
    }
    {
        std::lock_guard<std::mutex> lock(s_encode_mutex);
        s_failed_path.clear();
    }
    PERF_LOG("img_clear", "ClearCurrent called path=" + path_at_clear.substr(0, 60));

    int r = 0, c = 0, n = 0, w = 0;
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        if (s_cached_path.empty() || s_cached_data.empty()) {
            PERF_LOG("img_clear", "ClearCurrent nothing to clear");
            return;
        }
        r = s_term_rows;
        c = s_term_cols;
        n = s_image_rows;
        w = s_panel_width;
        PERF_LOG("img_clear", "clearing " + std::to_string(n) + " rows starting at " +
                 std::to_string(r) + " col=" + std::to_string(c) +
                 " width=" + std::to_string(w));
        PERF_LOG("img_clear", "pending_before_clear: path=" +
                 s_pending_path.substr(0, 40) +
                 " size=" + std::to_string(s_pending_data.size()));

        s_cached_path.clear();
        s_cached_data.clear();
        s_term_rows = 0;
        s_term_cols = 0;
        s_image_rows = 0;
        s_max_image_rows = 0;
        s_panel_width = 0;
        s_render_w = 0;
        s_render_h = 0;

        s_pending_path.clear();
        s_pending_data.clear();
        s_pending_rows = 0;
        s_pending_col = 0;
        s_pending_image_rows = 0;
        s_pending_max_rows = 0;
        s_pending_panel_width = 0;
        s_pending_render_w = 0;
        s_pending_render_h = 0;
        PERF_LOG("img_clear", "pending cleared");
    }

    if (s_protocol && n > 0 && w > 0) {
        s_protocol->ClearArea(r, n, c, w);
    }
    PERF_LOG("img_clear", "ClearCurrent completed");
}

// ---- Deferred flush ----

void ImageOutputManager::SetPending(const std::string& path,
                                     const std::string& data,
                                     int term_row, int term_col,
                                     int image_rows,
                                     int max_image_rows,
                                     int panel_width,
                                     int render_w,
                                     int render_h) {
    std::string tid_str;
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        tid_str = oss.str();
    }
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        s_pending_path = path;
        s_pending_data = data;
        s_pending_rows = term_row;
        s_pending_col = term_col;
        s_pending_image_rows = image_rows;
        s_pending_max_rows = max_image_rows;
        s_pending_panel_width = panel_width;
        s_pending_render_w = render_w;
        s_pending_render_h = render_h;
        s_pending_time = std::chrono::steady_clock::now();
    }
    PERF_LOG("img_defer", "SetPending path=" + path.substr(0, 50) +
             " rows=" + std::to_string(term_row) +
             " col=" + std::to_string(term_col) +
             " image_rows=" + std::to_string(image_rows) +
             " max_rows=" + std::to_string(max_image_rows) +
             " panel_width=" + std::to_string(panel_width) +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
             " size=" + std::to_string(data.size()) +
             " tid=" + tid_str);
}

bool ImageOutputManager::HasPending() {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    return !s_pending_path.empty() && !s_pending_data.empty();
}

void ImageOutputManager::FlushPendingIfDirty() {
    if (!s_protocol) return;

    std::string path;
    std::string data;
    int rows = 0, col = 0, image_rows = 0, max_rows = 0;
    std::string tid_str;
    int panel_width = 0;
    int render_w = 0, render_h = 0;

    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        if (s_pending_path.empty() || s_pending_data.empty()) {
            return;
        }
        path = s_pending_path;
        data = s_pending_data;
        rows = s_pending_rows;
        col = s_pending_col;
        image_rows = s_pending_image_rows;
        max_rows = s_pending_max_rows;
        panel_width = s_pending_panel_width;
        render_w = s_pending_render_w;
        render_h = s_pending_render_h;
        // Also update the active cache so IsActive() etc. work
        s_cached_path = path;
        s_cached_data = data;
        s_term_rows = rows;
        s_term_cols = col;
        s_image_rows = image_rows;
        s_max_image_rows = max_rows;
        s_panel_width = panel_width;
        s_render_w = render_w;
        s_render_h = render_h;

        {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            tid_str = oss.str();
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(now - s_pending_time).count();
    PERF_LOG("img_defer", "FLUSHING path=" + path.substr(0, 60) +
             " row=" + std::to_string(rows) +
             " col=" + std::to_string(col) +
             " size=" + std::to_string(data.size()) +
             " image_rows=" + std::to_string(image_rows) +
             " pending_age=" + std::to_string(latency_us / 1000) + "ms" +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
             " tid=" + tid_str);

    PERF_TIMER("img_defer", "write_tty");
    s_protocol->WriteToTTY(data, render_w, render_h, rows, col);
}

// ---- Async encode ----

void ImageOutputManager::EncodeAsync(const std::string& path,
                                      int pixel_w, int pixel_h) {
    if (!s_protocol) return;

    uint64_t my_version;
    {
        std::lock_guard<std::mutex> lock(s_encode_mutex);
        if (s_encoding_path == path) {
            PERF_LOG("img_async", "already encoding, skip path=" + path.substr(0, 50));
            return;
        }
        if (s_failed_path == path) {
            PERF_LOG("img_async", "already failed, skip path=" + path.substr(0, 50));
            return;
        }
        s_encoding_path = path;
        my_version = ++s_encode_version;
    }
    PERF_LOG("img_async", "submit path=" + path.substr(0, 50) +
             " pixel=" + std::to_string(pixel_w) + "x" + std::to_string(pixel_h) +
             " ver=" + std::to_string(my_version));

    std::thread([path, pixel_w, pixel_h, my_version]() {
        std::string tid_str;
        {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            tid_str = oss.str();
        }
        PERF_LOG("img_async", "worker start path=" + path.substr(0, 50) +
                 " tid=" + tid_str + " ver=" + std::to_string(my_version));

        int term_rows = 0, render_w = 0, render_h = 0;
        PERF_TIMER("img_async", "protocol_encode");
        TerminalImageProtocol* proto = ImageOutputManager::ActiveProtocol();
        if (!proto) {
            PERF_LOG("img_async", "no active protocol, aborting");
            return;
        }
        std::string encoded = proto->Encode(path, pixel_w, pixel_h,
                                            term_rows, render_w, render_h);

        bool stale = false;
        bool failed = encoded.empty() || term_rows <= 0;
        {
            std::lock_guard<std::mutex> lock(s_encode_mutex);
            if (my_version != s_encode_version || s_encoding_path != path) {
                stale = true;
            } else {
                s_encoding_path.clear();
                if (failed) s_failed_path = path;
            }
        }

        if (stale) {
            PERF_LOG("img_async", "STALE result discarded path=" + path.substr(0, 50) +
                     " ver=" + std::to_string(my_version));
            return;
        }

        if (!failed) {
            Cache(path, encoded, term_rows, render_w, render_h);
            PERF_LOG("img_async", "cached path=" + path.substr(0, 50) +
                     " rows=" + std::to_string(term_rows) +
                     " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
                     " size=" + std::to_string(encoded.size()));
        } else {
            PERF_LOG("img_async", "encode failed path=" + path.substr(0, 50));
        }
    }).detach();
}

}  // namespace FTB
