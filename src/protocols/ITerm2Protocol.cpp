#include "../../include/protocols/ITerm2Protocol.hpp"
#include "../../include/utils/WebpDecoder.hpp"
#include "../../include/utils/PerfLogger.hpp"

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

    // Check for GIF → decode frames for animation if possible
    std::string ext;
    auto dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (ext == ".gif") {
        m_gif_base64_frames.clear();
        m_gif_delays.clear();
        m_gif_render_w = 0;
        m_gif_render_h = 0;

        int render_w = 0, render_h = 0;
        bool dec_ok = GifFrameDecoder::Decode(path, max_width, max_height,
            [&](int i, DecodedFrame& frame) -> bool {
                std::string b64 = rgba_to_png_base64(
                    frame.rgba.data(), frame.width, frame.height);
                if (b64.empty()) {
                    m_gif_base64_frames.clear();
                    m_gif_delays.clear();
                    return false;
                }
                m_gif_base64_frames.push_back(std::move(b64));
                m_gif_delays.push_back(frame.delay_ms);
                return true;
            }, render_w, render_h);
        if (dec_ok) {
            out_render_w = render_w;
            out_render_h = render_h;
            out_term_rows = (render_h + 17) / 18;
            m_gif_render_w = render_w;
            m_gif_render_h = render_h;

            if (!m_gif_base64_frames.empty()) {
                return m_gif_base64_frames[0];
            }
        }
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

bool ITerm2Protocol::StartGifAnimation(int term_row, int term_col) {
    char cwd_buf[1024];
    PERF_LOG("ITerm2Gif", "StartGifAnimation row=" + std::to_string(term_row)
        + " col=" + std::to_string(term_col)
        + " frames=" + std::to_string(m_gif_base64_frames.size())
        + " cwd=" + std::string(::getcwd(cwd_buf, sizeof(cwd_buf)) ? cwd_buf : "?"));
    if (m_gif_base64_frames.empty()) return false;

    StopGifAnimation();

    int id = ++m_gif_image_id;
    if (id <= 0) id = 1;

    auto& info = TerminalProbe::Detect();
    auto& tmux = TmuxContext::Instance();
    bool use_direct_tty = info.needs_direct_tty;
    const std::string& tty_path = use_direct_tty ? tmux.OuterTty() : std::string("/dev/tty");
    int row_off = use_direct_tty ? tmux.PaneTop() : 0;
    int col_off = use_direct_tty ? tmux.PaneLeft() : 0;

    struct stat tty_st;
    bool tty_is_chr = (::stat(tty_path.c_str(), &tty_st) == 0 && S_ISCHR(tty_st.st_mode));
    PERF_LOG("ITerm2Write", "tty_path=" + tty_path + " use_direct_tty="
        + std::to_string(use_direct_tty) + " is_chr_dev=" + std::to_string(tty_is_chr));

    auto write_apc = [&](const std::string& seq) {
        int fd = ::open(tty_path.c_str(), O_WRONLY);
        if (fd == -1) {
            PERF_LOG("ITerm2Write", "open(O_WRONLY) FAILED path=" + tty_path
                + " errno=" + std::to_string(errno));
            fd = ::open(tty_path.c_str(), O_RDWR);
        }
        if (fd == -1) {
            PERF_LOG("ITerm2Write", "open(O_RDWR) FAILED path=" + tty_path
                + " errno=" + std::to_string(errno));
            return;
        }
        struct stat st;
        bool is_tty = (::fstat(fd, &st) == 0 && S_ISCHR(st.st_mode));
        ssize_t written = ::write(fd, seq.data(), seq.size());
        if (written < 0) {
            PERF_LOG("ITerm2Write", "write FAILED fd=" + std::to_string(fd)
                + " is_tty=" + std::to_string(is_tty)
                + " seq_len=" + std::to_string(seq.size())
                + " errno=" + std::to_string(errno));
        }
        PERF_LOG("ITerm2Write", "write fd=" + std::to_string(fd)
            + " is_tty=" + std::to_string(is_tty)
            + " seq_len=" + std::to_string(seq.size())
            + " written=" + std::to_string(written));
        ::close(fd);
    };

    for (size_t i = 0; i < m_gif_base64_frames.size(); ++i) {
        std::string apc;
        int z = std::max(1, m_gif_delays[i] / 10);
        if (i == 0) {
            char header[256];
            std::snprintf(header, sizeof(header),
                "\033[%d;%dH\033_Ga=f,f=100,s=%d,v=%d,i=%d,c=0,z=%d;",
                term_row + 1 + row_off, term_col + 1 + col_off,
                m_gif_render_w, m_gif_render_h, id, z);
            apc = header;
        } else {
            char header[128];
            std::snprintf(header, sizeof(header),
                "\033_Ga=f,i=%d,c=%zu,z=%d;",
                id, i, z);
            apc = header;
        }
        apc += m_gif_base64_frames[i] + "\033\\";
        write_apc(apc);
    }

    char ctrl[64];
    std::snprintf(ctrl, sizeof(ctrl), "\033_Ga=a,i=%d,s=3,v=1\033\\", id);
    write_apc(ctrl);

    m_gif_running = true;
    return true;
}

}  // namespace FTB
