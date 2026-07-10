#include "../../include/protocols/SixelProtocol.hpp"

#include "../../include/utils/TerminalProbe.hpp"
#include "../../include/utils/TmuxContext.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
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
    // Protocol selection is now handled by TerminalProbe in ImageOutputManager.
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

std::string SixelProtocol::Encode(
    const std::string& path,
    int max_width,
    int max_height,
    int& out_term_rows,
    int& out_render_w,
    int& out_render_h
) {
    out_term_rows = 0;

    int img_w, img_h, channels;
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);
    if (!data) {
        return {};
    }

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
            stbi_image_free(data);
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
            stbi_image_free(data);
            return {};
        }
    } else {
        scaled = data;
    }

    out_render_w = render_w;
    out_render_h = render_h;
    out_term_rows = (render_h + 5) / 6;

    sixel_dither_t* dither = nullptr;
    sixel_output_t* output = nullptr;
    std::string sixel_result;

    SIXELSTATUS status = sixel_dither_new(&dither, 256, nullptr);
    if (SIXEL_FAILED(status)) {
        if (scaled != data) std::free(scaled);
        stbi_image_free(data);
        return {};
    }

    status = sixel_dither_initialize(
        dither, scaled, render_w, render_h,
        SIXEL_PIXELFORMAT_RGB888,
        SIXEL_LARGE_AUTO, SIXEL_REP_AUTO, SIXEL_QUALITY_HIGH);
    if (SIXEL_FAILED(status)) {
        sixel_dither_destroy(dither);
        if (scaled != data) std::free(scaled);
        stbi_image_free(data);
        return {};
    }

    sixel_dither_set_diffusion_type(dither, SIXEL_DIFFUSE_FS);
    sixel_dither_set_optimize_palette(dither, 1);

    SixelWriteContext ctx = {&sixel_result};
    status = sixel_output_new(&output, sixel_write_callback, &ctx, nullptr);
    if (SIXEL_FAILED(status)) {
        sixel_dither_destroy(dither);
        if (scaled != data) std::free(scaled);
        stbi_image_free(data);
        return {};
    }

    sixel_output_set_8bit_availability(output, 0);
    sixel_output_set_skip_dcs_envelope(output, 1);

    status = sixel_encode(scaled, render_w, render_h, 3, dither, output);

    sixel_output_destroy(output);
    sixel_dither_destroy(dither);
    if (scaled != data) std::free(scaled);
    stbi_image_free(data);

    return sixel_result;
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

        // When using direct TTY, write to outer TTY with pane offsets
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

        // Also clear on tmux's virtual screen (no pane offset)
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

#endif

}  // namespace FTB
