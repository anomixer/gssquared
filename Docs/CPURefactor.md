# CPU Refactor

### consider refactoring cpu execute_next.

Now, I've got this function pointer for execute_next. let's contemplate..

cpu_state
  cpu_6502_state
  cpu_65c02_state
  cpu_65816_state

ok! So I got the inc abs,x stuff refactored. here it is. A few innovations here. First, I defined a union struct to make dealing with address calculations like this way easier - instead of doing tons of manual masking and bit-shifting I can just refer to lo and hi portions of an address or the whole thing. This makes coding the pieces that are modifying these bits much easier, and probably more efficient TBH.

Second is just formatting the code in a nice way to show the cycle breakdowns more clearly inside the function.

```
struct addr_t {
    union {
        struct {
            uint8_t al;
            uint8_t ah;
        };
        uint16_t a;
    };
};
        case OP_INC_ABS_X: /* INC Absolute, X */
            {
                inc_operand_absolute_x(cpu);

inline absaddr_t get_operand_address_absolute_x_rmw(cpu_state *cpu) {
    addr_t ba;
    ba.al = cpu->read_byte_from_pc();      // T1

    ba.ah = cpu->read_byte_from_pc();      // T2

    addr_t ad;
    ad.al = (ba.al + cpu->x_lo);
    ad.ah = (ba.ah);
    cpu->read_byte( ad.a );                // T3 

    absaddr_t taddr = ba.a + cpu->x_lo;    // first part of T4, caller has to do the data fetch

    TRACE(cpu->trace_entry.operand = ba.a; cpu->trace_entry.eaddr = taddr; )
    return taddr;
}

inline void inc_operand(cpu_state *cpu, absaddr_t addr) {
    byte_t N = cpu->read_byte(addr); // in abs,x this is completion of T4
    
    // T5 write original value
    cpu->write_byte(addr,N); 
    N++;
    
    // T6 write new value
    set_n_z_flags(cpu, N);
    cpu->write_byte(addr, N);

    TRACE(cpu->trace_entry.data = N;)
}
```

A next step is, I think, to try redefining these as macros instead of as inline functions. That will result in much faster code when compiling non-optimized. It may also eliminate the question of whether the compiler inlined a function or not. More importantly, I think it will avoid all the errors I get when editing the cpu.cpp code.

Another revelation is that the 65c02 implementation will be very different because of this - some of its instructions are different cycle counts, and it does fewer of these false reads. And then the 65816 puts many of them back in! holy toledo.

And also I had been doing a lot of thinking about the '816, its 16 bit registers, the D and B handling, etc. are all going to make it very difficult to just reuse this code.

And this implies a different code re-use strategy. In fact it may not be possible to reuse large chunks of the switch statement because of this as I'd originally envisioned. And, the "undocumented opcodes" probably shouldn't be ignored.

[ ] I have a lot of functions where the instruction switch just calls the function. I should flatten that out. That will also help unoptimized execution time.  
[ ] Explore: can I have an optimized memory read function for zero page, stack since they can't possibly do any I/O stuff? They -can- be remapped but can't do I/O. Not sure it will make a big difference.

## cpu_state

cpu_state is already generic. It can sit as-is.

execute_next_cputype has to change a lot on every cpu instance because the switch statement (implemented opcodes) will all be quite different. And of course on the 65816 there are FOUR different combinations of modes: A=8,X=8; A=16,X=16; A=8,X=16; A=16,X=8; maybe five if emulation mode ends up being different yet.

So it will be quite hard to share anything there. Except that we are composing things in the opcodes cases like so:

case OP_INC_ZP:
    zpaddr_t zpaddr = get_operand_address_zeropage(cpu);
    inc_operand(cpu, zpaddr);

