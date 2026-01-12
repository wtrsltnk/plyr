#include "audio_sdl.h"

#include <stddef.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

typedef struct audio_ctx
{
    SDL_AudioDeviceID dev;
    SDL_AudioStream *stream;
    SDL_AudioSpec spec;

    decoder *dec;

    bool running;
} audio_ctx;


/* ============================================================
   Audio pump (replaces SDL2 callback)
   ============================================================ */
void audio_pump(
    void **audio_render)
{
    audio_ctx *ctx = (audio_ctx *)(*audio_render);

    if (ctx == NULL ) {
        return;
    }

    if (ctx->running == false) {
        return;
    }

    if (ctx->stream == NULL ) {
        return;
    }

    if (ctx->dec == NULL) {
        return;
    }

    // Check how much data is queued, and top it up if needed
    int queued = SDL_GetAudioStreamQueued(ctx->stream);

    // Calculate bytes per sample based on format
    int bytes_per_sample = (ctx->spec.format == SDL_AUDIO_F32) ? sizeof(float) : sizeof(Sint16);
    int target_bytes = ctx->spec.freq * ctx->spec.channels * bytes_per_sample / 4; // ~250ms of audio

    if (queued >= target_bytes) {
        return; // Already have enough queued
    }

    int want = target_bytes - queued;

    Uint8 *buffer = (Uint8 *)SDL_malloc(want);
    if (!buffer) {
        return;
    }

    int decoded_samples = decode_samples(ctx->dec, buffer, want);
    int decoded_bytes = decoded_samples * sizeof(mp3d_sample_t);

    SDL_PutAudioStreamData(ctx->stream, buffer, decoded_bytes);
    SDL_free(buffer);
}


/* ============================================================
   Init
   ============================================================ */
int sdl_audio_init(
    void **audio_render,
    int samplerate,
    int channels,
    int format,
    int buffer /* unused in SDL3 */
    )
{
    (void)buffer;
    *audio_render = NULL;

    if (!SDL_Init(SDL_INIT_AUDIO)) {
        printf("error: sdl init failed: %s\n", SDL_GetError());
        return 0;
    }

    audio_ctx *ctx = (audio_ctx *)calloc(1, sizeof(audio_ctx));
    if (!ctx) {
        return 0;
    }

    SDL_zero(ctx->spec);
    ctx->spec.freq = samplerate;
    ctx->spec.format = format ? SDL_AUDIO_F32 : SDL_AUDIO_S16;
    ctx->spec.channels = channels;

    /* Open default output device */
    ctx->dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &ctx->spec);
    if (!ctx->dev) {
        printf("error: couldn't open audio: %s\n", SDL_GetError());
        free(ctx);
        return 0;
    }

    /* Create passthrough stream */
    ctx->stream = SDL_CreateAudioStream(&ctx->spec, &ctx->spec);
    if (!ctx->stream) {
        printf("error: couldn't create audio stream: %s\n", SDL_GetError());
        SDL_CloseAudioDevice(ctx->dev);
        free(ctx);
        return 0;
    }

    SDL_BindAudioStream(ctx->dev, ctx->stream);

    printf("Opened audio device: %s\n",
           SDL_GetAudioDeviceName(ctx->dev));

    /* Resume the audio device to start playback */
    SDL_ResumeAudioDevice(ctx->dev);

    ctx->running = true;

    *audio_render = ctx;

    return 1;
}

/* ============================================================
   Release
   ============================================================ */
void sdl_audio_release(void *audio_render)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;
    if (!ctx) return;

    // Stop all future audio_pump calls
    ctx->running = false;

    if (ctx->dev) {
        SDL_CloseAudioDevice(ctx->dev);
    }

    if (ctx->stream) {
        SDL_DestroyAudioStream(ctx->stream);
    }

    free(ctx);
}


/* ============================================================
   Set decoder
   ============================================================ */
void sdl_audio_set_dec(
    void *audio_render,
    decoder *dec)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;
    ctx->dec = dec;
}

/* ============================================================
   Update stream format (for new file with different sample rate)
   ============================================================ */
void sdl_audio_update_stream_format(
    void *audio_render,
    int samplerate,
    int channels)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;
    if (!ctx) return;

    // Destroy old stream
    if (ctx->stream) {
        SDL_DestroyAudioStream(ctx->stream);
        ctx->stream = NULL;
    }

    // Create new stream with correct input format (from MP3) and output format (device)
    SDL_AudioSpec src_spec, dst_spec;
    SDL_zero(src_spec);
    SDL_zero(dst_spec);

    // Source: MP3 file format
    src_spec.freq = samplerate;
    src_spec.format = SDL_AUDIO_S16;  // minimp3 outputs S16
    src_spec.channels = channels;

    // Destination: Output device format
    dst_spec = ctx->spec;

    ctx->stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
    if (ctx->stream) {
        SDL_BindAudioStream(ctx->dev, ctx->stream);
        printf("Updated audio stream: %d Hz, %d channels -> %d Hz, %d channels\n",
               samplerate, channels, ctx->spec.freq, ctx->spec.channels);
    } else {
        printf("error: couldn't recreate audio stream: %s\n", SDL_GetError());
    }
}

/* ============================================================
   Pause
   ============================================================ */
void sdl_audio_pause(
    void *audio_render,
    int state)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;

    if (state) {
        SDL_PauseAudioDevice(ctx->dev);
    } else {
        SDL_ResumeAudioDevice(ctx->dev);
    }
}
