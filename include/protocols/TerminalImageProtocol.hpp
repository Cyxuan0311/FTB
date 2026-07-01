#pragma once

#include <string>

namespace FTB {

class TerminalImageProtocol {
public:
    virtual ~TerminalImageProtocol() = default;

    virtual std::string Name() const = 0;

    virtual bool IsSupported() const = 0;

    virtual std::string Encode(const std::string& path,
                                int pixel_w, int pixel_h,
                                int& out_term_rows,
                                int& out_render_w,
                                int& out_render_h) = 0;

    virtual void WriteToTerminal(const std::string& data,
                                  int render_w, int render_h,
                                  int term_row, int term_col) = 0;

    virtual void WriteToTTY(const std::string& data,
                             int render_w, int render_h,
                             int term_row, int term_col) = 0;

    virtual void ClearArea(int start_row, int num_rows,
                            int start_col = 1, int width = 0) = 0;

    virtual bool NeedsSkipArea() const { return true; }
};

}  // namespace FTB
