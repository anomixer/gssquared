#include <stdio.h>
#include <stdlib.h>

#define CHAR_LINE 9

int main(int argc, char *argv[]) {
    FILE *fp;
    unsigned char buffer[16];
    size_t bytes_read;
    int char_index = 0;

    char matrix[10][10];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <rom_filename>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    unsigned char data[2048];
    fread(data, 1, 2048, fp);
    
    // Read 8 bytes at a time (one character)
    int i = 0;
    do {
        
        // Print character index and file offset
        printf("Character 0x%02X (offset 0x%04X):\n", char_index, i);

        if ((i & 0xFF) == 0xFC) {
            printf("Page boundary. Skipping next 4 bytes\n");
            i += 4;
            continue;
        }
        
        // Print each line of the character
        for (int index = 0; index < CHAR_LINE; index++) {
            // Process each bit
            for (int bit = 0; bit <= 7; bit++) {
                matrix[index][8-bit] = (data[i+index] & (1 << bit)) ? '.' : '*';
            }
        }
        for (int y = 0; y <8; y++) {
            for (int x = 0; x < 9; x++) {
                printf("%c", matrix[x][y]);
            }
            printf("\n");
        }
        printf("\n");  // Blank line between characters
        char_index++;
        i+= CHAR_LINE;
    } while (i < 2048);

    fclose(fp);
    return 0;
}
