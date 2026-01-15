#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("ERROR: Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // Read first 16 bytes
    unsigned char buffer[16];
    size_t read = fread(buffer, 1, 16, f);

    printf("File: %s\n", argv[1]);
    printf("First %zu bytes (hex): ", read);
    for (size_t i = 0; i < read; i++)
    {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    // Check for common audio formats
    if (read >= 3)
    {
        // MP3 frame header
        if ((buffer[0] == 0xFF && (buffer[1] & 0xE0) == 0xE0) ||
            (buffer[0] == 0xFF && buffer[1] == 0xFB))
        {
            printf("Format: MP3 frame sync detected\n");
        }
        // ID3 tag
        else if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
        {
            printf("Format: ID3 tag detected (MP3 with metadata)\n");
        }
        else
        {
            printf("Format: Unknown (first bytes: %02X %02X %02X)\n",
                   buffer[0], buffer[1], buffer[2]);
        }
    }

    fclose(f);
    return 0;
}
