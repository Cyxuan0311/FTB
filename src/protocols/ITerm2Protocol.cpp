#include "../../include/protocols/ITerm2Protocol.hpp"

#include "../../include/protocols/base64_util.hpp"
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
#include "../../third_party/stb_image_resize2.h"
#include "../../third_party/stb_image_write.h"

namespace FTB {

bool ITerm2Protocol::IsSupported() const {
    auto& info = TerminalProbe::Detect();
    return info.iterm2;
}

struct PngWriteContext {
    std::vector<unsigned char>* buffer;
};

static void png_write_callback(void* context, void* data, int size) {
    auto* ctx = static_cast<PngWriteContext*>(context);
    auto* bytes = static_cast<const unsigned char*>(data);
    ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
}

std::string ITerm2Protocol::Encode(
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
        stbir_resize_uint8_srgb(data, img_w, img_h, 0,
                                scaled, render_w, render_h, 0,
                                STBIR_RGB);
    } else {
        scaled = data;
    }

    out_render_w = render_w;
    out_render_h = render_h;
    out_term_rows = (render_h + 17) / 18;

    std::vector<unsigned char> png_data;
    PngWriteContext ctx = {&png_data};
    int png_ok = stbi_write_png_to_func(png_write_callback, &ctx,
                                        render_w, render_h, 3,
                                        scaled, 0);
    if (scaled != data) std::free(scaled);
    stbi_image_free(data);

    if (!png_ok) {
        return {};
    }

    std::string b64 = detail::Base64Encode(png_data.data(), png_data.size());
    return b64;
}

void ITerm2Protocol::WriteToTerminal(const std::string& data,
                                      int render_w, int render_h,
                                      int term_row, int term_col) {

    std::string seq = "\033[" + std::to_string(term_row + 1) + ";" +
                      std::to_string(term_col + 1) + "H" +
                      "\033]1337;File=:size=" + std::to_string(data.size()) +
                      ";inline=1;width=" + std::to_string(render_w) + "px;" +
                      "height=" + std::to_string(render_h) + "px:" +
                      data + "\a";

    auto& tmux = TmuxContext::Instance();
    if (tmux.InTmux() && tmux.PassthroughEnabled()) {
        std::cout << tmux.WrapPassthrough(seq) << std::flush;
    } else {
        std::cout << seq << std::flush;
    }
}

void ITerm2Protocol::WriteToTTY(const std::string& data,
                                 int render_w, int render_h,
                                 int term_row, int term_col) {

    int fd = ::open("/dev/tty", O_WRONLY);
    if (fd == -1) fd = ::open("/dev/tty", O_RDWR);
    if (fd == -1) {
        return;
    }

    // Build the full sequence: cursor pos + OSC 1337 image
    std::string seq = "\033[" + std::to_string(term_row + 1) + ";" +
                      std::to_string(term_col + 1) + "H" +
                      "\033]1337;File=:size=" + std::to_string(data.size()) +
                      ";inline=1;width=" + std::to_string(render_w) + "px;" +
                      "height=" + std::to_string(render_h) + "px:" +
                      data + "\a";

    auto& tmux = TmuxContext::Instance();
    if (tmux.InTmux() && tmux.PassthroughEnabled()) {
        std::string wrapped = tmux.WrapPassthrough(seq);
        ssize_t r = ::write(fd, wrapped.data(), wrapped.size());
        if (r < 0) { ::close(fd); return; }
    } else {
        ssize_t r = ::write(fd, seq.data(), seq.size());
        if (r < 0) { ::close(fd); return; }
    }

    ::close(fd);
}

void ITerm2Protocol::ClearArea(int start_row, int num_rows,
                                int start_col, int width) {
    if (num_rows <= 0) return;

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
