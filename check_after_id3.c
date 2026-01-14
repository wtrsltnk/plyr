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

    // Read first 10 bytes to get ID3 header
    unsigned char header[10];
    if (fread(header, 1, 10, f) != 10)
    {
        printf("ERROR: Cannot read ID3 header\n");
        fclose(f);
        return 1;
    }

    // Check for ID3v2
    if (header[0] == 'I' && header[1] == 'D' && header[2] == '3')
    {
        unsigned char flags = header[5];

        // Decode synchsafe integer (ID3v2 size)
        int size = ((header[6] & 0x7F) << 21) |
                   ((header[7] & 0x7F) << 14) |
                   ((header[8] & 0x7F) << 7) |
                   (header[9] & 0x7F);

        printf("ID3v2.%d tag found\n", header[3]);
        printf("ID3 tag size: %d bytes\n", size);
        printf("Flags: 0x%02X\n", flags);

        // Check flags
        int has_extended_header = (flags & 0x40) != 0;
        int has_footer = (flags & 0x10) != 0;
        int unsynchronisation = (flags & 0x80) != 0;

        printf("  Extended header: %s\n", has_extended_header ? "YES" : "no");
        printf("  Footer: %s\n", has_footer ? "YES" : "no");
        printf("  Unsynchronisation: %s\n", unsynchronisation ? "YES" : "no");

        int total_skip = 10 + size;
        if (has_footer) {
            total_skip += 10; // Footer is also 10 bytes
            printf("  (Adding 10 bytes for footer)\n");
        }

        printf("Total bytes to skip: %d\n\n", total_skip);

        // Seek past ID3 tag (and footer if present)
        fseek(f, total_skip, SEEK_SET);

        // Read next 16 bytes after ID3 tag
        unsigned char after[16];
        size_t read = fread(after, 1, 16, f);

        printf("First %zu bytes after ID3 tag (hex): ", read);
        for (size_t i = 0; i < read; i++)
        {
            printf("%02X ", after[i]);
        }
        printf("\n");

        // Check what format this is
        if (read >= 4)
        {
            if (after[0] == 0xFF && (after[1] & 0xE0) == 0xE0)
            {
                printf("Format: MP3 frame sync found!\n");
                printf("  MPEG version: %d\n", (after[1] >> 3) & 0x03);
                printf("  Layer: %d\n", (after[1] >> 1) & 0x03);
            }
            else if (after[0] == 0x00 && after[1] == 0x00 && after[2] == 0x00)
            {
                printf("Format: Null/padding bytes (possibly file corruption or trailer)\n");
            }
            else if (read >= 12 && after[4] == 'f' && after[5] == 't' && after[6] == 'y' && after[7] == 'p')
            {
                printf("Format: MP4/M4A container detected! (This is NOT an MP3 file)\n");
            }
            else
            {
                printf("Format: Unknown/unsupported format\n");
                printf("  ASCII: ");
                for (size_t i = 0; i < read; i++)
                {
                    if (after[i] >= 32 && after[i] < 127)
                        printf("%c", after[i]);
                    else
                        printf(".");
                }
                printf("\n");
            }
        }

        // Get file size
        fseek(f, 0, SEEK_END);
        long filesize = ftell(f);
        printf("\nFile size: %ld bytes\n", filesize);
        printf("Audio data starts at: %d bytes\n", size + 10);
        printf("Audio data size: %ld bytes\n", filesize - (size + 10));
    }
    else
    {
        printf("No ID3v2 tag found at start of file\n");
    }

    fclose(f);
    return 0;
}
