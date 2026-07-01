#include "../../include/protocols/SixelProtocol.hpp"

#include "../../include/utils/PerfLogger.hpp"

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
#include <sixel/sixel.h>
#endif

namespace FTB {

#ifdef FTB_ENABLE_LIBSIXEL

bool SixelProtocol::IsSupported() const {
    const char* term_cstr = std::getenv("TERM");
    std::string term_str = term_cstr ? term_cstr : "(unset)";
    PERF_LOG("sixel_detect", "TERM=" + term_str);

    if (term_cstr) {
        std::string t(term_cstr);
        static const std::vector<std::string> sixel_terms = {
            "xterm-kitty", "foot", "foot-extra", "foot-direct",
            "contour", "wezterm", "xterm-direct", "xterm-24bit",
            "st-direct", "st-24bit",
        };
        for (const auto& st : sixel_terms) {
            if (t == st) {
                PERF_LOG("sixel_detect", "MATCHED sixel term: " + t);
                return true;
            }
        }
        if (t.find("xterm") != std::string::npos) {
            const char* xterm_version = std::getenv("XTERM_VERSION");
            if (xterm_version) {
                PERF_LOG("sixel_detect", "MATCHED xterm with XTERM_VERSION=" + std::string(xterm_version));
                return true;
            }
        }
        const char* wt_session = std::getenv("WT_SESSION");
        if (wt_session) {
            PERF_LOG("sixel_detect", "MATCHED Windows Terminal (WT_SESSION)");
            return true;
        }
    }
    const char* ftb_sixel = std::getenv("FTB_SIXEL");
    if (ftb_sixel && ftb_sixel[0] == '1') {
        PERF_LOG("sixel_detect", "FORCED via FTB_SIXEL=1");
        return true;
    }
    PERF_LOG("sixel_detect", "NOT supported (term=" + term_str + ")");
    return false;
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
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        PERF_LOG("sixel_encode", "SixelProtocol::Encode path=" + path +
                 " max_w=" + std::to_string(max_width) +
                 " max_h=" + std::to_string(max_height) +
                 " tid=" + oss.str());
    }

    int img_w, img_h, channels;
    PERF_TIMER("sixel_encode", "stbi_load");
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);
    if (!data) {
        PERF_LOG("sixel_encode", "FAILED stbi_load for " + path);
        return {};
    }
    PERF_LOG("sixel_encode", "stbi_load ok: " + std::to_string(img_w) + "x" +
             std::to_string(img_h) + " ch=" + std::to_string(channels) +
             " file=" + path);

    double target_ratio = static_cast<double>(img_w) / img_h;
    int render_w, render_h;
    if (static_cast<double>(max_width) / max_height > target_ratio) {
        render_h = max_height;
        render_w = std::max(1, static_cast<int>(max_height * target_ratio));
    } else {
        render_w = max_width;
        render_h = std::max(1, static_cast<int>(max_width / target_ratio));
    }
    PERF_LOG("sixel_encode", "render size: " + std::to_string(render_w) +
             "x" + std::to_string(render_h) +
             " (original: " + std::to_string(img_w) + "x" + std::to_string(img_h) + ")");

    unsigned char* scaled = nullptr;
    if (render_w != img_w || render_h != img_h) {
        scaled = static_cast<unsigned char*>(std::malloc(
            static_cast<size_t>(render_w) * render_h * 3));
        if (!scaled) {
            PERF_LOG("sixel_encode", "FAILED malloc for scaled buffer");
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
            PERF_LOG("sixel_encode", "FAILED sixel_helper_scale_image status=" +
                     std::to_string(status));
            std::free(scaled);
            stbi_image_free(data);
            return {};
        }
        PERF_LOG("sixel_encode", "scaled ok");
    } else {
        scaled = data;
        PERF_LOG("sixel_encode", "no scale needed");
    }

    out_render_w = render_w;
    out_render_h = render_h;
    out_term_rows = (render_h + 5) / 6;
    PERF_LOG("sixel_encode", "out_term_rows=" + std::to_string(out_term_rows) +
             " render_w=" + std::to_string(render_w) +
             " render_h=" + std::to_string(render_h));

    sixel_dither_t* dither = nullptr;
    sixel_output_t* output = nullptr;
    std::string sixel_result;

    SIXELSTATUS status = sixel_dither_new(&dither, 256, nullptr);
    if (SIXEL_FAILED(status)) {
        PERF_LOG("sixel_encode", "FAILED sixel_dither_new");
        if (scaled != data) std::free(scaled);
        stbi_image_free(data);
        return {};
    }

    status = sixel_dither_initialize(
        dither, scaled, render_w, render_h,
        SIXEL_PIXELFORMAT_RGB888,
        SIXEL_LARGE_AUTO, SIXEL_REP_AUTO, SIXEL_QUALITY_HIGH);
    if (SIXEL_FAILED(status)) {
        PERF_LOG("sixel_encode", "FAILED sixel_dither_initialize status=" +
                 std::to_string(status));
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
        PERF_LOG("sixel_encode", "FAILED sixel_output_new");
        sixel_dither_destroy(dither);
        if (scaled != data) std::free(scaled);
        stbi_image_free(data);
        return {};
    }

    sixel_output_set_8bit_availability(output, 0);
    sixel_output_set_skip_dcs_envelope(output, 1);

    status = sixel_encode(scaled, render_w, render_h, 3, dither, output);
    if (SIXEL_FAILED(status)) {
        PERF_LOG("sixel_encode", "sixel_encode returned FAILED status=" +
                 std::to_string(status) + " but using partial data");
    }

    sixel_output_destroy(output);
    sixel_dither_destroy(dither);
    if (scaled != data) std::free(scaled);
    stbi_image_free(data);

    PERF_LOG("sixel_encode", "SixelProtocol::Encode SUCCESS size=" +
             std::to_string(sixel_result.size()) + " bytes" +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
             " term_rows=" + std::to_string(out_term_rows));

    if (sixel_result.size() >= 4) {
        std::string header;
        for (size_t i = 0; i < std::min(size_t(4), sixel_result.size()); ++i) {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%02x", (unsigned char)sixel_result[i]);
            header += buf;
        }
        PERF_LOG("sixel_encode", "sixel header hex: " + header +
                 " human: '" + sixel_result.substr(0, std::min(size_t(10), sixel_result.size())) + "'");
    }
    return sixel_result;
}

void SixelProtocol::WriteToTerminal(const std::string& data,
                                     int /*render_w*/,
                                     int /*render_h*/,
                                     int term_row,
                                     int term_col) {
    PERF_LOG("sixel_write", "SixelProtocol::WriteToTerminal row=" + std::to_string(term_row) +
             " col=" + std::to_string(term_col) +
             " data_size=" + std::to_string(data.size()));

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
    PERF_LOG("sixel_write", "SixelProtocol::WriteToTTY row=" + std::to_string(term_row) +
             " col=" + std::to_string(term_col) +
             " data_size=" + std::to_string(data.size()));

    int fd = ::open("/dev/tty", O_WRONLY);
    if (fd == -1) {
        fd = ::open("/dev/tty", O_RDWR);
    }
    if (fd == -1) {
        PERF_LOG("sixel_write", "FAILED to open /dev/tty errno=" + std::to_string(errno));
        return;
    }

    char pos_buf[32];
    int pos_len = std::snprintf(pos_buf, sizeof(pos_buf),
                                "\033[%d;%dH", term_row + 1, term_col + 1);
    ssize_t r;
    r = ::write(fd, pos_buf, static_cast<size_t>(pos_len));
    if (r < 0) { PERF_LOG("sixel_write", "FAILED cursor pos write"); ::close(fd); return; }

    r = ::write(fd, "\033P1;0q", 5);
    if (r < 0) { PERF_LOG("sixel_write", "FAILED DCS write"); ::close(fd); return; }

    r = ::write(fd, data.data(), data.size());
    if (r < 0) { PERF_LOG("sixel_write", "FAILED sixel data write"); ::close(fd); return; }
    PERF_LOG("sixel_write", "TTY wrote " + std::to_string(r) + " bytes");

    r = ::write(fd, "\033\\", 2);
    if (r < 0) { PERF_LOG("sixel_write", "FAILED ST write"); ::close(fd); return; }

    ::close(fd);
}

void SixelProtocol::ClearArea(int start_row, int num_rows,
                               int start_col, int width) {
    if (num_rows <= 0) return;
    PERF_LOG("sixel_clear", "SixelProtocol::ClearArea start=" + std::to_string(start_row) +
             " rows=" + std::to_string(num_rows) +
             " col=" + std::to_string(start_col) +
             " width=" + std::to_string(width));

    if (width > 0) {
        int fd = ::open("/dev/tty", O_WRONLY);
        if (fd == -1) fd = ::open("/dev/tty", O_RDWR);
        if (fd != -1) {
            std::string blank(static_cast<size_t>(width), ' ');
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
