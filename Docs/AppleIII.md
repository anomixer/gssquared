# Apple III

https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Software/Operating%20Systems/Apple%20SOS/Manuals/Apple%20III%20SOS%20Reference%20Manual.pdf


For reference:
https://www.apple3.org/Documents/Magazines/AppleIIIExtendedAddressing.html


For later reference: Apple III hardware description:
https://groups.google.com/g/comp.sys.apple2/c/_NYADLx16G8/m/MZKv-Y20uTQJ
https://ftp.apple.asimov.net/documentation/apple3/A3SOSReferenceVol.1.pdf

Everything seems straightforward except:
"Extended/enhanced indirect addressing"
https://www.apple3.org/Documents/Magazines/AppleIIIExtendedAddressing.html
also discussed in detail in 2.4.2.2 in the A3 SOS Reference above.

Huge amount of Apple /// info here: https://ftp.apple.asimov.net/documentation/apple3/

There are great disassemblies of Apple III SOS and probably ROM. These can be used to 
answer questions about the hardware.

Apple III disk image formats:
https://retrocomputing.stackexchange.com/questions/12684/what-are-the-most-common-apple-ii-disk-image-formats-and-what-hardware-disk-driv

A whole bunch of Apple III software:
https://www.apple3.org/iiisoftware.html

This patent covers the Apple III memory management:
https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20III/Apple%20III/Patents/US%20Patent%204383296%20-%20Computer%20with%20a%20memory%20system%20for%20remapping%20a%20memory%20having%20two%20memory%20output%20buses%20for%20high%20resolution%20display%20with%20scrolling%20of%20the%20displayed%20characteristics.pdf
It is largely incomprehensible gobbledygook.

This also discusses it:
https://mirrors.apple2.org.za/ftp.apple.asimov.net/documentation/apple3/A3SOSDeviceDriverWritersGuide_alt.pdf

This refers to a "Z register". What the heck is that? "Zero Page register" $FFD0. 

Here's a good overview of some of the hardware, especially display, character generator, character sets, etc.

https://www.apple3.org/Documents/Magazines/apple3_Info_IIIBitsJeppson.pdf

Great Youtube Video:
https://www.youtube.com/watch?v=H0CCTFjooY0


## Apple III MMU

There are these following keys to Apple III memory management:

* bank switching
* enhanced indirect addressing
* Zero Page and Stack Relocation
* $C000 - $CFFF I/O space

### Bank Switching

The address space from $2000 to $9FFF (32K) is bank-switched, under control of a register at $FFEF - the bank register. This is called "Current-Bank Address".

The rest of memory ($0000 - $1FFF, $A000 - $FFFF) is referred to as S-Bank addresses.

### Enhanced indirect addressing

```
Enhanced indirect addressing merely adds one step to this process. To
perform an enhanced indirect addressing operation, in the interpreter
environment, on location $xx:hilo, you store $10 in $nn, $hi in $nn+1,
and $xx in location $16nn+1. Then perform the operation in an indirect
mode on location $nn. The location $16nn+1 is the extension byte, or
X-byte , of the pointer. 

Enhanced indirect addressing takes effect whenever you execute an
indirect-mode instruction and bit 7 of the pointer's extension byte (X-byte)
is 1: that is, whenever the extension byte is between $80 and $8F. If you
wish to perform normal indirect operations, using bank-switched
addressing rather than enhanced indirect addressing, you should store
your pointer in bank-switched form in the zero page, and set its extension
byte to $00, which will make sure bit 7 is 0
```

ok, this is WEIRD. but it must work like this: the MMU watches for two consecutive Zero Page accesses (nn, nn+1) followed by .. ?

Yes, that's exactly what it is:
https://claude.ai/chat/7a491889-5f2e-4561-aab0-0e0875efe7c9

At least, Claude thinks so, lol.

```
Whenever the 6502 uses a (ZP),Y addressing mode, and the zero page register ($FFD0) contains a number from $18 to $1F, the "switch box" uses the above-mentioned extra byte (called the "Xbyte") to perform "extended addressing." If the Xbyte (fetched from page ($FFD0) XOR $0C) is outside the range $80...$8F, then the indirect memory access occurs normally. Otherwise the switch box intervenes again and briefly remaps memory just long enough enough for the indirect memory access. If the low nibble of the Xbyte is n, then the indirect addressing sees a memory map composed of bank n (that's the same bank n from the normal bank-switching method mentioned above) mapped into $0000...$7FFF, and bank n+1 mapped into $8000...$FFFF.

....Well, almost. If the Xbyte is $8F, something special happens. The indirect addressing sees the normal Apple III memory map with bank 0 switched into $2000...$9FFF. As an added bonus, it also sees the RAM that's hiding under the VIA control registers at $FFD0...$FFEF. In fact, this is apparently the ONLY way to get at those 32 bytes of RAM. Since this memory is unavailable by other means, the operating system uses it to store the last valid value of the system clock.
```

So, short version is the Apple III MMU can detect this. It would be zero page nn, cycle y; then nn+1, cycle y+1; that sets a state where the next memory access will be modified by the temporary bank number (which we can cheat and read directly from the Zero Page Bank EOR $0C:nn).

So it works with indirect X or Y. OPC ($LL,X) or OPC ($LL),Y - either of these will hit the ZP twice in succession at incrementing addresses. Then set "enhanced" flag in MMU. Then on next read or write, take the X-byte into account.

### Memory Management Registers - "Those Registers"

https://www.apple3.org/Documents/Magazines/apple3_Info_IIIBitsJeppson.pdf

Two 6522 VIA chips are mapped to:
FFD0 - FFDF, and FFE0 - FFEF. There is surely a bit of ROM in the last 16 bytes?

D-VIA

$FFD0 | Zero Page register
$FFDD | | "Any-slot' interrupt flag. read/write. 
$FFDF | IORA | Environment register.  

FFDF Bits

bit | function | Bit = 0 | Bit = 1 |
0 | F000.FFFF | RAM | ROM
1 | ROM #     | ROM #2 | ROM #1
2 | stack | alternate | normal (true 0100)
3 | C000.FFFF | read/write | read-only
4 | reset key | disabled | enabled 
5 | video | disabled | enabled
6 | C000.CFFF | RAM | I/O
7 | clockspeed | 2MHz | 1MHz


E-VIA

$FFEC | E-VIA Peripheral Control Register
$FFEE | E-VIA Interrupt Enable Register
$FFEF | IORA | Bank Register

We've emulated this chip on the Mockingboard. In particular, we are likely to need a lot of this functionality, the interrupt-driven counters, etc. Perhaps we should look at pulling the 6522 out into its own class. Worst case we can just copy the code over.

## Interrupts

The Apple III firmware / SOS make extensive use of interrupts. There are interrupts from the VIAs; there seem to be from the Keyboard; 

## Slots

There are only 4 expansion slots, numbered 1 to 4. So slot-card ROM space is only C100 - C4FF; C500 - C7FF is always RAM.

Lookie, there is all kinds of crazy memory mapping stuff by page here!

There is an ACIA serial port whose registers live in $C0F0-$C0FF. There is a virtual slot 6 with disk registers in $C0E0 - $C0EF. But no old timey boot firmware there.

