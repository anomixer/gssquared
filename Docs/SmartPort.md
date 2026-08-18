### SmartPort / ProDOS Block-Device Interface

SmartPort became a standard for: 5.25" drives, 3.5" drives, and some hard disk drives,
for the ProDOS operating system.

It is documented in the Apple IIgs Technical Reference Manual, chapter 7.

$Cn01 - $20
$Cn03 - $00
$Cn05 - $03
$Cn07 - $00 - "SmartPort Signature Byte"
$CnFB - ?? - SmartPort ID Type Byte

bit 7: extended
Bits 6-2: Reserved
Bit 1: SCSI
Bit 0: RAM Card

The ProDOS Dispatch address is at $CnFF. That byte indicates the address of the
dispatch routine. if VV = value at $CnFF, then the ProDOS dispatch routine is at $CnVV.
The SmartPort dispatch address is 3 bytes past that. 

Both the Apple 3.5 Disk and the UniDisk 3.5 have the ability to override various
internal routines, such as scanning for sector headers, etc. This was done to enable
copy protection. We don't need to implement this. We will implement a generic UniDisk 3.5
interface to simulate an 800K floppy. Attempts to call SetHook will just return an error.

Our primary goal here is to support a simple block device interface, the fastest way to get
disk storage on here so we can boot ProDOS < 2.0. on my apple II+.

Some info is at:
https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Companies/Apple/Documentation/Apple%20II%20Technical%20Notes%201989-09.pdf

(Apple II Technical Notes)

We would want to support this using ParaVirtualization. We will 'notice' and capture any JSR DISPATCH
instructions. Then instead of performing the normal instruction emulation, we will instead call
our paravirtual routines.
