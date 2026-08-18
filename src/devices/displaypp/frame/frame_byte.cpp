#include <cstdint>
#include <cstdio>
#include <cstring>

#include "frame.hpp"

#if 0
Frame_Bytestream::Frame_Bytestream(uint16_t width, uint16_t height) { // pixels
    f_width = width;
    f_height = height;
    scanline = 0;
    hloc = 0;
}

Frame_Bytestream::~Frame_Bytestream() {
  // nothing to do
}

void Frames_Bytestream::print() {
    // nope
    printf("Frame: %u x %u\n", f_width, f_height);
    for (int i = 0; i < f_height; i++) {
        set_line(i);
        uint16_t linepos = 0;
        for (int j = 0; j < f_width / 64; j++) {
            uint64_t wrd = 0;
            for (int b = 0; b < 64; b++) {
                wrd = (wrd << 1) | pull();
                if (linepos++ >= f_width) break; // don't go past the end of the line
            }
            printf("%016llx ", wrd);
            if (linepos >= f_width) break; // don't go past the end of the line
        }
        printf("\n");
    }
}

void Frame_Bytestream::clear() {
    // don't need this when we're bytewise
    ///memset(display_bitstream, 0, sizeof(display_bitstream));
}
#endif