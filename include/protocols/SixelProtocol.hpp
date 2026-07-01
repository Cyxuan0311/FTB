#pragma once

#include "protocols/TerminalImageProtocol.hpp"

namespace FTB {

class SixelProtocol : public TerminalImageProtocol {
public:
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
};

}  // namespace FTB
