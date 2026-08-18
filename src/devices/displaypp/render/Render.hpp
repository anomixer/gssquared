#pragma once

#include "devices/displaypp/frame/frame.hpp"
#include "devices/displaypp/frame/Frames.hpp"

class Render {

    public:
        Render(bool shift_enabled = true) : shift_enabled(shift_enabled) {};
        ~Render() {};

        void set_shift_enabled(bool shift_enabled) { this->shift_enabled = shift_enabled; }
        void set_mono_color(RGBA_t color) { this->mono_color = color; }
        virtual void render(Frame560 *frame_byte, FrameVSG *frame_vsg) = 0;

    protected:
        bool shift_enabled = false;
        static constexpr RGBA_t black = RGBA_t::make(0x00, 0x00, 0x00, 0xFF);
        RGBA_t mono_color = RGBA_t::make(0xFF, 0xFF, 0xFF, 0xFF);
};