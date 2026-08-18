# C800-CFFF Memory Mapping

My understanding is: if any CnXX is referenced, it enables the slot-card ROM for $C8 - $CF.
If you write to $CFFF, it disables that $C8 - $CF map.

```
This is generalized as certain slots have certain restrictions.
Each slot in the apple //e has the exclusive use of 256 bytes of rom
address, 8 bytes of ram and 16 I/O lines. Cards that wish to use the 2K
expansion rom space $C800 - $CFFF must follow the rules. They must include
circuitry such that a when the address lines for $CFFF becomes active they
unlatch their expansion rom from the address space. Most cards use a simple
nand gate arrangement so when $CFxx is active they unlatch and thus they
loose the last 255 bytes of expansion rom. When a card whishes to use its
expansion rom it does a read or write to $CFFF. This causes any card using
expansion rom to unlatch from the address space (including its own). When
the card next address its own slot its expansion rom will be available from
$C800-$CFFE. Or with the simple nand gate from $C800- $CEFF.
```

So, this memory mapping is driven by each card. In an emulator context,
we could either implement each card to always be in a chain of calls that are
called in sequence, or, we assume their behavior and centralize the handling.

What would it mean to "unlatch"? E.g. on a //e does it have ROM built-in
that always lives at $C800 and that is sometimes overridden?

I think it's probably okay to have the bus.cpp logic handle this and restore that to
empty (but allocated) RAM when accessed.

The question would be, do some cards write $CF00 to disable cuz they know that works
for them, but that wouldn't work if the emulator was checking only $CFFF. I guess there's
only one way to find out.

Some more observations:

```
The proper use of $C8xx..$CFFF space is slightly different than has
been suggested. Any ROM space which a card may have is enabled
(on the card itself) by an access to the $Csxx slot space, but then
code in the slot space must _disable_ the ROM space by making an
access to $CFFF, which will switch off _all_ cards' expansion ROM
(again, using the cards' own logic). Fetching of the next instruction
in sequence from the slot space will then re-enable "this" card's
expansion ROM, with the guarantee that it is the only card whose
ROM is enabled--assuming that all the other cards play by the
rules.
```
* https://comp.sys.apple2.narkive.com/FgF7Fw4Y/what-apple-ii-cards-used-the-c800-rom-address-space

OK so what that is saying is that every single access to CsXX space will
do this memory-map control. Which we get. We don't want to have to reset the
memory mapping for every single access. We can implement it that way but then
also implement a cache where we keep track of the slot number whose ROM is
currently enabled, so we don't have to constantly set the memory map.
