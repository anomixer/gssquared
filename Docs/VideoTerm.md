# Videx Videoterm Emulation

I have the Videx Videoterm manual, and it's quite thorough as to how it works.

## General Notes

Usually in slot 3

has on-board RAM addressed from $000 to $7FF

Uses memory locations in the main video page to help control the display:

| Register | Address |
|----------|---------|
| Screen base address (lo) | $478 + n |
| Screen base address (hi) | $4F8 + n |
| Cursor horizontal position | $578 + n |
| Cursor vertical position | $5F8 + n |
| Pascal character write location | $678 + n |
| First line on screen | $6F8 + n |
| Power off / leadin indicator | $778 + n |
| Video setup flags | $7F8 + n |

The horizontal position is 0-79.
Vertical is 0-23.
Pascal write character location is where Apple Pascal looks to find the next character to send
to terminal or other peripheral.
First line on screen pointer is used in text scrolling.

Video setup flags:

Bit 0: Alternate character set. 0 = standard, 1 = alternate. 
If you have modified VT for "inverse video option", then 0 is standard video and 1
is inverse video.
Bit 4: number of rows. =0 , 18 lines of text. = 1, 24 lines of text.
Bit 6: upper/lower case. 0 "non-conversion of entered text". 1 means will convert an uppercase character typed on keyboard to lowercase. (apple II didn't have u/l case)
Bit 7: GETLN flag. 0 = input came from a GET statement. 1 = input came from a GETLN routine.


Registers and values:

| Register | Description |  Read | Write | 9x9 | 9x12 |
|----------|-------------|-------|-------|-----|------|
| R0 | Horizontal Total | No | Yes | $7B | $7B |
| R1 | Horizontal Displayed | No | Yes | $50 | $50 |
| R2 | Horizontal Sync Position | No | Yes | $62 | $62 |
| R3 | Horizontal Sync Width | No | Yes | $29 | $29 |
| R4 | Vertical Total | No | Yes | $1B | $14 |
| R5 | Vertical Adjust | No | Yes | $08 | $08 |
| R6 | Vertical Displayed | No | Yes | $18 | $12 |
| R7 | Vertical Sync Position | No | Yes | $19 | $13 |
| R8 | Interlace Mode | No | Yes | $00 | $00 |
| R9 | Max Scan Line Address | No | Yes | $08 | $0B |
| R10 | Cursor Start | No | Yes | $C0 | $C0 |
| R11 | Cursor End | No | Yes | $08 | $0B |
| R12 | Start Address (Lo) | No | Yes | $00 | $00 |
| R13 | Start Address (Hi) | No | Yes | $00 | $00 |
| R14 | Cursor Hi | Yes | No | $00 | $00 |
| R15 | Cursor Lo | Yes | No | $00 | $00 |
| R16 | Light Pen High | Yes | No
| R17 | Light Pen Low | Yes | No

* $C080 + x = Register number
* $C081 + x = Register value

### R0 - R3
These control the horizontal sync timing.
* R0 - total horizontal positions?
* R1 - number of characters per line. e.g. $50 = 80.
* R2 - horz sync position. This and R3 determine the horizontal sync width.
* R4 - vertical refresh rate, in conjunction with R5. Vary only to provide 50hz/60hz operation.
* R6 - number of displayed character rows. 9x9 chars = 24 rows. 9x12 = 18 rows.
* R7 - vertical sync with respect to top reference line.
* R8 - raster scan mode.

| bit 0 | bit 1 | mode |
|-------|-------|------|
| 0     | x     | Normal Sync Mode |
| 1     | 0     | Interlace Sync Mode |
| 1     | 1     | Interlace Sync with Video |

Normal sync mode - 60hz. 
Interlace Sync Mode - 60Hz but interlaced, doubles number of dots, duplicating each dot below its position increasing quality of displayed character.
Interlace Syncw ith Video keeps the character dot matrix the same but doubles lines on the screen so twice as many characters, each one half their normal size, are displayed on the screen.
This mode should only be chosen if you are using a long-phosphor video monitor.

* R9 - number of scan lines per character row minus one.
* R10 - R11 - cursor start and end. Start and end scanlines (on a row) of inversion to indicate the cursor position.

| Reg | Bit 6 | Bit 5 | Bits 4-0 |
|-----|-------|-------|----------|
| R10 | 1  | 0 | Cursor Blink - 1/16th Field rate blink (about 1/4 second)|
| R10 | 1  | 1 | Cursor Blink - 1/32nd Field rate blink (about 1/2 second) |
| R10 | 0  | 0 | Cursor Displayed - No Blink |
| R10 | 0  | 1 | Cursor Not Displayed |

* R12 - R13 - high and lo byte of where to start writing on screen. Not for use by
users.
* R14 - R15 - R14 6 bits, R15 8 bit, position of where we're writing.
* R16 - R17 - light pen low and high on strobe.

## Memory Map

C800 - CFFF

C800 - CBFF - 1K ROM
CC00 - CDFF - 512 byte window into on board 2K RAM
CE00 - CFFF - not used

CC00 to CDFF is paged. "once the videoterm is activated by the correct memory reference,
it will automatically set the correct active page".
You then write the character you want displayed to the relative address within the 512-byte
group.

Read on:
$C080 + x : page 0
$C084 + x : page 1
$C088 + x : page 2
$C08C + x : page 3

* Complete ROM set: https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/videx/

There's the ROM ROM and various character ROMs.

There is this cool thing:
Virtual 6502 disassembler: https://www.masswerk.at/6502/disassembler.html

the ROM is 1K, maps into $C800 - CBFF. CB00 feels like what gets mapped into 
$CS00 - $CSFF (e.g, $C300 - $C3FF, the stuff that would handle pr#3.

So this thing scrolls rapidly by changing the start position of the memory scanner. change the pointer in two registers, clear only one line (or two lines or whatever) instead of copying every line on the screen.

Assume our virtual card has the "Soft Video Switch" installed. 
* C058 - annunciator 0 off, normal apple ii video
* C059 - annunciator 0 on, enable the 80-column mode IF "color killer is enabled" (i.e., we're in all text mode). If graphics mode, we switch to graphics mode.

In short, this screen display logic will replace the normal display if and only if C051 is active. We can still do the normal Render Line stuff. and call the different line render if C051 & C059.

We need a pixel buffer that is bigger than normal. It needs to be 640x216. We'll allocate that texture in the Videx module initialization, and allocate a.

Model using the memory expansion card.

If user changes registers, especially the start screen, dirty all the display lines.

## Character ROMs

There are quite a few video character ROM chips that were available. Foreign languages; Katakana; normal and inverse; super and subscript; symbols, APL, and Epson.

The card had an extra ROM socket for the "alternate character set" (selected per above).

All Videx characater rom files are 2KB. each matrix in the rom is 8 pixels by 16 pixels. 

so the Videx rom "ultra.bin" contains the regular char set in the 1st half, and 7x12 character set in the 2nd half of the ROM. (2K per half). The characters are actually 8 x 12. The first column of each character is usually blank, to provide spacing between chars, unless it's a graphics character.

So, 80x18 screen this way is: 640 x 216 pixels. 24 x 9 (normal) is also 640 x 216 pixels. The ultraterm has a 132 column mode, but the VideoTerm does not. honestly, that might be really cool later on..

The VIDEOTERM has  2K of onboard RAM, paged into the 512 byte window from $CC00 to $CDFF. See the softswitches for controlling paging above.

Character set is selected by the high bit of a character in video memory. Cleared is Main character set; Set is Alternate character set.

## Soft Video Switch

if "color killer is off" (i.e., we're in a graphics mode).
if the color killer is on (text mode), then the Soft Video Switch follows the state
of Annunciator 0 at $C058/$C059. If Anc is off, the video switch displays 40 columns.
If the Anc is on, 80 columns is displayed.

## Programming the Videx Videoterm

https://glasstty.com/using-the-videx-videoterm-with-assembly-language/

## Software Support

Apple Writer II - with pre-boot disk
Magic Wand
Executive Secretary
Zardax
Easy Writer Professional
Apple PIE 2.0
Letter Perfect
Super-Text II
WordStar (cp/m ?)

