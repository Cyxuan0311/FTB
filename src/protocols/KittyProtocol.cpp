#include "../../include/protocols/KittyProtocol.hpp"
#include "../../include/utils/WebpDecoder.hpp"
#include "../../include/protocols/base64_util.hpp"
#include "../../include/utils/TerminalProbe.hpp"
#include "../../include/utils/TmuxContext.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "../../third_party/stb_image.h"
#include "../../third_party/stb_image_resize2.h"
#include "../../third_party/stb_image_write.h"

namespace FTB {

bool KittyProtocol::IsSupported() const {
    auto& info = TerminalProbe::Detect();
    return info.kitty;
}

struct PngWriteContext {
    std::vector<unsigned char>* buffer;
};

static void png_write_callback(void* context, void* data, int size) {
    auto* ctx = static_cast<PngWriteContext*>(context);
    auto* bytes = static_cast<const unsigned char*>(data);
    ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
}

static std::string rgba_to_png_base64(const unsigned char* rgba, int w, int h) {
    std::vector<unsigned char> png_data;
    PngWriteContext ctx = {&png_data};
    int ok = stbi_write_png_to_func(png_write_callback, &ctx,
                                    w, h, 3, rgba, 0);
    if (!ok) return {};
    return detail::Base64Encode(png_data.data(), png_data.size());
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

    if (!png_ok) {
        return {};
    }

    return detail::Base64Encode(png_data.data(), png_data.size());
}

void KittyProtocol::WriteToTerminal(const std::string& data,
                                     int render_w, int render_h,
                                     int term_row, int term_col) {

    constexpr size_t CHUNK = 4096;
    size_t total = data.size();
    size_t offset = 0;
    bool first = true;

    while (offset < total) {
        size_t this_chunk = std::min(CHUNK, total - offset);
        bool more = (offset + this_chunk) < total;

        if (first) {
            std::cout << "\033[" << term_row + 1 << ";" << term_col + 1 << "H"
                      << "\033_Ga=T,f=100,s=" << render_w
                      << ",v=" << render_h
                      << ",m=" << (more ? '1' : '0') << ";";
            first = false;
        } else {
            std::cout << "\033_Gm=" << (more ? '1' : '0') << ";";
        }

        std::cout.write(data.data() + offset, static_cast<std::streamsize>(this_chunk));
        std::cout << "\033\\";

        offset += this_chunk;
    }

    std::cout << std::flush;
}

void KittyProtocol::WriteToTTY(const std::string& data,
                                int render_w, int render_h,
                                int term_row, int term_col) {
    auto& info = TerminalProbe::Detect();
    auto& tmux = TmuxContext::Instance();
    bool use_direct_tty = info.needs_direct_tty;
    const std::string& tty_path = use_direct_tty ? tmux.OuterTty() : std::string("/dev/tty");
    int row_off = use_direct_tty ? tmux.PaneTop() : 0;
    int col_off = use_direct_tty ? tmux.PaneLeft() : 0;

    int fd = ::open(tty_path.c_str(), O_WRONLY);
    if (fd == -1) fd = ::open(tty_path.c_str(), O_RDWR);
    if (fd == -1) {
        return;
    }

    constexpr size_t CHUNK = 4096;
    size_t total = data.size();
    size_t offset = 0;
    bool first = true;

    while (offset < total) {
        size_t this_chunk = std::min(CHUNK, total - offset);
        bool more = (offset + this_chunk) < total;
        ssize_t r;

        if (first) {
            char pos_header[128];
            int pos_header_len = std::snprintf(pos_header, sizeof(pos_header),
                "\033[%d;%dH\033_Ga=T,f=100,s=%d,v=%d,m=%c;",
                term_row + 1 + row_off, term_col + 1 + col_off,
                render_w, render_h,
                more ? '1' : '0');
            r = ::write(fd, pos_header, static_cast<size_t>(pos_header_len));
            if (r < 0) {
                ::close(fd);
                return;
            }
            first = false;
        } else {
            char mid_header[16];
            int mid_header_len = std::snprintf(mid_header, sizeof(mid_header),
                "\033_Gm=%c;", more ? '1' : '0');
            r = ::write(fd, mid_header, static_cast<size_t>(mid_header_len));
            if (r < 0) {
                ::close(fd);
                return;
            }
        }

        r = ::write(fd, data.data() + offset, this_chunk);
        if (r < 0) {
            ::close(fd);
            return;
        }

        r = ::write(fd, "\033\\", 2);
        if (r < 0) {
            ::close(fd);
            return;
        }

        offset += this_chunk;
    }

    ::close(fd);
}

void KittyProtocol::ClearArea(int start_row, int num_rows,
                               int start_col, int width) {
    if (num_rows <= 0) return;
    auto& info = TerminalProbe::Detect();
    auto& tmux = TmuxContext::Instance();
    bool use_direct_tty = info.needs_direct_tty;
    int row_off = use_direct_tty ? tmux.PaneTop() : 0;
    int col_off = use_direct_tty ? tmux.PaneLeft() : 0;

    if (use_direct_tty) {
        int fd = ::open(tmux.OuterTty().c_str(), O_WRONLY);
        if (fd == -1) fd = ::open(tmux.OuterTty().c_str(), O_RDWR);
        if (fd != -1) {
            static const char delete_imgs[] = "\033_Ga=d\033\\";
            ssize_t unused_unused = ::write(fd, delete_imgs, sizeof(delete_imgs) - 1);
            (void)unused_unused;
            ::close(fd);
        }
    } else {
        static const char delete_imgs[] = "\033_Ga=d\033\\";
        ssize_t unused_unused = ::write(STDOUT_FILENO, delete_imgs, sizeof(delete_imgs) - 1);
        (void)unused_unused;
    }

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

bool KittyProtocol::StartGifAnimation(int term_row, int term_col) {
    return false;
}

}  // namespace FTB
