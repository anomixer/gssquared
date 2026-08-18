#include <cstdio>
#include "ScanBuffer.hpp"
#include "VideoScannerII.hpp"

void ScanBuffer::saveToFile(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == nullptr) {
        printf("Error: could not open file %s\n", filename);
        return;
    }
    uint16_t h = 0, v = 0;
    uint32_t count = get_count();
    for (int i = 0; i < count; i++) {
        Scan_t scan = get(i);
        fprintf(file, "%5d [%3dH %3dV]: %02X %02X %02X %02X %08X", i, h, v, scan.mode, scan.mainbyte, scan.auxbyte, scan.flags, scan.shr_bytes);
        fprintf(file, "| %s | ", video_mode_names[scan.mode]);
        if (scan.mode == VM_VSYNC) v = 0;
        else if (scan.mode == VM_HSYNC) { h = 0; v++; }
        else h++;

        if (scan.flags & SA_FLAG_HBL) fprintf(file, "HBL ");
        if (scan.flags & SA_FLAG_VBL) fprintf(file, "VBL ");
        if (scan.flags & SA_FLAG_BORDER) fprintf(file, "BORDER ");
        if (scan.flags & SA_FLAG_SCB) fprintf(file, "SCB ");
        if (scan.flags & SA_FLAG_PALETTE) fprintf(file, "PALETTE ");
        if (scan.flags & SA_FLAG_SHR) fprintf(file, "SHR ");
        if (scan.flags & SA_FLAG_VSYNC) { fprintf(file, "VSYNC "); }
        if (scan.flags & SA_FLAG_HSYNC) { fprintf(file, "HSYNC "); }
        fprintf(file, "\n");
    }
    fclose(file);
}