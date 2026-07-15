#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "protocols/TerminalImageProtocol.hpp"

namespace FTB {

class SixelProtocol : public TerminalImageProtocol {
public:
    struct AnimFrame {
        std::string sixel_data;
        int delay_ms = 100;
    };

    ~SixelProtocol() override { StopGifAnimation(); }

    std::string Name() const override { return "sixel"; }
    bool IsSupported() const override;
    std::string Encode(const std::string& path,
                        int pixel_w, int pixel_h,
                        int& out_term_rows,
                        int& out_render_w,
                        int& out_render_h) override;
    void WriteToTerminal(const std::string& data,
                          int render_w, int render_h,
                          int term_row, int term_col) override;
    void WriteToTTY(const std::string& data,
                     int render_w, int render_h,
                     int term_row, int term_col) override;
    void ClearArea(int start_row, int num_rows,
                    int start_col = 1, int width = 0) override;
    bool NeedsSkipArea() const override { return true; }

    // GIF animation via timer thread
    bool StartGifAnimation(int term_row, int term_col) override;
    void StopGifAnimation() override;
    bool IsGifAnimating() const override { return m_animating.load(); }

private:
    bool sixel_encode_frame(const unsigned char* rgba, int w, int h,
                            std::string& out_data);

    std::vector<AnimFrame> m_anim_frames;
    std::atomic<bool> m_animating{false};
    std::mutex m_anim_mutex;
    std::condition_variable m_anim_cv;
    std::thread m_anim_thread;
    int m_anim_render_w = 0;
    int m_anim_render_h = 0;
    int m_anim_term_row = 0;
    int m_anim_term_col = 0;

    // For delayed frame encoding in animation thread
    std::string m_gif_path;
    int m_gif_encode_max_w = 0;
    int m_gif_encode_max_h = 0;
};

}  // namespace FTB
