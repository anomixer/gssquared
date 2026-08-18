# Hi-Res Version 3

The current hires implementation (v2) is a little messy, with weird handling of filling between pixels, etc.

Doing a lot of reading and AI research / testing on this.

The color is determined by the phase of the data signal in comparison to the color reference signal.

The color reference signal is 3.5M.

Here's a 3.5 signal:

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

if the bit clock is in phase with color reference, and we have a 3.5M signal (i.e it's actually oscillating), then we have green.

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
          ____      ____      ____      ____      ____      ____      ____      ____      
     ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

If the phase is off by 1/2 cycle (180 degrees) then we have violet / purple.

Note, in a real apple II these values are offset from the "Text-hires clock" by not quite 90' extra or so, because of delays in the hardware and the way the Color Ref signal is generated. But this is how they get the colors they do.

If the hi bit is set in a data byte, then we are offset by another delay of a 14M (1/4, or 90 degrees).

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
       ____      ____      ____      ____      ____      ____      ____      ____      
  ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    


Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
            ____      ____      ____      ____      ____      ____      ____      ____      
       ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

These waveforms generate blue and orange.

At any given pixel location on the screen (assume 560 pixels), we read the phase at that point in time and select color accordingly.

Lo-res colors and double-hires and various other artifacts are created by different kinds of waveforms: for example, non-square waves (duty cycles other than 50%).

Lo-res colors "dark" have 25% duty cycle. "Light" have 75% duty cycle. The duty cycle affects the luminance. Is that how we get white ("100%") and black (0%) luminance?

In a normal NTSC TV, the input waveform is not a square wave at all. it's a complex composite (hence the name) of signals, none of which are square. There are:

* the colorburst (only present part of the time during the horizontal blanking interval
* I and Q signals - color difference signals
* luminance

Luminance can be thought of as a DC offset from the black level. 

I and Q are combined in one signal at around 3.58MHz, and the phase difference between this and the colorburst is what determines color.

To extract Luminance, I and Q from a mixed signal - whether a TV signal, or the digital 0/1 output the Apple II creates - we use several low-pass filters, each tuned to certain frequency and "bandwidth". For example, luminance from original black and white TVs is on the order of 2.5 - 2.75MHz of bandwidth in the the 6MHz TV channel. I and Q have around 1.3MHz and 0.6MHz bandwidths respectively. Then in a TV signal there's audio, which we don't have here.

So we first extract luminance with a low-pass filter. And then I and Q with a band-pass (as they center around 3.5MHz), multiply each by a sin or cos function of the colorburst, then low-pass the results of each.

Astonishingly, this extracts useful color information from what the apple II just emits as a simple binary digital output stream.

The stream is clocked at 14.31818MHz, and the colorburst is at 3.579545MHz. The Apple II text, lo-res, and standard hi-res display modes have 280 pixels but a total of *560 pixel positions*. One pixel is normally 2 14M cycles. In hires mode though, if the hi bit of the data byte is set, then pixels in that byte are delayed by one 14M cycle. So some pixels start on an even column and are 2 dots wide, and some pixels start on an odd column and are 2 dots wide.

The combination and collision and specific timing of these different bit patterns is what creates the various colors.