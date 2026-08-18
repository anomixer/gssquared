# Tracing / Debugging

We will ease into debugging. Otherwise it will be a giant rabbit hole.

But, tracing.

Have a large memory block.
Debug statements, generate text into the block. Keep track of head and tail. And, wraparound.
For now, have a way to dump that to disk, starting from wherever the head is.
Eventually, be able to scroll through it in-app.
Have a UI widget on that screen that lets you enable the various debug bits one at a time.

So instead of constructing the text output, the trace creates first a binary log. Each binary log line is a compact array of structs.
This is constructed without any conditionals.

E.g.,

struct system_log_entry_t {
    uint64_t cycle;
    uint8_t opcode[4]; // up to 4 bytes per opcode for 65816.

    uint16_t a;
    uint16_t x;
    uint16_t y;
    uint16_t sp;
    uint16_t d;
    uint8_t p;
    uint8_t db;
    uint8_t pb;

    uint8_t address_mode; // decoded address mode for instruction so we don't have to do it again.
    uint32_t effective_addr; // the effective memory address used.
    uint8_t memory_read_write;
}

cpu {
    alignas(64) log_entry_t log;
}

12 + 11 + 1 + 2 + 1 = 31 <- sweet

This records the opcode and operand data; the values of registers before execution of the instruction; and the memory value read or written.

So this is under 32 bytes probably. We can record the last 1 million instructions in 32 megabytes.
Decoding the info of course uses lots of conditionals, but that will be out of band, and only for a relative handful of instructions, and we won't care. Each frame of debugger display might have 60 to 100 lines on it. Big whoop!

Also then easy to dump the whole thing in binary format and decode outside the emu also.
log Trace on-off hot key; when enabled, we stream binary log to disk. This will generate reams of data.

Step over-execute single instruction. if single instruction is JSR, then execute until we get to a RTS.
Step into-just execute single instruction.
breakpoint - break on instruction execution; break on memory access. These can be compact bit arrays of 4KBytes. one bit per byte.

We want to make the log struct be divisible into a cache line, so, 64 bytes, 32 maybe. And be 32-byte aligned. For that matter, we want the whole cpu struct to be aligned that way.

Examining the current code, and how it is compiled, we definitely cause a lot of cruft having the IF(DEBUG()) stuff in there. Since we are bit testing against a variable, the compiler cannot optimize away the IFs even if no flags are set.
I think the log record needs to go into cpu().
Have a single bool in cpu for tracing enabled / disabled.

