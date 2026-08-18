# Display ++

This is the next evoluion of the display code.

We break down the Apple II motherboard display system into several stages.

1. Generate 560x192 bitstream.
2. Render bitstream to a pixelmap (and then texture)
3. Render texture to window

There are a few motivations for this:

1. have all display routines work on data in and out in the same manner and with the same interfaces
2. allow great stuff like WmS cycle-count based display logic
3. allow API for "give me the current lo-res/text/hgr/dhgr frame" for the debugger.

Currently we have:

update_display_apple2

This loops the 24 "lines" and if changed, calls the line type specified. However, is is calling functions named stuff like "render_line_ntsc". So, that's inside-out from what we want. This part of the code should not care what the final rendering routine is.

So, update_frame_apple2 should be modified to call function that generates an intermediate bit stream:

* generate_line_text
* generate_line_lgr
* generate_line_hgr
* generate_line_text80
* generate_line_dlgr
* generate_line_dhgr

Something then sends the frame to one of these display render routines:

* render_frame_ntsc
* render_frame_rgb
* render_frame_mono

#define BITSTREAM_WIDTH 567 - it's still 560, it's just displayed earlier. We compose into pixelmap in different spot.
#define BITSTREAM_WIDTH_ULL ((uint64_t)BISTREAM_WIDTH+64)/64

This is because the IIe starts lines 7 pixels early (and phase shifted) when in 80-column modes. We may want to tweak this to include the borders. And the GS is even more differenter.

#define BITSTREAM_HEIGHT 192

Same questions here.

class Frame_Bitstream
    Bitstream(uint16_t width, uint16_t height)  // pixels
    uint64_t display_bitstream[BITSTREAM_HEIGHT][BITSTREAM_WIDTH_ULL]
    uint64_t working = 0;
    int scanline = 0;
    int hpos = 0;
    int hloc = 0;
    push(bit) { working <<1 | bit; hpos++; if hpos>=64 { display_bitstream[scanline][hloc++] = working; } }
    bool pull() { return working & (1 << 63); hpos++; if hpos>=64 { working = display_bitstream[scanline][hloc++]} };
    setline(line) { scanline = line; hpos = 0; hloc = 0; }

use cases:
    setline(line); then push a line's worth of bits.
    setline(line); then pull a line's worth of bits;
    mixing will have undefined results.

display() owns one of these. But the debugger can allocate its own and call the generators and renderers to get its own display images of not-the-current-display-mode.
(For extra credit: implement a VNC server inside GS2 that allows remote access to the VM - how hard could that be? tee hee).
We can also do a "wayback" thing that lets you see video frames from prior points in time. Offering a kind of scrollback functionality. These compact bitstreams would be very memory efficient.

directories

displaypp/render/ntsc.cpp
displaypp/render/rgb.cpp
displaypp/render/mono.cpp
displaypp/frame/frame.cpp, .hpp
displaypp/generate/text.cpp
displaypp/generate/text80.cpp
displaypp/generate/cyclestream.cpp
etc.

## cycle-based video stream

stream.cpp is what handles cycle-based video streaming for emulating cycle-accurate video routines on the II/II+/IIe/IIc. When activated, this feature causes incr_cycle to lookup and read current video byte, inside incr_cycle. Then that is streamed to a buffer that holds three byte values:
[mode] [maindata] [altdata]

Then generate/cyclestream.cpp reads that and renders a whole frame to a Frame_Bitstream. Then rendering stage as usual.

# Pixel Encoding

Pixels with the new Frame concept are 1 or 0. However, I think the NTSC code expects 0xFF or 0.
Monochrome seems like the easist place to start, and, from the bottom up.
I wonder if I should have a little bit more formal pixelmap class. Yes.
ok I created a template version of the class that will take any old value, 0, 1, 0xFF. But I should decide if there is a reason I want to keep using 0xFF.

PIXELFORMAT_ABGR_8888 is twice as fast as PIXELFORMAT_RGBA! Well now that's special. I still seem to be going way slower than gs2 itself..

differences are that I'm using rectangles. I could try that..

## Character ROMs

So, the IIe character ROMs are 4K for US, and 8K for foreign.
The first 2K seems to be characters as usual.
The next 2K seems to be related to the character index. Feels graphics-related in some way.. maybe they feed bits into hires / double hires with this?

```
Character 0x1FD (offset 0x0FE8):
*.....*.  
*.....*.  
..*...*.  
*...*...
*.....*.
*.....*.
........
........

Character 0x17D (offset 0x0BE8):
*.....*.
*.....*.
..*...*.
*...*...
*.....*.
*.....*.
*...*...
..*...*.
```

The bit order is reversed compared to II+. Also, the meaning of bits may be reversed here too (i.e., 1 means "pixel off"). So we need to decide whether to reverse the ROM file. I could have different code modules for each, but, it seems like a smart ROM loader that would mangle the bits the way we need so the same routine could draw ii+, iie, what have you would be the way to go. I think we want to reverse the II+ one, and shift out the right. 

## Quick entry into Double Hi-Res

If you know how to write to the iie 80 column text screen directly, it should be easy enough.
The cleanest sample I have seen to turn it on is
10 D$=CHR$(4)
20 PRINT D$;"PR#3" : REM 80-column mode
30 POKE 49246,0 : REM $C05E - enable double resolution graphics
40 GR : REM Turn on double lo-res mode
Once in this mode, data stored to the 80 column screen will show up as double low res. On the iigs you can just use the plot command in AppleSoft. I think on the iie you have to do it with pokes.

ok have this cool program to generate a lores test pattern. (it's potest.po : GR80TEST)

however my lo80 of course starts with the wrong phase. All the 80-column stuff needs to start 180 degrees off.
So we need to know what phase to start on.

## RGB Algorithm from Patent 4,786,893.

This appears to be related to how the IIgs converts the hires bitstream into RGB signals.

Block 20: Detect Change in Color Pattern

Block 21: Examine Last four bits for each new bit in bit stream

Block 22: As shown by block 21, the last four bits in the bit stream are examined and as shown by block 22, this is done to determine the color. For instance, if the color is contained in bits ABCD, the color is determined from these bits when in this order. When the next bit A arrives, the bit stream appears as BCDA. The bits are rearranged to again read ABCD to determine the color.

If a change in color is detected from block 20 the pattern that existed before the change is used until a certain or predetermined condition is met (block 23). Thus if from the bit stream DABC a color change is detected the pattern that existed before this detected change (CDAB) after being reordered to ABCD is used as the color. After the predetermined condition is met, the color then being detected is used. This in effect eliminates the transition color. The predetermined condition can be an algorithm such as implemented in Fig 4, or simply the passage of a predetermined number of bits.

BUT the preferred embodiment of the present invention is Fig 4.

So the first step here is: bits in, rearrange.

11001100

We start by reading bit A, then B, C, D in that order, and view them like so:

ABCD = 1100

the next bit we read is back to A, and we have:

BCDA = 1001

But then we reorder to ABCD, and get:
ABCD = 1100

ok, that's cool.

The next bit we read is B = 1, and we have

CDAB = 0011

Again, rearrange to get:

ABCD = 1100

So let's call where we're at the "phase".

ABCD - phase 0
BCDA - phase 1
CDAB - phase 2
DABC - phase 3

We can use a set of four 16-entry lookup tables in lieu of the barrel shifter 31. So we're going to have:

* phase
* ShiftReg
* ReordVal
* LatchedColor
* JK43
* JK44

43 and 44 are flip-flops with J/K inputs. 

| J | K | Qnext | Comment |
|---|---|-------|---------|
| 0 | 0 | Q | No change |
| 0 | 1 | 0 | Reset |
| 1 | 0 | 1 | Set |
| 1 | 1 | !Q | Toggle |

where Q is the current value.

then for control signals:

These are flip-flops, so they latch the last value and will change *after* our virtual cycle.

43:
!SR3 & INP -> J
SR2 -> K

44:
SR3 & !INP -> J
!SR2 -> K

OR of flip-flop outputs then selects either the LATCH data or BS data as coded RGB output.

Then we look up the RGB color we want from a 16-entry LUT that converts our 4-bit index into RGB.

https://archive.org/details/IIgs_2523063_Master_Color_Values

This is a IIGS technical note mapping the color values on the IIGS to RGB.

| Color Name | Color Register Value | Master Color Value |
|-|-|-|
| Black | $0 | $0000 |
| Deep Red | $1 | $0D03 |
| Dark Blue | $2 | $0009 |
| Purple | $3 | $0D2D |
| Dark Green | $4 | $0072 |
| Dark Gray | $5 | $0555 |
| Medium Blue | $6 | $022F |
| Light Blue | $7 | $06AF |
| Brown | $8 | $0850 |
| Orange | $9 | $0F60 |
| Light Gray | $A | $0AAA |
| Pink | $B | $0F98 |
| Light Green | $C | $01DO |
| Yellow  | $D | $0FF0 |
| Aquamarine | $E | $04F9 |
| White | $F | $0FFF |

Andy McFadden in this post:

https://comp.sys.apple2.narkive.com/lTSrj2ZI/apple-ii-colour-rgb

says:

See Apple IIgs tech note #63:

http://www.gno.org/pub/apple2/doc/apple/technotes/iigs/tn.iigs.063

I believe the "border color" table entries correspond to the lo-res and double hi-res colors. The purple, orange, medium blue, and light green correspond to the hi-res colors.

"Assume the embodiment of Fig 6 is in use. the counter 51 counts, for example, 3 counts or cycles. After the third count, the color 0010 is used as shown in Fig 5. Thus the fringe colors are not displayed."

