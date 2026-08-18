#pragma once

#include <functional>
#include <cstdint>
#include "cpu.hpp"

struct Test {
    uint8_t program[256];
    uint32_t program_size;
    uint32_t program_address;
    std::function<void(cpu_state*)> setup;
    std::function<void(cpu_state*)> assertions;
};

void demo_ram();
