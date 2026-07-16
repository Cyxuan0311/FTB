#include "../../include/protocols/SixelProtocol.hpp"
#include "../../include/utils/WebpDecoder.hpp"
#include "../../include/utils/PerfLogger.hpp"

#include "../../include/utils/TerminalProbe.hpp"
#include "../../include/utils/TmuxContext.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../../third_party/stb_image.h"

#ifdef FTB_ENABLE_LIBSIXEL
#  ifdef LIBSIXEL_HAS_SUBDIR
#    include <sixel/sixel.h>
#  else
#    include <sixel.h>
#  endif
#endif

namespace FTB {

#ifdef FTB_ENABLE_LIBSIXEL

bool SixelProtocol::IsSupported() const {
    auto& info = TerminalProbe::Detect();
    return info.sixel;
}

struct SixelWriteContext {
    std::string* output;
};

static int sixel_write_callback(char* data, int size, void* priv) {
    auto* ctx = static_cast<SixelWriteContext*>(priv);
    ctx->output->append(data, static_cast<size_t>(size));
    return size;
}

bool SixelProtocol::sixel_encode_frame(
    const unsigned char* rgba, int w, int h,
    std::string& out_data)
{
    sixel_dither_t* dither = nullptr;
    sixel_output_t* output = nullptr;
    out_data.clear();

    SIXELSTATUS status = sixel_dither_new(&dither, 256, nullptr);
    if (SIXEL_FAILED(status)) return false;

    status = sixel_dither_initialize(
        dither, const_cast<unsigned char*>(rgba), w, h,
        SIXEL_PIXELFORMAT_RGB888,
        SIXEL_LARGE_AUTO, SIXEL_REP_AUTO, SIXEL_QUALITY_HIGH);
    if (SIXEL_FAILED(status)) {
        sixel_dither_destroy(dither);
        return false;
    }

    sixel_dither_set_diffusion_type(dither, SIXEL_DIFFUSE_FS);
    sixel_dither_set_optimize_palette(dither, 1);

    SixelWriteContext ctx = {&out_data};
    status = sixel_output_new(&output, sixel_write_callback, &ctx, nullptr);
    if (SIXEL_FAILED(status)) {
        sixel_dither_destroy(dither);
        return false;
    }

    sixel_output_set_8bit_availability(output, 0);
    sixel_output_set_skip_dcs_envelope(output, 1);

    status = sixel_encode(const_cast<unsigned char*>(rgba), w, h, 3, dither, output);

    sixel_output_destroy(output);
    sixel_dither_destroy(dither);

    return !SIXEL_FAILED(status);
}

struct StbOrWebpData {
    unsigned char* data = nullptr;
    std::vector<unsigned char> webp_owned;

    ~StbOrWebpData() {
        if (data && webp_owned.empty()) stbi_image_free(data);
    }

    bool load(const std::string& path, int& w, int& h, int& ch) {
        data = stbi_load(path.c_str(), &w, &h, &ch, 3);
        if (data) return true;
        auto result = WebpDecoder::Decode(path);
        if (!result.success) return false;
        w = result.width;
        h = result.height;
        webp_owned = std::move(result.pixels);
        data = webp_owned.data();
        return true;
    }
};

std::string SixelProtocol::Encode(
    const std::string& path,
    int max_width,
    int max_height,
    int& out_term_rows,
    int& out_render_w,
    int& out_render_h
) {
    out_term_rows = 0;

    // Detect GIF → load all frames for animation
    std::string ext;
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".gif") {
        int render_w = 0, render_h = 0;
        m_anim_frames.clear();
        bool dec_ok = GifFrameDecoder::Decode(path, max_width, max_height,
            [&](int i, DecodedFrame& frame) -> bool {
                if (i > 0) return false; // only encode frame 0 in Encode()
                std::string sixel_data;
                if (!sixel_encode_frame(frame.rgba.data(),
                                         frame.width, frame.height, sixel_data)) {
                    PERF_LOG("SixelGif", "sixel_encode FAILED frame[0]");
                    m_anim_frames.clear();
                    return false;
                }
                PERF_LOG("SixelGif", "sixel[0] len="
                    + std::to_string(sixel_data.size())
                    + " delay=" + std::to_string(frame.delay_ms) + "ms");
                AnimFrame af;
                af.sixel_data = std::move(sixel_data);
                af.delay_ms = frame.delay_ms;
                m_anim_frames.push_back(std::move(af));
                return true; // stop after frame 0
            }, render_w, render_h);
        PERF_LOG("SixelGif", "decode path=" + path + " ok=" + std::to_string(dec_ok)
            + " render=" + std::to_string(render_w) + "x" + std::to_string(render_h));
        if (dec_ok) {
            m_gif_path = path;
            m_gif_encode_max_w = max_width;
            m_gif_encode_max_h = max_height;
            out_render_w = render_w;
            out_render_h = render_h;
            out_term_rows = (render_h + 5) / 6;
            m_anim_render_w = render_w;
            m_anim_render_h = render_h;

            if (!m_anim_frames.empty()) {
                return m_anim_frames[0].sixel_data;
            }
        }
        // Fall through to static image flow
    }

    int img_w, img_h, channels;
    StbOrWebpData img;
    if (!img.load(path, img_w, img_h, channels)) {
        return {};
    }
    unsigned char* data = img.data;

    double target_ratio = static_cast<double>(img_w) / img_h;
    int render_w, render_h;
    if (static_cast<double>(max_width) / max_height > target_ratio) {
        render_h = max_height;
        render_w = std::max(1, static_cast<int>(max_height * target_ratio));
    } else {
        render_w = max_width;
        render_h = std::max(1, static_cast<int>(max_width / target_ratio));
    }

    unsigned char* scaled = nullptr;
    if (render_w != img_w || render_h != img_h) {
        scaled = static_cast<unsigned char*>(std::malloc(
            static_cast<size_t>(render_w) * render_h * 3));
        if (!scaled) {
            return {};
        }
        SIXELSTATUS status = sixel_helper_scale_image(
            scaled, data, img_w, img_h,
            SIXEL_PIXELFORMAT_RGB888,
            render_w, render_h,
            SIXEL_RES_BILINEAR,
            nullptr);
        if (SIXEL_FAILED(status)) {
            std::free(scaled);
            return {};
        }
    } else {
        scaled = data;
    }

    out_render_w = render_w;
    out_render_h = render_h;
    out_term_rows = (render_h + 5) / 6;

    std::string sixel_result;
    if (!sixel_encode_frame(scaled, render_w, render_h, sixel_result)) {
        if (scaled != data) std::free(scaled);
        return {};
    }

    if (scaled != data) std::free(scaled);

    return sixel_result;
}

bool SixelProtocol::StartGifAnimation(int term_row, int term_col) {
    PERF_LOG("SixelGif", "StartGifAnimation row=" + std::to_string(term_row)
        + " col=" + std::to_string(term_col)
        + " frames=" + std::to_string(m_anim_frames.size()));

    StopGifAnimation();

    if (m_gif_path.empty() || m_anim_frames.empty()) return false;

    // Keep only frame 0 (encoded during Encode), re-encode the rest
    m_anim_frames.resize(1);

    m_anim_term_row = term_row;
    m_anim_term_col = term_col;
    m_animating = true;

    m_anim_thread = std::thread([this,
        path = m_gif_path,
        max_w = m_gif_encode_max_w,
        max_h = m_gif_encode_max_h]()
    {
        // ── Phase 1: encode remaining frames ──
        PERF_LOG("SixelGif", "anim: phase1 encode remaining frames");
        {
            int render_w = 0, render_h = 0;
            GifFrameDecoder::Decode(path, max_w, max_h,
                [&](int i, DecodedFrame& frame) -> bool {
                    if (i == 0) return true; // frame 0 already in m_anim_frames
                    std::string sixel_data;
                    if (!sixel_encode_frame(frame.rgba.data(),
                                             frame.width, frame.height, sixel_data)) {
                        PERF_LOG("SixelGif", "anim: phase1 encode FAILED frame["
                            + std::to_string(i) + "]");
                        return false;
                    }
                    PERF_LOG("SixelGif", "anim: phase1 frame[" + std::to_string(i)
                        + "] len=" + std::to_string(sixel_data.size())
                        + " delay=" + std::to_string(frame.delay_ms) + "ms");
                    AnimFrame af;
                    af.sixel_data = std::move(sixel_data);
                    af.delay_ms = frame.delay_ms;
                    m_anim_frames.push_back(std::move(af));
                    return m_animating.load();
                }, render_w, render_h);
        }
        PERF_LOG("SixelGif", "anim: phase1 done frames="
            + std::to_string(m_anim_frames.size()));

        if (!m_animating.load() || m_anim_frames.empty()) return;

        // ── Phase 2: playback loop ──
        int loop = 0;
        while (m_animating.load()) {
            PERF_LOG("SixelGif", "anim_loop=" + std::to_string(loop)
                + " frames=" + std::to_string(m_anim_frames.size()));
            size_t idx = 0;
            for (const auto& frame : m_anim_frames) {
                if (!m_animating.load()) break;
                PERF_LOG("SixelGif", "anim_frame[" + std::to_string(idx) + "/"
                    + std::to_string(m_anim_frames.size())
                    + "] loop=" + std::to_string(loop)
                    + " delay=" + std::to_string(frame.delay_ms) + "ms");
                WriteToTTY(frame.sixel_data,
                           m_anim_render_w, m_anim_render_h,
                           m_anim_term_row, m_anim_term_col);
                std::unique_lock<std::mutex> lock(m_anim_mutex);
                m_anim_cv.wait_for(lock,
                    std::chrono::milliseconds(frame.delay_ms),
                    [this]{ return !m_animating.load(); });
                ++idx;
            }
            ++loop;
        }
        PERF_LOG("SixelGif", "anim_thread exited loop=" + std::to_string(loop));
    });

    return true;
}

void SixelProtocol::StopGifAnimation() {
    PERF_LOG("SixelGif", "StopGifAnimation called");
    m_animating = false;
    m_anim_cv.notify_one();
    if (m_anim_thread.joinable()) {
        m_anim_thread.join();
        PERF_LOG("SixelGif", "anim_thread joined");
    }
    PERF_LOG("SixelGif", "StopGifAnimation done");
}

void SixelProtocol::WriteToTerminal(const std::string& data,
                                     int /*render_w*/,
                                     int /*render_h*/,
                                     int term_row,
                                     int term_col) {

    std::cout << "\033[s"
              << "\033[" << term_row + 1 << ";"
                        << term_col + 1 << "H"
              << "\033P1;0q"
              << data
              << "\033\\"
              << "\033[u"
              << std::flush;
}

void SixelProtocol::WriteToTTY(const std::string& data,
                                int /*render_w*/,
                                int /*render_h*/,
                                int term_row,
                                int term_col) {
    auto& info = TerminalProbe::Detect();
    auto& tmux = TmuxContext::Instance();
    bool use_direct_tty = info.needs_direct_tty;
    const std::string& tty_path = use_direct_tty ? tmux.OuterTty() : std::string("/dev/tty");
    int row_off = use_direct_tty ? tmux.PaneTop() : 0;
    int col_off = use_direct_tty ? tmux.PaneLeft() : 0;

    int fd = ::open(tty_path.c_str(), O_WRONLY);
    if (fd == -1) {
        fd = ::open(tty_path.c_str(), O_RDWR);
    }
    if (fd == -1) {
        return;
    }

    char pos_buf[32];
    int pos_len = std::snprintf(pos_buf, sizeof(pos_buf),
                                "\033[%d;%dH", term_row + 1 + row_off, term_col + 1 + col_off);
    ssize_t r;
    r = ::write(fd, pos_buf, static_cast<size_t>(pos_len));
    if (r < 0) { ::close(fd); return; }

    r = ::write(fd, "\033P1;0q", 5);
    if (r < 0) { ::close(fd); return; }

    r = ::write(fd, data.data(), data.size());
    if (r < 0) { ::close(fd); return; }

    r = ::write(fd, "\033\\", 2);
    if (r < 0) { ::close(fd); return; }

    ::close(fd);
}

void SixelProtocol::ClearArea(int start_row, int num_rows,
                               int start_col, int width) {
    if (num_rows <= 0) return;
    auto& info = TerminalProbe::Detect();
    auto& tmux = TmuxContext::Instance();
    bool use_direct_tty = info.needs_direct_tty;
    int row_off = use_direct_tty ? tmux.PaneTop() : 0;
    int col_off = use_direct_tty ? tmux.PaneLeft() : 0;

    if (width > 0) {
        std::string blank(static_cast<size_t>(width), ' ');

        if (use_direct_tty) {
            int fd_outer = ::open(tmux.OuterTty().c_str(), O_WRONLY);
            if (fd_outer == -1) fd_outer = ::open(tmux.OuterTty().c_str(), O_RDWR);
            if (fd_outer != -1) {
                for (int i = 0; i < num_rows; ++i) {
                    char buf[32];
                    int len = std::snprintf(buf, sizeof(buf),
                                            "\033[%d;%dH", start_row + i + 1 + row_off, start_col + 1 + col_off);
                    ssize_t unused = ::write(fd_outer, buf, static_cast<size_t>(len));
                    (void)unused;
                    unused = ::write(fd_outer, blank.data(), blank.size());
                    (void)unused;
                }
                ::close(fd_outer);
            }
        }

        int fd = ::open("/dev/tty", O_WRONLY);
        if (fd == -1) fd = ::open("/dev/tty", O_RDWR);
        if (fd != -1) {
            for (int i = 0; i < num_rows; ++i) {
                char buf[32];
                int len = std::snprintf(buf, sizeof(buf),
                                        "\033[%d;%dH", start_row + i + 1, start_col + 1);
                ssize_t unused = ::write(fd, buf, static_cast<size_t>(len));
                (void)unused;
                unused = ::write(fd, blank.data(), blank.size());
                (void)unused;
            }
            ::close(fd);
        }
    } else {
        for (int i = 0; i < num_rows; ++i) {
            std::cout << "\033[" << start_row + i + 1 << ";1H\033[K";
        }
        std::cout << std::flush;
    }
}

#else // !FTB_ENABLE_LIBSIXEL

bool SixelProtocol::IsSupported() const { return false; }
std::string SixelProtocol::Encode(const std::string&, int, int, int&, int&, int&) { return {}; }
void SixelProtocol::WriteToTerminal(const std::string&, int, int, int, int) {}
void SixelProtocol::WriteToTTY(const std::string&, int, int, int, int) {}
void SixelProtocol::ClearArea(int, int, int, int) {}
bool SixelProtocol::StartGifAnimation(int, int) { return false; }
void SixelProtocol::StopGifAnimation() {}

#endif

}  // namespace FTB
