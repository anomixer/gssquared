#pragma once

#include <cstdint>

#include "regs.hpp"

enum rw {
    READ =0,
    WRITE = 1,
    READ_EXPECT = 2,
    END = 3,
};

struct reg_record_t {
    rw action;
    uint64_t cycle;
    uint8_t reg;
    uint8_t value;
};

reg_record_t recs1[] = {
    // we're in 1-shot mode.
    {WRITE, 1, MB_6522_IER, 0b11000000}, // enable T1 interrupt (7:1 = "set", 6:1 = "set")
    {WRITE,5, MB_6522_T1L_L, 0x05},
    {WRITE, 8, MB_6522_T1C_H, 0x00},
    {WRITE, 20, MB_6522_T1C_H, 0x00}, // write T1C-H, should clear interrupt pending and set up for another 1-shot

    {WRITE, 29, MB_6522_ACR, 0x40},
    {WRITE, 30, MB_6522_T1C_H, 0x00},
    {WRITE, 40, MB_6522_IFR, 0x40}, // clear T1 interrupt
    {WRITE, 60, MB_6522_IER,0b10100000}, // enable T2 interrupt also
    {WRITE, 61, MB_6522_T2L_L, 0x05},
    {WRITE, 62, MB_6522_T2C_H, 0x00},

    {WRITE, 79, MB_6522_IER, 0x40}, // disable T1 interrupt
    {WRITE, 80, MB_6522_T1L_L, 10},
    {WRITE, 81, MB_6522_T1C_H, 0x00},
    {WRITE, 85, MB_6522_T1L_L, 10},
    {WRITE, 86, MB_6522_T1C_H, 0x00},
    {WRITE, 90, MB_6522_T1L_L, 10},
    {WRITE, 91, MB_6522_T1C_H, 0x00},
    

    // should have a concept here to READ the register.
    {READ, 100, MB_6522_T2L_L, 0x00},
    {END, 105, 0, 0},
};

// Test:
// Continuous mode, T1 Latch set to 0, should cycle 1.5 times before IRQ.
reg_record_t recs2[] = {
    {WRITE, 1, MB_6522_IER, 0b11000000}, // enable T1 interrupt (7:1 = "set", 6:1 = "set")
    {WRITE, 2, MB_6522_ACR, 0x40},
    {WRITE,5, MB_6522_T1L_L, 0x00},
    {WRITE, 8, MB_6522_T1C_H, 0x00},
    {END, 30, 0, 0},
};

// Test:
// Continuous mode, T1 Latch set to 5
reg_record_t recs3[] = {    
    {WRITE, 1, MB_6522_IER, 0b11000000}, // enable T1 interrupt (7:1 = "set", 6:1 = "set")
    {WRITE, 2, MB_6522_ACR, 0x40},
    {WRITE,5, MB_6522_T1L_L, 0x05},
    {WRITE, 8, MB_6522_T1C_H, 0x00},
    {END, 30, 0, 0},
};

// Test:
// Continuous mode, T1 Latch set to FFFF
reg_record_t recs4[] = {    
    {WRITE, 1, MB_6522_IER, 0b11000000}, // enable T1 interrupt (7:1 = "set", 6:1 = "set")
    {WRITE, 2, MB_6522_ACR, 0x40},
    {WRITE,5, MB_6522_T1L_L, 0xFF},
    {WRITE, 8, MB_6522_T1C_H, 0xFF},
    {END, 65550, 0, 0},
};

// Test:
// Continuous mode, T1 Latch set to 5, then go back to one-shot mode.
reg_record_t recs5[] = {    
    {WRITE, 1, MB_6522_IER, 0b11000000}, // enable T1 interrupt (7:1 = "set", 6:1 = "set")
    {WRITE, 2, MB_6522_ACR, 0x40},
    {WRITE,5, MB_6522_T1L_L, 0x05},
    {WRITE, 8, MB_6522_T1C_H, 0x00},
    {WRITE, 18, MB_6522_ACR, 0x00}, // turn off continuous mode
    {END, 50, 0, 0},
};

// mb-audit repro: 11:03:00 (T6522_3 subtest 00)
// Sequence extracted from chip-6522.a:
// 1) write T1L_L = FF
// 2) write T1C_H = FF  (preload T1C/T1L=FFFF)
// 3) write T1C_H = 00  (STY abs with Y=00)
// 4) first read of T1C_L should be F1
// 5) next cycle read of T1C_L should be F0
// Simple T1 load-then-read with compressed cycle spacing.
// Post-fix semantics: T1C_H write at cycle 10 is a pure load cycle (no decrement
// on the transition out of cycle 10). First decrement lands on the transition
// 11 -> 12, so:
//   read at cycle 24 -> $F2 (13 decrements from $FF)
//   read at cycle 25 -> $F1 (14 decrements from $FF)
reg_record_t recs6[] = {
    {WRITE,  1, MB_6522_T1L_L, 0xFF},
    {WRITE,  2, MB_6522_T1C_H, 0xFF},
    {WRITE, 10, MB_6522_T1C_H, 0x00},
    {READ_EXPECT, 24, MB_6522_T1C_L, 0xF2},
    {READ_EXPECT, 25, MB_6522_T1C_L, 0xF1},
    {END, 30, 0, 0},
};

// mb-audit repro (accurate cycle spacing): 11:03:00 / T6522_3 subTest #0
// Real mb-audit sequence cycle accounting (from chip-6522.a @readT1C):
//   STY abs  (4cy, write T1C_H on cycle 4 = cycle W)
//   JSR      (6cy)
//   LDY zp   (3cy)
//   DEY      (2cy)
//   LDA abs,y(4cy, read T1C_L on cycle 4 = cycle W+15)
// Real 6522 does NOT decrement T1 on the cycle T1C_H is written (the load cycle),
// so 15 elapsed cycles yield 14 decrements and T1C_L reads $F1 (= $FF - 14).
// If our emulator decrements on the load cycle too, we'll observe $F0 (= 15 decrements).
reg_record_t recs7[] = {
    {WRITE,   1, MB_6522_T1L_L, 0xFF},
    {WRITE,   2, MB_6522_T1C_H, 0xFF},       // prime T1C=T1L=FFFF
    {WRITE, 100, MB_6522_T1C_H, 0x00},       // STY abs-equivalent (cycle W=100)
    {READ_EXPECT, 115, MB_6522_T1C_L, 0xF1}, // W+15: mb-audit expects F1
    {END, 120, 0, 0},
};

// mb-audit repro for DetectMegaAudioCard (chip-6522.a line 132).
// The routine primes T1 with T1C=$0006 (one-shot mode) and watches exactly
// when the T1 IFR flag becomes visible. A real 6522 asserts IRQ on the
// 2nd cycle of the subsequent STA zp (i.e. 8 cycles after the T1C_H write).
// An FPGA-based 6522 (MegaAudio) asserts on the 3rd cycle (9 cycles after).
// Our pre-11:03:00-fix emulator matched real 6522. After the load-cycle skip
// was added, the underflow + 2-step-rollover chain now emits IRQ 1 cycle
// later, masquerading as MegaAudio. This test locks in the correct timing.
//
// Cycle map from the DetectMegaAudioCard write of T1C_H = 0 at cycle W:
//   W+8 -> IFR bit6 should be set (real 6522 timing)
//   W+9 -> IFR bit6 should remain set (no change)
// Before W+8, IFR should read 0.
reg_record_t recs8[] = {
    {WRITE,  1, MB_6522_IER, 0xC0},        // b7=set, b6: enable T1 IRQ
    {WRITE,  2, MB_6522_ACR, 0x00},        // one-shot mode
    {WRITE,  3, MB_6522_T1L_L, 0x06},
    {WRITE, 10, MB_6522_T1C_H, 0x00},      // T1C = $0006 load at cycle W=10
    {READ_EXPECT, 15, MB_6522_IFR, 0x00},  // W+5: counter mid-countdown
    {READ_EXPECT, 16, MB_6522_IFR, 0x00},  // W+6
    {READ_EXPECT, 17, MB_6522_IFR, 0x00},  // W+7
    {READ_EXPECT, 18, MB_6522_IFR, 0xC0},  // W+8: mb-audit real-6522 says IRQ visible here
    {READ_EXPECT, 19, MB_6522_IFR, 0xC0},  // W+9
    {END, 25, 0, 0},
};

// mb-audit repro: 11:09:01 (T6522_9 subTest #1)
//   "Test 4x that T1C in one-shot mode (on underflow) gets reloaded with T1C_h=$02"
// The real 6522 reloads T1C from T1L on *every* underflow in both continuous
// and one-shot modes; only the IRQ is "one-shot". If the counter is not
// reloaded in one-shot, WaitT1Underflow ultimately reads T1C_H=$FF because
// the counter merely continues past $FFFF ($0000 -> $FFFF -> $FFFE ...).
//
// Here we prime T1C=T1L=$0101 in one-shot mode and observe T1C_H across the
// underflow window. The critical read point is the cycle *after* rollover
// step-B completes, where:
//   - correct behavior: t1_counter has been reloaded from the latch => T1H=$01
//   - buggy behavior:   t1_counter has decremented past $FFFF       => T1H=$FF
//
// Cycle map (W = T1C_H write cycle = 3):
//   W+0   : pure load cycle (t1_skip_next_decrement = true)
//   W+1   : skip consumed, counter stays $0101
//   W+2   : first decrement -> $0100
//   W+258 : counter enters as $0000, IRQ fires, rollover=1, counter -> $FFFF
//   W+259 : rollover step-B: one-shot reload from latch -> counter = $0101
//   W+260 : decrement from reload -> $0100
// Reads in the harness are taken *before* that cycle's incr_cycle, so a read
// scheduled at cycle (W+260) observes the post-step-B state ($0101 -> T1H=$01).
reg_record_t recs9[] = {
    {WRITE,  1, MB_6522_ACR, 0x00},        // one-shot mode (b6=0)
    {WRITE,  2, MB_6522_T1L_L, 0x01},
    {WRITE,  3, MB_6522_T1C_H, 0x01},      // T1C = T1L = $0101 (W=3)
    {READ_EXPECT, 260, MB_6522_T1C_H, 0x00}, // mid-countdown, T1C just underflowed -> $FFFF prior cycle
    {READ_EXPECT, 263, MB_6522_T1C_H, 0x01}, // post-reload: fixed=$01, buggy=$FF
    {END, 270, 0, 0},
};

reg_record_t *recs[] = {
    recs1,
    recs2,
    recs3,
    recs4,
    recs5,
    recs6,
    recs7,
    recs8,
    recs9,
};
