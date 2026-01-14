#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#define MINIMP3_IMPLEMENTATION
#include "decode.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void get_spectrum(decoder *dec, int numch)
{
    int i, ch, band;
    const mp3dec_t *d = &dec->mp3d.mp3d;
    // Average spectrum power for 32 frequency bands
    for (ch = 0; ch < numch; ch++)
    {
        for (band = 0; band < 32; band++)
        {
            float band_power = 0;
            for (i = 0; i < 9; i++)
            {
                float sample = d->mdct_overlap[ch][i + band*9];
                band_power += sample*sample;
            }
            // w/o averaging
            dec->spectrum[band][ch] = band_power/9;
        }
    }
    // Calculate dB scale from power for better visualization
    for (ch = 0; ch < numch; ch++)
    {
        for (band = 0; band < 32; band++)
        {
            float power = dec->spectrum[band][ch];
            float db_offset = 100;      // to shift dB values from [0..-inf] to [max..0]
            float db = 10*log10f(power + 1e-15f) + db_offset;
            if (db < 0) db = 0;
            dec->spectrum[band][ch] = db;
        }
    }
}

int decode_samples(decoder *dec, uint8_t *buf, int bytes)
{
    memset(buf, 0, bytes);
    int samples = mp3dec_ex_read(&dec->mp3d, (mp3d_sample_t*)buf, bytes/sizeof(mp3d_sample_t));

    if (samples > 0)
    {
        get_spectrum(dec, dec->mp3d.info.channels);
    }

    return samples;
}

int open_dec(decoder *dec, const char *file_name)
{
    if (!dec || !file_name || !*file_name)
    {
        fprintf(stderr, "decode error: invalid parameters\n");
        return 0;
    }

    memset(dec, 0, sizeof(*dec));

    printf("Attempting to open file: %s\n", file_name);

    int result = mp3dec_ex_open(&dec->mp3d, file_name, MP3D_SEEK_TO_SAMPLE);

    printf("mp3dec_ex_open result: %d\n", result);
    printf("  samples: %llu\n", (unsigned long long)dec->mp3d.samples);
    printf("  info.hz: %d\n", dec->mp3d.info.hz);
    printf("  info.channels: %d\n", dec->mp3d.info.channels);
    printf("  info.layer: %d\n", dec->mp3d.info.layer);
    printf("  info.bitrate_kbps: %d\n", dec->mp3d.info.bitrate_kbps);

    if (result != 0)
    {
        fprintf(stderr, "decode error: mp3dec_ex_open failed with code %d for file: %s\n", result, file_name);

        // Print common error codes
        if (result == -1) fprintf(stderr, "  Error: File I/O error or file not found\n");
        else if (result == -2) fprintf(stderr, "  Error: Not enough memory\n");
        else fprintf(stderr, "  Error: Unknown error code\n");

        return 0;
    }

    if (!dec->mp3d.samples)
    {
        fprintf(stderr, "decode error: no audio samples found in file: %s\n", file_name);
        fprintf(stderr, "  This might indicate a corrupted file or unsupported format (e.g., MP4/M4A)\n");
        mp3dec_ex_close(&dec->mp3d);
        return 0;
    }

    printf("Successfully opened MP3: %llu samples, %d Hz, %d channels\n",
           (unsigned long long)dec->mp3d.samples, dec->mp3d.info.hz, dec->mp3d.info.channels);

    return 1;
}

int close_dec(decoder *dec)
{
    mp3dec_ex_close(&dec->mp3d);
    memset(dec, 0, sizeof(*dec));
    return 1;
}

// Gradually decay spectrum values to zero (for pause effect)
void decay_spectrum(decoder *dec)
{
    if (!dec)
        return;

    const float decay_rate = 0.98f; // Adjust for faster/slower fall

    for (int band = 0; band < 32; band++)
    {
        for (int ch = 0; ch < 2; ch++)
        {
            dec->spectrum[band][ch] *= decay_rate;

            // Snap to zero when very small to avoid floating point drift
            if (dec->spectrum[band][ch] < 0.1f)
                dec->spectrum[band][ch] = 0.0f;
        }
    }
}
