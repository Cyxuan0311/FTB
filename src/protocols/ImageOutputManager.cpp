#include "../../include/protocols/ImageOutputManager.hpp"

#include "../../include/protocols/KittyProtocol.hpp"
#include "../../include/protocols/ITerm2Protocol.hpp"
#include "../../include/protocols/SixelProtocol.hpp"
#include "../../include/utils/TerminalProbe.hpp"
#include "../../include/utils/TmuxContext.hpp"

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
std::string ImageOutputManager::s_encoding_path;
uint64_t ImageOutputManager::s_encode_version = 0;
std::string ImageOutputManager::s_failed_path;

bool ImageOutputManager::s_protocol_enabled_ = true;

std::string ImageOutputManager::s_last_flushed_path;
int ImageOutputManager::s_last_flushed_render_w = 0;
int ImageOutputManager::s_last_flushed_render_h = 0;

// ---- mutexes ----

static std::mutex s_manager_mutex;
static std::mutex s_encode_mutex;

// ---- Protocol management ----

void ImageOutputManager::DetectProtocol() {
    // Use TerminalProbe for reliable terminal detection
    auto& info = TerminalProbe::Detect();

    if (!s_protocol_enabled_) {
        s_protocol = nullptr;
        return;
    }

    // Select protocol based on probe results (priority: Kitty > iTerm2 > Sixel)
    if (info.kitty) {
        s_protocol = std::make_unique<KittyProtocol>();
        return;
    }
    if (info.iterm2) {
        s_protocol = std::make_unique<ITerm2Protocol>();
        return;
    }
#ifdef FTB_ENABLE_LIBSIXEL
    if (info.sixel) {
        s_protocol = std::make_unique<SixelProtocol>();
        return;
    }
#endif
    s_protocol = nullptr;
}

void ImageOutputManager::ReDetectProtocol() {
    ClearCurrent();
    {
        std::lock_guard<std::mutex> lock(s_encode_mutex);
        s_encoding_path.clear();
        s_failed_path.clear();
    }
    DetectProtocol();
}

bool ImageOutputManager::IsProtocolEnabled() {
    return s_protocol_enabled_;
}

void ImageOutputManager::SetProtocolEnabled(bool on) {
    s_protocol_enabled_ = on;
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
}

bool ImageOutputManager::GetCached(const std::string& path,
                                    std::string& out_data,
                                    int& out_rows,
                                    int* out_render_w,
                                    int* out_render_h) {
    std::lock_guard<std::mutex> lock(s_manager_mutex);
    if (path != s_cached_path) {
        return false;
    }
    if (s_cached_data.empty()) {
        return false;
    }
    out_data = s_cached_data;
    out_rows = s_image_rows;
    if (out_render_w) *out_render_w = s_render_w;
    if (out_render_h) *out_render_h = s_render_h;
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

    int r = 0, c = 0, n = 0, w = 0;
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        if (s_cached_path.empty() || s_cached_data.empty()) {
            return;
        }
        r = s_term_rows;
        c = s_term_cols;
        n = s_image_rows;
        w = s_panel_width;

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

        // Reset last-flushed so next image will be re-flushed
        s_last_flushed_path.clear();
        s_last_flushed_render_w = 0;
        s_last_flushed_render_h = 0;
    }

    if (s_protocol && n > 0 && w > 0) {
        s_protocol->ClearArea(r, n, c, w);
    }
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
    }
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

        // Skip flush if same image (path + geometry) was already flushed
        if (path == s_last_flushed_path &&
            render_w == s_last_flushed_render_w &&
            render_h == s_last_flushed_render_h) {
            // Still update the active cache for downstream queries
            s_cached_path = path;
            s_cached_data = data;
            s_term_rows = rows;
            s_term_cols = col;
            s_image_rows = image_rows;
            s_max_image_rows = max_rows;
            s_panel_width = panel_width;
            s_render_w = render_w;
            s_render_h = render_h;
            return;
        }

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
    }

    s_protocol->WriteToTTY(data, render_w, render_h, rows, col);

    // Record what was flushed so redundant flushes are skipped
    {
        std::lock_guard<std::mutex> lock(s_manager_mutex);
        s_last_flushed_path = path;
        s_last_flushed_render_w = render_w;
        s_last_flushed_render_h = render_h;
    }
}

// ---- Encode state queries ----

bool ImageOutputManager::HasFailed(const std::string& path) {
    std::lock_guard<std::mutex> lock(s_encode_mutex);
    return path == s_failed_path;
}

bool ImageOutputManager::IsEncoding(const std::string& path) {
    std::lock_guard<std::mutex> lock(s_encode_mutex);
    return path == s_encoding_path;
}

// ---- Async encode ----

void ImageOutputManager::EncodeAsync(const std::string& path,
                                      int pixel_w, int pixel_h) {
    if (!s_protocol) return;

    uint64_t my_version;
    {
        std::lock_guard<std::mutex> lock(s_encode_mutex);
        if (s_encoding_path == path) {
            return;
        }
        if (s_failed_path == path) {
            return;
        }
        s_encoding_path = path;
        my_version = ++s_encode_version;
    }

    std::thread([path, pixel_w, pixel_h, my_version]() {
        int term_rows = 0, render_w = 0, render_h = 0;
        TerminalImageProtocol* proto = ImageOutputManager::ActiveProtocol();
        if (!proto) {
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
            return;
        }

        if (!failed) {
            Cache(path, encoded, term_rows, render_w, render_h);
        }
    }).detach();
}

}  // namespace FTB
