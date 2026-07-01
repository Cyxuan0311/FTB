#include "../../include/protocols/KittyProtocol.hpp"

#include "../../include/protocols/base64_util.hpp"
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
#include "../../third_party/stb_image_resize2.h"
#include "../../third_party/stb_image_write.h"

namespace FTB {

bool KittyProtocol::IsSupported() const {
    const char* kitty_window = std::getenv("KITTY_WINDOW_ID");
    if (kitty_window) {
        PERF_LOG("kitty_detect", "KITTY_WINDOW_ID=" + std::string(kitty_window));
        return true;
    }
    const char* term_cstr = std::getenv("TERM");
    if (term_cstr && std::string(term_cstr) == "xterm-kitty") {
        PERF_LOG("kitty_detect", "TERM=xterm-kitty");
        return true;
    }
    const char* ftb_kitty = std::getenv("FTB_KITTY");
    if (ftb_kitty && ftb_kitty[0] == '1') {
        PERF_LOG("kitty_detect", "FORCED via FTB_KITTY=1");
        return true;
    }
    PERF_LOG("kitty_detect", "NOT supported");
    return false;
}

struct PngWriteContext {
    std::vector<unsigned char>* buffer;
};

static void png_write_callback(void* context, void* data, int size) {
    auto* ctx = static_cast<PngWriteContext*>(context);
    auto* bytes = static_cast<const unsigned char*>(data);
    ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
}

std::string KittyProtocol::Encode(
    const std::string& path,
    int max_width,
    int max_height,
    int& out_term_rows,
    int& out_render_w,
    int& out_render_h
) {
    out_term_rows = 0;
    out_render_w = 0;
    out_render_h = 0;
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        PERF_LOG("kitty_encode", "KittyProtocol::Encode path=" + path +
                 " max_w=" + std::to_string(max_width) +
                 " max_h=" + std::to_string(max_height) +
                 " tid=" + oss.str());
    }

    int img_w, img_h, channels;
    PERF_TIMER("kitty_encode", "stbi_load");
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 3);
    if (!data) {
        PERF_LOG("kitty_encode", "FAILED stbi_load for " + path);
        return {};
    }
    PERF_LOG("kitty_encode", "stbi_load ok: " + std::to_string(img_w) + "x" +
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

    unsigned char* scaled = nullptr;
    if (render_w != img_w || render_h != img_h) {
        scaled = static_cast<unsigned char*>(std::malloc(
            static_cast<size_t>(render_w) * render_h * 3));
        if (!scaled) {
            PERF_LOG("kitty_encode", "FAILED malloc for scaled buffer");
            stbi_image_free(data);
            return {};
        }
        stbir_resize_uint8_srgb(data, img_w, img_h, 0,
                                scaled, render_w, render_h, 0,
                                STBIR_RGB);
    } else {
        scaled = data;
    }

    out_render_w = render_w;
    out_render_h = render_h;
    out_term_rows = (render_h + 17) / 18;
    PERF_LOG("kitty_encode", "render: " + std::to_string(render_w) + "x" +
             std::to_string(render_h) + " term_rows=" + std::to_string(out_term_rows));

    std::vector<unsigned char> png_data;
    PngWriteContext ctx = {&png_data};
    int png_ok = stbi_write_png_to_func(png_write_callback, &ctx,
                                        render_w, render_h, 3,
                                        scaled, 0);
    if (scaled != data) std::free(scaled);
    stbi_image_free(data);

    if (!png_ok) {
        PERF_LOG("kitty_encode", "FAILED stbi_write_png_to_func");
        return {};
    }

    PERF_LOG("kitty_encode", "PNG size=" + std::to_string(png_data.size()) +
             " bytes");

    std::string b64 = detail::Base64Encode(png_data.data(), png_data.size());
    PERF_LOG("kitty_encode", "base64 size=" + std::to_string(b64.size()) +
             " bytes");

    PERF_LOG("kitty_encode", "KittyProtocol::Encode SUCCESS");
    return b64;
}

void KittyProtocol::WriteToTerminal(const std::string& data,
                                     int render_w, int render_h,
                                     int term_row, int term_col) {
    PERF_LOG("kitty_write", "KittyProtocol::WriteToTerminal row=" +
             std::to_string(term_row) + " col=" + std::to_string(term_col) +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
             " data_size=" + std::to_string(data.size()));

    std::cout << "\033[" << term_row + 1 << ";" << term_col + 1 << "H"
              << "\033_Ga=T,f=100,s=" << render_w
              << ",v=" << render_h
              << ",m=0;" << data << "\033\\"
              << std::flush;
}

void KittyProtocol::WriteToTTY(const std::string& data,
                                int render_w, int render_h,
                                int term_row, int term_col) {
    PERF_LOG("kitty_write", "KittyProtocol::WriteToTTY row=" +
             std::to_string(term_row) + " col=" + std::to_string(term_col) +
             " render=" + std::to_string(render_w) + "x" + std::to_string(render_h) +
             " data_size=" + std::to_string(data.size()));

    int fd = ::open("/dev/tty", O_WRONLY);
    if (fd == -1) fd = ::open("/dev/tty", O_RDWR);
    if (fd == -1) {
        PERF_LOG("kitty_write", "FAILED to open /dev/tty errno=" +
                 std::to_string(errno));
        return;
    }

    char pos_buf[32];
    int pos_len = std::snprintf(pos_buf, sizeof(pos_buf),
                                "\033[%d;%dH", term_row + 1, term_col + 1);
    ssize_t r;
    r = ::write(fd, pos_buf, static_cast<size_t>(pos_len));
    if (r < 0) { PERF_LOG("kitty_write", "FAILED cursor pos write"); ::close(fd); return; }

    std::string header = "\033_Ga=T,f=100,s=" + std::to_string(render_w) +
                         ",v=" + std::to_string(render_h) + ",m=0;";
    r = ::write(fd, header.data(), header.size());
    if (r < 0) { PERF_LOG("kitty_write", "FAILED header write"); ::close(fd); return; }

    r = ::write(fd, data.data(), data.size());
    if (r < 0) { PERF_LOG("kitty_write", "FAILED data write"); ::close(fd); return; }
    PERF_LOG("kitty_write", "TTY wrote " + std::to_string(r) + " bytes");

    r = ::write(fd, "\033\\", 2);
    if (r < 0) { PERF_LOG("kitty_write", "FAILED ST write"); ::close(fd); return; }

    ::close(fd);
}

void KittyProtocol::ClearArea(int start_row, int num_rows,
                               int start_col, int width) {
    if (num_rows <= 0) return;
    PERF_LOG("kitty_clear", "KittyProtocol::ClearArea start=" +
             std::to_string(start_row) + " rows=" + std::to_string(num_rows) +
             " col=" + std::to_string(start_col) + " width=" + std::to_string(width));

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

}  // namespace FTB
