#include "../../include/utils/GifFrameDecoder.hpp"
#include "../../include/utils/PerfLogger.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "gif_lib.h"
#include "../../third_party/stb_image_resize2.h"

namespace FTB {

static void CompositeFrame(
    GifFileType* gif,
    const SavedImage* frame,
    std::vector<unsigned char>& canvas)
{
    int cw = gif->SWidth;
    int ch = gif->SHeight;

    int fl = frame->ImageDesc.Left;
    int ft = frame->ImageDesc.Top;
    int fw = frame->ImageDesc.Width;
    int fh = frame->ImageDesc.Height;

    ColorMapObject* cmap = frame->ImageDesc.ColorMap
        ? frame->ImageDesc.ColorMap
        : gif->SColorMap;
    if (!cmap) return;

    GifRowType raster = frame->RasterBits;
    if (!raster) return;

    GraphicsControlBlock gcb;
    bool has_gcb = (DGifSavedExtensionToGCB(gif, static_cast<int>(
        frame - gif->SavedImages), &gcb) == GIF_OK);
    int transparent = has_gcb ? gcb.TransparentColor : -1;

    for (int y = 0; y < fh; ++y) {
        int cy = ft + y;
        if (cy < 0 || cy >= ch) continue;
        for (int x = 0; x < fw; ++x) {
            int cx = fl + x;
            if (cx < 0 || cx >= cw) continue;
            int idx = raster[y * fw + x];
            if (idx == transparent) continue;
            if (idx < 0 || idx >= cmap->ColorCount) continue;
            int dst = (cy * cw + cx) * 3;
            canvas[dst + 0] = cmap->Colors[idx].Red;
            canvas[dst + 1] = cmap->Colors[idx].Green;
            canvas[dst + 2] = cmap->Colors[idx].Blue;
        }
    }
}

static void GetBgColor(const GifFileType* gif,
                       unsigned char& r, unsigned char& g, unsigned char& b)
{
    if (gif->SColorMap && gif->SBackGroundColor < gif->SColorMap->ColorCount) {
        r = gif->SColorMap->Colors[gif->SBackGroundColor].Red;
        g = gif->SColorMap->Colors[gif->SBackGroundColor].Green;
        b = gif->SColorMap->Colors[gif->SBackGroundColor].Blue;
    } else {
        r = g = b = 0;
    }
}

static void ClearFrameArea(
    int cw, int ch,
    int fl, int ft, int fw, int fh,
    std::vector<unsigned char>& canvas,
    unsigned char bg_r, unsigned char bg_g, unsigned char bg_b)
{
    for (int y = 0; y < fh; ++y) {
        int cy = ft + y;
        if (cy < 0 || cy >= ch) continue;
        for (int x = 0; x < fw; ++x) {
            int cx = fl + x;
            if (cx < 0 || cx >= cw) continue;
            int dst = (cy * cw + cx) * 3;
            canvas[dst + 0] = bg_r;
            canvas[dst + 1] = bg_g;
            canvas[dst + 2] = bg_b;
        }
    }
}

bool GifFrameDecoder::Decode(
    const std::string& path,
    int max_w, int max_h,
    FrameCallback callback,
    int& out_w, int& out_h)
{
    out_w = 0;
    out_h = 0;

    int error = 0;
    GifFileType* gif = DGifOpenFileName(path.c_str(), &error);
    if (!gif) return false;

    if (DGifSlurp(gif) == GIF_ERROR) {
        DGifCloseFile(gif, &error);
        return false;
    }

    int cw = gif->SWidth;
    int ch = gif->SHeight;
    int total = gif->ImageCount;
    if (cw <= 0 || ch <= 0 || total <= 0) {
        DGifCloseFile(gif, &error);
        return false;
    }

    double target_ratio = static_cast<double>(cw) / ch;
    int render_w, render_h;
    if (static_cast<double>(max_w) / max_h > target_ratio) {
        render_h = max_h;
        render_w = std::max(1, static_cast<int>(max_h * target_ratio));
    } else {
        render_w = max_w;
        render_h = std::max(1, static_cast<int>(max_w / target_ratio));
    }

    out_w = render_w;
    out_h = render_h;

    unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
    GetBgColor(gif, bg_r, bg_g, bg_b);

    std::vector<unsigned char> canvas(static_cast<size_t>(cw * ch * 3));
    for (size_t i = 0; i < canvas.size(); i += 3) {
        canvas[i + 0] = bg_r;
        canvas[i + 1] = bg_g;
        canvas[i + 2] = bg_b;
    }
    std::vector<unsigned char> prev_canvas;

    size_t frame_stride = static_cast<size_t>(render_w * render_h * 3);
    int done_count = 0;

    for (int i = 0; i < total; ++i) {
        SavedImage* frame = &gif->SavedImages[i];

        GraphicsControlBlock gcb;
        bool has_gcb = (DGifSavedExtensionToGCB(gif, i, &gcb) == GIF_OK);
        int disposal = has_gcb ? gcb.DisposalMode : DISPOSAL_UNSPECIFIED;

        if (disposal == DISPOSE_PREVIOUS) {
            prev_canvas = canvas;
        }

        CompositeFrame(gif, frame, canvas);

        DecodedFrame df;
        df.rgba.resize(frame_stride);
        df.width = render_w;
        df.height = render_h;

        if (render_w == cw && render_h == ch) {
            std::memcpy(df.rgba.data(), canvas.data(),
                        static_cast<size_t>(cw * ch * 3));
        } else {
            stbir_resize_uint8_srgb(
                canvas.data(), cw, ch, 0,
                df.rgba.data(), render_w, render_h, 0,
                STBIR_RGB);
        }

        int delay_ms = has_gcb ? gcb.DelayTime * 10 : 100;
        if (delay_ms <= 0) delay_ms = 100;
        if (delay_ms < 20) delay_ms = 20;
        df.delay_ms = delay_ms;

        if (!callback(i, df)) break;
        ++done_count;

        switch (disposal) {
        case DISPOSE_BACKGROUND: {
            int fl = frame->ImageDesc.Left;
            int ft = frame->ImageDesc.Top;
            int fw = frame->ImageDesc.Width;
            int fh = frame->ImageDesc.Height;
            ClearFrameArea(cw, ch, fl, ft, fw, fh, canvas,
                           bg_r, bg_g, bg_b);
            break;
        }
        case DISPOSE_PREVIOUS:
            canvas = prev_canvas;
            break;
        default:
            break;
        }
    }

    DGifCloseFile(gif, &error);
    return done_count > 0;
}

}
