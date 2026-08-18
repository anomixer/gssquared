#pragma once

#include <cstdint>

class ScanBuffer;
class Render;

/** Shared video scan generator API for composite and RGB implementations. */
class VideoScanGeneratorIntf {
public:
    virtual ~VideoScanGeneratorIntf() = default;

    virtual void generate_frame(ScanBuffer *frame_scan) = 0;

    virtual void set_display_shift(bool enable) = 0;
    virtual void set_dhgr_mono_mode(bool mono) = 0;
    virtual bool get_dhgr_mono_mode() const = 0;
    virtual void set_mono_mode(bool mono) = 0;
    virtual bool get_mono_mode() const = 0;
    virtual void set_char_set(uint16_t char_set) = 0;
    virtual void set_render(Render *render) = 0;
    virtual void setDumpNextFrame(bool dump) = 0;

    virtual uint32_t get_h() const = 0;
    virtual uint32_t get_v() const = 0;
};
