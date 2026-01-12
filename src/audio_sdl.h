#pragma once

#include "decode.h"

#ifdef __cplusplus
extern "C" {
#endif

void audio_pump(
    void **audio_render);

int sdl_audio_init(
    void **audio_render,
    int samplerate,
    int channels,
    int format,
    int buffer);

void sdl_audio_release(
    void *audio_render);

void sdl_audio_set_dec(
    void *audio_render,
    decoder *dec);

void sdl_audio_update_stream_format(
    void *audio_render,
    int samplerate,
    int channels);

void sdl_audio_pause(
    void *audio_render,
    int state);

#ifdef __cplusplus
}

#endif
