# Display Next Generation

Currently hgrdecode uses source data of single bytes that are either FF or 00.

The new ntsc.cpp implementation in it converts to bits.

We use a 17-bit sliding window of the bits and feed that into a lookup table. Instead of starting with bytes and converting to bits, just start with bits and feed bits. This routine is then going to be hyper-efficient.

We're going to want routines that adds certain numbers of bits to the bit stream, that all the display modes will use:
   * 4 bits, for lo-res.
   * 1 bit, for text mode.
   * 14 bits for hires mode.
   * 2 bits for double hires.

All the display modes will be reimplemented the same way - as a 560x192 bit grid, that will be fed directly into the composite renderer.

Display data is converted from whatever source format, into bits.

64 bits 

Since 560 isn't divisible by 64, we'll pad to get us to 576 bits per line layout in memory.

Then the rendering routine will have a func that provides the next 17-bit window of bits from the source data. This can all be implemented with keeping a counter, and two 64-bit values from the source line. The current 17-bit value is just "reg0 & 0x1FFFF" into the lookup table. Whenever reg1 is empty we fetch the next 64-bit value from the source line or a 0 if we're at the end of the line.

One goal of this, is to optimize for caching. The M1 L1 cache line size is 128 bytes. This means that every one of our scanlines will be able to be fetched with either 1 or 2 cache lines, only two memory accesses. Most of the heavy lifting will be done shifting stuff in registers.

L1 is 128KB, L2 is 12MB. This means the entire lookup table will easily fit into L2 cache. We may want to lay out the lookup table in memory in a way that further optimizes cache usage. For instance, we know we will iterate across the phase parameter 0, 0.25, 0.5, 0.75 as we process a scanline. If we've got a run of similar color pixels (e.g. 101010101010) that will be effectively cached. Easy enough to test.

We can do testing to see if the 13-bit is sufficient - it would work the same way. It will use less memory for the lookup table. And may affect overall cache performance. And would be faster to regenerate if the user wants to change any of the input parameters.

The renderer iterates 192 scanlines and 560 pixels per line, drawing each pixel.
It will have two modes:
   * color-kill (i.e., white or monochrome)
   * composite

Maybe I will eventually create a RGB mode that will have different filter parameters going into the lookup table - faster response times.

If we are in a pure text mode, we use color-kill. If we are in a graphics mode, either full or mixed mode, we use the composite mode for the whole frame. Or, do we use a LUT for monochrome that captures the luma value from the composite calculations? I guess it depends on whether luma ever is significantly different from 1. Should be able to back-convert a rendered image to YUV to look at the Y values. If there is variation in the Y values then it would make sense to do LUT for monochrome. If not, just do it the way we do it not.

We don't have to re-calculate the LUT whenever params change. We're a desktop app, we can just pre-generate as many different LUTs as we want to support different modes and mode combinations.

Currently it takes 100microseconds to render a frame. I am betting that we can reduce this quite a bit further.

On the one hand, at 100uS we are already doing great. On the other hand, this is with pretty fast modern hardware. If we can optimize more we can improve operation on older hardware.

## Pseudo-code for frame rendering

At startup time:

   * buildHires40Font(MODEL_IIE, 0);
   * setupConfig();
   * init_hgr_LUT();
   * Create bit image frame buffer - 560 x 192 bytes.

In main loop:
   * For each dirty "line" (8 scanlines) call the appropriate mode function to update the bit image frame buffer.
   * Call the composite renderer to render just the 8 scanlines. call generateHiresScanLine() for each of the 8 scanlines.
   *   processAppleIIFrame_LUT(graymap, hgrdecode_LUT_outputImage);


Let's have the new code in the same area as the current code, and have an if statement to 

## Video modes - generation of bitstream.

### Text mode

Only for updated lines:
Read and double values from the font. Based on scanline % 8, grab that scanline of data from the pre-computed font and emit each byte twice to double it.

### LGR mode

only for updated lines:
For each scanline, read the byte. scanline / 8 to find the line, then convert line to memory location.
If scanline % 8 between 0-3, emit the lo nibble. If scanline % 8 between 4-7, emit the hi nibble. (or whichever it is).

as long as we're on an X location in this dot, emit output based on the pixel X location mod 4 as an index into the bits of the lo-res pattern. Sometimes we start halfway through a pattern; sometimes we end halfway through a pattern. This is what generates artifacts between different colored lores blocks.

### HGR mode

only for updated lines:
Already know how to do that.

### DHGR mode

only for updated lines:
Similar to HGR mode, except pulling values first from alt bank, then main, back and forth across the 40 bytes. And, disregards the hi bit of each byte.
Each byte starts at a 180 degree phase offset from each other, and, the whole scanline starts at 90 degree phase difference from normal hgr.


## Fancy timing-based updates

The video frame can now be generated, if we like, down to the cycle level. We can track where the "beam" is based on cycle counts. This is just some simple math. So if display mode is changed in the middle of a scanline, we could in theory start rendering the bitstream based on the new mode from that point on. This is how some of the other emulators do it (OE, AppleWin).
I still am not sure how important this is to me. But, we're much closer to being able to do it.


# New RGB Approach

took a look at the hgrd code - I remember that I manually brought in a filter coefficient array instead of calculating it. If I want to try higher-bandwidth devices, I can edit a template in openemulator and try them live! ok that's cool. Once I settle on parameters in OE I can calculate a new coefficient array for 'RGB'. And that would be easier than trying to write more special code for 'rgb mode'.

these params are pretty good for composite, about the best we get. if chrome bandwidth is too high, the color changes so rapidly you get effectively dithering.

```
        <property name="videoBandwidth" value="7000000"/>
        <property name="videoLumaBandwidth" value="2000000"/>
        <property name="videoChromaBandwidth" value="2250000"/>
```

the higher the videoLumaBandwidth, the more you get the dot pattern colored as opposed to smeared. i.e., you see distinct black dots in the middle of what should be color fields. text in gr mode at 6M / 2.25M is colored, but very easy to read. 2.5M for each isn't bad either. slight dithering effect but good text resolution.

if luma too high, you get black. in middle range, you get for example yellow and darker yellow where there's a 0 bit. The black dots (high luma) might be how the IIe RGB worked. (SOMEWHERE in my memory i remember a crisp display with striations. it's not the GS RGB.)

SO. no, that doesn't work. RGB is in nature different from NTSC. it does not split luma from chroma in the same way, and it does not combine colors the same way. SO, that implies the IIgs video display chip is doing some very special things.

This is interesting..
https://comp.sys.apple2.narkive.com/r0nqAIM3/mega-ii-chip-information
https://tek4um.com/Apple%20Cork%20Cupertino%20Arhive/photos/2gsDesingDoc1985/


the GS might be using a ROM lookup table. "VGC takes standard apple II video information from the Mega II and generates the video output".
"adds enhancements to existing apple ii video modes"
There's a bit in the manual that discusses how to view the dhgr color generation, as a sliding window 4 pixels wide. That might be the best way to think about it - the color of a pixel is determined by a lookup of those 4 values. That would be a 16-color lookup table. OH. the Mega II diagram says: 16x12. 16 entries, 12 bits - 4096 colors. I think that's what Mega II / VGC is doing. This would also make HGR and DHGR consistent with each other. How about LGR? it is clearly using solid colors. So the GS is picking the color based on the LGR nibble value, and keeping that one throughout the LGR block. Same for DLGR.

So, it's a lookup table you want, is it?

AppleWin does this:

```
	uint32_t mask  =  0x01C0; //  00|000001 1|1000000
	uint32_t chck1 =  0x0140; //  00|000001 0|1000000
	uint32_t chck2 =  0x0080; //  00|000000 1|0000000

	// HIRES render in RGB works on a pixel-basis (1-bit data in framebuffer)
	// The pixel can be 'color', if it makes a 101 or 010 pattern with the two neighbour bits
	// In all other cases, it's black if 0 and white if 1
	// The value of 'color' is defined on a 2-bits basis
```

Call it a 3-bit sliding window of the hgr data. So what would this look like using the dhgr data? a 6-bit sliding window (or, in dhgr, 8-bit?)

let's assume 6.

1100110011001100

so 110011 is a color
001100 is a color (phase shifted 90 or 180 degrees in the LUT)
111111 is white
000000 is black

this is dhgr table from:
https://www.appleoldies.ca/graphics/dhgr/dhgrtechnote.txt

```
                                                Repeated
                                                Binary
          Color         aux1  main1 aux2  main2 Pattern
          Black          00    00    00    00    0000
          Magenta        08    11    22    44    0001
          Brown          44    08    11    22    0010
          Orange         4C    19    33    66    0011
          Dark Green     22    44    08    11    0100
          Grey1          2A    55    2A    55    0101
          Green          66    4C    19    33    0110
          Yellow         6E    5D    3B    77    0111
          Dark Blue      11    22    44    08    1000
          Violet         19    33    66    4C    1001
          Grey2          55    2A    55    2A    1010
          Pink           5D    3B    77    6E    1011
          Medium Blue    33    66    4C    19    1100
          Light Blue     3B    77    6E    5D    1101
          Aqua           77    6E    5D    3B    1110
          White          7F    7F    7F    7F    1111
```

* violet = 1100
* Green = 0011
* Medium Blue = 0110 - 1 dot shift
* orange = 1001 - 1 dot shift
* white = 1111
* black = 0000

So extracted - the hires colors map into this, blue/orange phase-shifted from the violet/green.

This approach should still pick up the artifacts. It's not dissimilar to the ntsc code in that regard. hires will always generate only one of those six patterns (except on color and byte edge crossings).

The IIGS LGR is clearly generated more like the way I did it originally. (i.e. we have a color, paint the block that color - i.e., no artifacts, except, there is just a tiny hair of phosphor matrix coloring).

On the IIGS RGB, there is -no NTSC fringing-. It DOES do the dots / half pixel for the hi bit delay.

```
   11 00 11 00 - repeated violet

00 11 00         - color - ror 2, violet
   11 00 11      - color - violet
      00 11 00   - color. ror 2 violet.
         11 00 00 - black.
```
00 11 11 - black, white, white (double of course)
11 11 00 - white, white, black (double up of course)

this is hires bits, not 560 dots:

10 - gives us a single dot of purple
1010 - gives us three dots of purple. i.e., it fills the 0 with the same purple color
111 - three dots of white

1001 - should be a dot of purple and a dot of green.

So on any pixel - we look ahead 2 pixels. if there is any kind of repeating pattern, 010 or 101, we color this dot and the next dot. (a "wide" pixel)
if we have a non-repeating pattern, 100, we color only this dot. (a "narrow" pixel)

When we draw white line at right edge (279), we do get this value in memory: C0
that is, shifted, and last bit set. last bit set is not a double 1. so we don't get white, we get brown. (hi res artifact!)
(unshifted would get us what..) unshifted gets us green. 0x40. 1 in odd column, so yeah, green.
So the color lookup is based on the 560 dots value. but we're jumping by two dots.
So on GS RGB there ARE some artifacts but they're not because of ntsc. they're because of the way bit streams are calculomated.

how about look behind one, look ahead one
```
   11 11 11 00 00 - white

00 11 11          - white
   11 11 11       - white  - white. ror 2 it's still white.
      11 11 00    - white. 
         11 00 00 - the pixels are off, so NO COLOR.
```

I think I'm going to bag the above, and just patch the current 'rgb' code. Which isn't awful.
NO I AM NOT A QUITTER! HA HA.

I did fix a bug where when writing black I was writing with alpha=0, which is wrong. Fixed to write 0xFF alpha.

So, I think the bug was actually: you have to clear the buffer before you update the texture. that old thing again. So now I clear the entire scanline to 0x000000FF before doing the other updates. Much happier. let's try Choplifter, to "test".

Let's think about modes again.

there is:
   NTSC (color) mode
   RGB (color) mode
   Mono mode (based on NTSC engine)
      with green, amber, white color choices.
   There should not be a mono+rgb mode. Doesn't make sense.
   These should be vectored in display.cpp, not anywhere else.
   All options: user can select LINEAR or NEAREST.
   Another option: scanline option (to show distinct scanlines).

F2 becomes: toggle between display engines - NTSC, RGB, Mono.

Have:

render_line_ntsc
render_line_rgb
render_line_mono


# Optimizing the LUT-creation code.

Starting point:

LUT init time: 425731833 nanoseconds (425 milliseconds).

the phase is only ever one of four values. it's 0, 0.25, 0.5, 0.75.
So precalculate these ..
LUT init time: 402014750 nanoseconds

pre-calculate them once and put them into config. SCORE.

width being pased in to processAppleIIScanline_init is 48.
He's 'painting' the 17 bits into positions 8 .. 24. plus 3, or 27. So use 28 as width instead of 48?
I don't think we need to loop in here..

put the bit pattern into frameData 0 .. 16
run processAppleIIScanline only on pixel position 8.

we DO need to convert 17 pixels to YIQ
but we DON'T need to run the filter on anything except the center pixel. That, mind-blowingly, gets us to:

LUT init time: 27367666 nanoseconds

a 15x speedup. ok that is still not fast enough yet to change this at runtime. It is, however, fast enough to pre-compute a couple different ones at startup time.

Next, we are calling processPixel 9M times, doing two float multiplies, and a divide of luminance by 255. The thing is, luminance is only ever 0 or 255. So luminance is only ever 0 or 1. So the values are either:
  luminance = 255: output = 1.0f, config.phase_sin, config.phase_cos
  luminance = 0  : output = 0.0f, 0.0f, 0.0f

ok, not getting it below 27ms so far, but simplifying and cleaning up the code..

so with the phases.. wouldn't phases 1-3 just be the same data as phase 0, only indexed differently? if that's true, then perhaps we could greatly reduce the memory footprint and execution time by 4x. That would help with cache etc too. cuz it's a half million x 4 = 2MB of data.

while there are some lines in here that repeat the entire line compared to other entries, there is no clear-cut repetition.

I bet we may need to calculate the filter for N taps if that's how many taps we want. As opposed to 8. We're calculating for 8 but only using 5 or 6 or whatever. I bet that's the issue.



so with the phases.. wouldn't phases 1-3 just be the same data as phase 0, only indexed differently? if that's true, then perhaps we could greatly reduce the memory footprint and execution time by 4x. That would help with cache etc too. cuz it's a half million x 4 = 2MB of data.

while there are some lines in here that repeat the entire line compared to other entries, there is no clear-cut repetition.

I bet we may need to calculate the filter for N taps if that's how many taps we want. As opposed to 8. We're calculating for 8 but only using 5 or 6 or whatever. I bet that's the issue.


8 / 50 = looks great
7 / 45 = pretty good
6 / 35 = pretty good
5 / 25 = some artifacts


Light wavelengths in nm of different CRT phosphors:

https://www.labguysworld.com/crt_phosphor_research.pdf

I think it's going to be : the actual phosphor frequency, but, some average brightness based on peak and persistence. "time to decay to 10% of peak".
For instance P1 green says 20ms to decay to 10% of peak. So a frame is 16ms, so the average brightness will be what, 50%, assuming a linear dropoff? There are online wavelength to RGB converter tools.

Measurement app says 549nm dominant wavelength on the monitor ///. The app is probably not calibrated well. and the ambient light might be affecting my readings. but it was reading 549/550 on both. So maybe that's the way to do it.

openemulator measures at 495nm. The Apple monochrome monitor //e uses EIA Type P31 phosphor; the "monochrome monitor" uses EIA P4. P4 peak is 540 - HOWEVER "range" is 330-699", i.e., white. Probably not evenly. P1 says peak 525 range 490-580 (fairly narrow). P31 is 531nm.

So the dominant is not the only important thing, but, is just the peak.

OK, I have continued refactoring the code here, I have it from 28ms down to 16ms by eliminating unnecessary arrays, flattening applyFilter and yiqtorgb into processappleIIScanline_lut. Combined two loops into one that were operating on the same indexes. Made the processing of the filter linear instead of center, then outside + and - etc repeating until 8. The next chunk - the code is using an integer to iterate through all 128k possible bit combinations; it is then generating an array of 17 bytes to represent these. Then it calls processScanline, which parses those bytes. 
inputScanline[x] is being accessed in order. So instead of generating and reading bytes, let's just pass in the int and process the bits right in processAppleIIscanline_lut. That shaved another 2ms. 

## Further Discussion

I see this referenced here:

https://groups.google.com/g/comp.sys.apple2/c/X_LKB6t6njE
https://observablehq.com/@zellyn/apple-ii-ntsc-emulation-openemulator-explainer
https://zellyn.com/apple2shader/ - the webgl port of openemulator ntsc code. 
https://github.com/zellyn/apple2shader
