# MMU - Refactor

Currently there is a blurry line between the CPU and memory. "Memory" meaning, the memory bus. Let's follow a typical call from an instruction decode:

```
core6502:
case OP_LDA_ABS: /* LDA Absolute */
    {
        cpu->a_lo = get_operand_absolute(cpu);
        set_n_z_flags(cpu, cpu->a_lo);
    }
    break;

6502.hpp:
inline byte_t get_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    byte_t N = read_byte(cpu, addr);
    //if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, addr);
    TRACE(cpu->trace_entry.data = N;)
    return N;
}

inline absaddr_t get_operand_address_absolute(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    //if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    TRACE(cpu->trace_entry.operand = addr; cpu->trace_entry.eaddr = addr; )
    return addr;
}

memory.cpp:
uint16_t read_word_from_pc(cpu_state *cpu) {
    uint16_t value = read_byte(cpu, cpu->pc) | (read_byte(cpu, cpu->pc + 1) << 8);
    cpu->pc += 2;
    return value;
}

uint8_t read_byte(cpu_state *cpu, uint16_t address) {
    uint8_t value = read_memory(cpu, address);
    return value;
}

uint8_t read_memory(cpu_state *cpu, uint16_t address) {
    incr_cycles(cpu);
    uint8_t typ = cpu->memory->page_info[address / GS2_PAGE_SIZE].type;
    if (typ == MEM_ROM || typ == MEM_RAM) {
        return cpu->memory->pages_read[address / GS2_PAGE_SIZE][address % GS2_PAGE_SIZE];
    }
    // IO - call the memory bus dispatcher thingy.
    return memory_bus_read(cpu, address);
}
```

LDA absolute make two sets of memory fetches. First, it gets two bytes (a word) from the PC. Then, it gets the byte at that address.

If the page is marked as RAM or ROM, then we cut out there and (finally) return the value. Note a few things:
* these are spread across 3 files, so while certain functions inside a file can benefit from inlining, every time we cross a file we lose that.
* this is quite a deep call stack so we are really losing out.
* for some reason, incr_cycles is WAY down in read_memory.

All the stuff in memory.cpp should be in the CPU object. and read_memory() should be the MMU interface to the CPU. AND, incr_cycles() should be in read_byte. (and write_byte).
The only things that call read_memory() are read_byte(), and pdblock2. Currently we sort of simulate dma by ticking the cpu cycles once per byte read/written. But we'll put in a TODO to deal with that later. (it's not that important).

OK, now read_memory and write_memory are in the "oldmmu" library, and I brought in read_word read_byte etc into the CPU object. Performance about the same as before. Now I inlined them, let's see what happens..
about the same as before. I think these matter, but, there may be additional bottlenecks elsewhere. (it's 522MHz. Why am I complaining?? hahaha.)

I forgot to take out unneeded cpu_state argument to these functions, so in essence I was passing the cpu_state twice. let's see.. No difference. I suppose subroutines on the ARM are pretty efficient. (Or the stuff being called is inefficient..)

Important part is, CPU and oldMMU are decoupled. Ready to start testing with new MMU. Woot! First, do the cputest. OK, that's done.
Now, of course, the whole rest of the Apple II will be quite a bit more involved. Have to modify all the devices etc. And test.
One note: I did the test at 574MHz before. Now I run it at 500-515MHz. But it's working.
