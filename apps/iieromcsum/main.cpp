
// load iie ROM
#include <cstdint>
#include <cstdio>

const char *rom_path = "assets/roms/apple2e/main.rom";

int main(int argc, char **argv) {
    uint8_t *rom = new uint8_t[0x2000];

    FILE *rom_file = fopen(rom_path, "rb");
    fread(rom, 1, 0x2000, rom_file);
    fclose(rom_file);

    uint32_t checksum = 0;
    for (int i = 0x0100; i < 0x2000; i++) {
        if (i == 0x0400) continue;
        checksum += rom[i];
        if (i % 0x100 == 0xFF) {
            printf("csum at %04X: %08X\n", i + 0xC000, checksum);
        }
    }
    printf("Checksum: %04X\n", checksum);
}