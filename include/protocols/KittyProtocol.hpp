#pragma once

#include <string>
#include <vector>

#include "protocols/TerminalImageProtocol.hpp"

namespace FTB {

class KittyProtocol : public TerminalImageProtocol {
public:
    std::string Name() const override { return "kitty"; }
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
    bool NeedsSkipArea() const override { return false; }

    // GIF animation (reserved, currently disabled)
    bool StartGifAnimation(int term_row, int term_col) override;
    void StopGifAnimation() override {}
    bool IsGifAnimating() const override { return false; }

private:
};

}  // namespace FTB
