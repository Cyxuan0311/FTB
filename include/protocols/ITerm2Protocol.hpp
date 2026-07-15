#pragma once

#include <string>
#include <vector>

#include "protocols/TerminalImageProtocol.hpp"

namespace FTB {

class ITerm2Protocol : public TerminalImageProtocol {
public:
    std::string Name() const override { return "iterm2"; }
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

    // GIF animation via Kitty protocol (WezTerm/iTerm2 support it)
    bool StartGifAnimation(int term_row, int term_col) override;
    void StopGifAnimation() override { m_gif_running = false; }
    bool IsGifAnimating() const override { return m_gif_running; }

private:
    bool m_gif_running = false;
    int m_gif_image_id = 0;
    std::vector<std::string> m_gif_base64_frames;
    std::vector<int> m_gif_delays;
    int m_gif_render_w = 0;
    int m_gif_render_h = 0;
};

}  // namespace FTB
