# Apple Memory Expansion Card

This was a card for II, II+, IIe.
Probably works in GS too, but, you wouldn't want to use it since the GS had its own
large memory space and RAMdisk built in.

This should be a simple implementation. 


## Registers

| Location | Description |
|----------|-------------|
| $C080 | Low byte of RAM address |
| $C081 | middle byte of RAM Address |
| $C082 | High byte of RAM Address |
| $C083 | Data at addressed locaton |
| $C08F | Firmware Bank Select |

On power-up, the registers are disabled. They are activated by any access to
$CS00 - $CSFF. (the slot memory page).

The address bytes are r/w.

If the card has 1MB memory or less, the hi byte will always be in a range
$F0 - $FF.  The top nybble is always F.

If the card has more than 1MB memory, then the hi nybble is meaningful.

When you read or write the data byte, the address automatically increments.
Whenever the lower or middle address byte changes from a value with 
bit 7 = 1 to one with bit 7 = 0, the next higher byte is automatically
incremented. Always load bytes in order low-middle-high, and always load
all 3 of them.

* ROM image: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Interface%20Cards/Memory/Apple%20II%20Memory%20Expansion%20Card/ROM%20Images/Apple%20II%20Memory%20Expansion%20ROM%20-%20341-0344A.bin

The ROM appears to be mapped as follows:
The first bit of the ROM (C800-C8FF) is text from the guy who copied the ROM.

| Offset in File | mapped address |
|----------------|----------------|
| $0000 | Text message |
| $0100 |  slot 1 $C100 |
| $0200 |  slot 1 $C200 |
| $0300 |  slot 1 $C300 |
| $0400 |  slot 1 $C400 |
| $0500 |  slot 1 $C500 |
| $0600 |  slot 1 $C600 |
| $0700 |  slot 1 $C700 |
| $0800 to $0FFF |  $C800 to $CFFF |

The code is the same in each of the first 8 pages, except there are differences for each slot number.


## Similar / clone cards

The Applied Engineer RAMFactor works the same way, just has different ROM.

* ROM - https://ae.applearchives.com/all_apple_iis/ramfactor/

The register interface is the same.

## This will require us to understand and implement the slot-card ROM mapping.

See C800-CFFF.md

