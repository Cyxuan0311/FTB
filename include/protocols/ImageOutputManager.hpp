#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "protocols/TerminalImageProtocol.hpp"

namespace FTB {

class ImageOutputManager {
public:
    // --- Protocol detection and access ---
    static void DetectProtocol();
    static TerminalImageProtocol* ActiveProtocol();
    static std::string ProtocolName();

    // --- Cache ---
    static bool HasCached(const std::string& path);
    static void Cache(const std::string& path, const std::string& data,
                      int term_rows, int render_w, int render_h);
    static bool GetCached(const std::string& path, std::string& out_data, int& out_rows,
                          int* out_render_w = nullptr, int* out_render_h = nullptr);

    // --- Position / State ---
    static int  CurrentRow();
    static int  CurrentCol();
    static int  CurrentImageRows();
    static int  CurrentMaxRows();
    static bool IsActive();

    // --- Clear ---
    static void ClearCurrent();

    // --- Deferred flush ---
    static void SetPending(const std::string& path,
                           const std::string& data,
                           int term_row, int term_col, int image_rows,
                           int max_image_rows, int panel_width,
                           int render_w, int render_h);
    static bool HasPending();
    static void FlushPendingIfDirty();

    // --- Protocol enable/disable ---
    static void ReDetectProtocol();
    static bool IsProtocolEnabled();
    static void SetProtocolEnabled(bool on);

    // --- Async encode ---
    static void EncodeAsync(const std::string& path, int pixel_w, int pixel_h);
    static bool HasFailed(const std::string& path);
    static bool IsEncoding(const std::string& path);

private:
    static std::unique_ptr<TerminalImageProtocol> s_protocol;

    // Cache state
    static std::string s_cached_path;
    static std::string s_cached_data;
    static int s_term_rows;
    static int s_term_cols;
    static int s_image_rows;
    static int s_max_image_rows;
    static int s_panel_width;

    // Pending (deferred flush) state
    static std::string s_pending_path;
    static std::string s_pending_data;
    static int s_pending_rows;
    static int s_pending_col;
    static int s_pending_image_rows;
    static int s_pending_max_rows;
    static int s_pending_panel_width;
    static int s_pending_render_w;
    static int s_pending_render_h;

    // Render dimensions
    static int s_render_w;
    static int s_render_h;

    // Last flushed state (to avoid redundant flushes)
    static std::string s_last_flushed_path;
    static int s_last_flushed_render_w;
    static int s_last_flushed_render_h;

    // Protocol enable flag
    static bool s_protocol_enabled_;

    // Async encode state
    static std::string s_encoding_path;
    static uint64_t s_encode_version;
    static std::string s_failed_path;
};

}  // namespace FTB
