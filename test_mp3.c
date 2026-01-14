#include <stdio.h>
#include <string.h>
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <mp3_file>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    printf("Testing file: %s\n\n", filename);

    // Try to open with minimp3
    mp3dec_ex_t dec;
    memset(&dec, 0, sizeof(dec));

    printf("Attempting mp3dec_ex_open...\n");
    int result = mp3dec_ex_open(&dec, filename, MP3D_SEEK_TO_SAMPLE);

    printf("\n=== Results ===\n");
    printf("Return code: %d\n", result);

    if (result == 0)
    {
        printf("SUCCESS: File opened\n\n");
        printf("Audio properties:\n");
        printf("  Samples: %llu\n", (unsigned long long)dec.samples);
        printf("  Sample rate: %d Hz\n", dec.info.hz);
        printf("  Channels: %d\n", dec.info.channels);
        printf("  Layer: %d\n", dec.info.layer);
        printf("  Bitrate: %d kbps\n", dec.info.bitrate_kbps);

        if (dec.samples > 0)
        {
            double duration = (double)dec.samples / (double)dec.info.channels / (double)dec.info.hz;
            printf("  Duration: %.2f seconds\n", duration);

            // Try to read a small chunk
            mp3d_sample_t buffer[1024];
            int samples_read = mp3dec_ex_read(&dec, buffer, 1024);
            printf("\nTest read: %d samples\n", samples_read);

            if (samples_read > 0)
            {
                printf("First few samples: %d, %d, %d\n",
                       buffer[0], buffer[1], buffer[2]);
            }
        }
        else
        {
            printf("\nWARNING: samples = 0 (empty or invalid file)\n");
        }

        mp3dec_ex_close(&dec);
    }
    else
    {
        printf("FAILED: Could not open file\n");
        printf("Error codes:\n");
        printf("  -1 = File I/O error or file not found\n");
        printf("  -2 = Not enough memory\n");
        printf("  Other = Unknown error\n");
    }

    return result;
}
