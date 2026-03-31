/*
 * X# Standard Library - Audio Module Implementation
 * ====================================================
 * Software audio mixer with WAV loading, oscillator synthesis,
 * and WAV file rendering.  Uses /dev/dsp for playback when
 * available; otherwise audio can be rendered to WAV files.
 */

#include "audio_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===== Internal types ===== */

#define SAMPLE_RATE    44100
#define MAX_SOUNDS     64
#define MIX_BUF_FRAMES 2048

typedef enum { WAVE_SINE, WAVE_SQUARE, WAVE_SAW, WAVE_TRIANGLE } WaveType;

typedef struct {
    int16_t* samples;      /* interleaved stereo PCM */
    int      frame_count;  /* number of frames (L+R = 1 frame) */
    int      channels;
    int      sample_rate;
} SoundData;

typedef struct {
    int         active;
    int         paused;
    int         is_oscillator;
    /* For loaded sounds */
    SoundData*  data;
    int         position;  /* current frame */
    int         loop;
    /* For oscillators */
    WaveType    wave;
    double      frequency;
    double      phase;
    /* Shared */
    double      volume;    /* 0.0 - 1.0 */
    double      pan;       /* -1.0 (left) to 1.0 (right) */
} SoundSlot;

typedef struct {
    int        initialized;
    SoundSlot  slots[MAX_SOUNDS];
    int        output_fd;  /* /dev/dsp or -1 */
    double     master_volume;
} AudioEngine;

static AudioEngine g_audio = {0};

/* ===== WAV Parsing ===== */

#pragma pack(push, 1)
typedef struct {
    char     riff[4];
    uint32_t file_size;
    char     wave[4];
} WavRiffHeader;

typedef struct {
    char     id[4];
    uint32_t size;
} WavChunkHeader;

typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WavFmtChunk;
#pragma pack(pop)

static SoundData* load_wav_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    WavRiffHeader header;
    if (fread(&header, sizeof(WavRiffHeader), 1, f) != 1) { fclose(f); return NULL; }
    if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) {
        fclose(f); return NULL;
    }

    WavFmtChunk fmt;
    memset(&fmt, 0, sizeof(fmt));
    int16_t* raw_data = NULL;
    int data_size = 0;

    WavChunkHeader chunk;
    while (fread(&chunk, sizeof(WavChunkHeader), 1, f) == 1) {
        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            size_t to_read = sizeof(WavFmtChunk) < chunk.size ? sizeof(WavFmtChunk) : chunk.size;
            fread(&fmt, to_read, 1, f);
            if (chunk.size > to_read) {
                fseek(f, chunk.size - to_read, SEEK_CUR);
            }
        } else if (memcmp(chunk.id, "data", 4) == 0) {
            data_size = chunk.size;
            raw_data = (int16_t*)malloc(data_size);
            if (fread(raw_data, 1, data_size, f) != (size_t)data_size) {
                free(raw_data); fclose(f); return NULL;
            }
        } else {
            fseek(f, chunk.size, SEEK_CUR);
        }
    }
    fclose(f);

    if (!raw_data || fmt.audio_format != 1) { /* PCM only */
        free(raw_data); return NULL;
    }

    SoundData* sd = (SoundData*)calloc(1, sizeof(SoundData));
    sd->channels = fmt.num_channels;
    sd->sample_rate = fmt.sample_rate;

    int bytes_per_sample = fmt.bits_per_sample / 8;
    int total_samples = data_size / bytes_per_sample;
    sd->frame_count = total_samples / fmt.num_channels;

    /* Convert to stereo int16 */
    sd->samples = (int16_t*)calloc(sd->frame_count * 2, sizeof(int16_t));

    if (fmt.bits_per_sample == 16) {
        if (fmt.num_channels == 1) {
            for (int i = 0; i < sd->frame_count; i++) {
                sd->samples[i * 2] = raw_data[i];
                sd->samples[i * 2 + 1] = raw_data[i];
            }
        } else {
            memcpy(sd->samples, raw_data, sd->frame_count * 2 * sizeof(int16_t));
        }
    } else if (fmt.bits_per_sample == 8) {
        uint8_t* u8 = (uint8_t*)raw_data;
        if (fmt.num_channels == 1) {
            for (int i = 0; i < sd->frame_count; i++) {
                int16_t s = ((int16_t)u8[i] - 128) * 256;
                sd->samples[i * 2] = s;
                sd->samples[i * 2 + 1] = s;
            }
        } else {
            for (int i = 0; i < sd->frame_count; i++) {
                sd->samples[i * 2]     = ((int16_t)u8[i * 2] - 128) * 256;
                sd->samples[i * 2 + 1] = ((int16_t)u8[i * 2 + 1] - 128) * 256;
            }
        }
    }
    free(raw_data);
    return sd;
}

/* ===== Oscillator sample generation ===== */

static double oscillator_sample(SoundSlot* s) {
    double t = s->phase;
    double val = 0.0;
    switch (s->wave) {
        case WAVE_SINE:
            val = sin(2.0 * M_PI * t);
            break;
        case WAVE_SQUARE:
            val = (fmod(t, 1.0) < 0.5) ? 1.0 : -1.0;
            break;
        case WAVE_SAW:
            val = 2.0 * fmod(t, 1.0) - 1.0;
            break;
        case WAVE_TRIANGLE: {
            double p = fmod(t, 1.0);
            val = (p < 0.5) ? (4.0 * p - 1.0) : (3.0 - 4.0 * p);
            break;
        }
    }
    s->phase += s->frequency / (double)SAMPLE_RATE;
    if (s->phase > 1e6) s->phase -= 1e6;
    return val;
}

/* ===== Mix and render ===== */

static void mix_frames(int16_t* buf, int frames) {
    float mix_l[MIX_BUF_FRAMES];
    float mix_r[MIX_BUF_FRAMES];
    memset(mix_l, 0, frames * sizeof(float));
    memset(mix_r, 0, frames * sizeof(float));

    for (int si = 0; si < MAX_SOUNDS; si++) {
        SoundSlot* slot = &g_audio.slots[si];
        if (!slot->active || slot->paused) continue;

        double vol = slot->volume * g_audio.master_volume;
        double pan_l = (slot->pan <= 0) ? 1.0 : (1.0 - slot->pan);
        double pan_r = (slot->pan >= 0) ? 1.0 : (1.0 + slot->pan);

        for (int i = 0; i < frames; i++) {
            float sample_l, sample_r;
            if (slot->is_oscillator) {
                double v = oscillator_sample(slot);
                sample_l = sample_r = (float)(v * vol);
            } else if (slot->data) {
                if (slot->position >= slot->data->frame_count) {
                    if (slot->loop) { slot->position = 0; }
                    else { slot->active = 0; break; }
                }
                sample_l = (float)(slot->data->samples[slot->position * 2] / 32768.0 * vol);
                sample_r = (float)(slot->data->samples[slot->position * 2 + 1] / 32768.0 * vol);
                slot->position++;
            } else {
                break;
            }
            mix_l[i] += sample_l * (float)pan_l;
            mix_r[i] += sample_r * (float)pan_r;
        }
    }

    for (int i = 0; i < frames; i++) {
        float l = mix_l[i], r = mix_r[i];
        if (l > 1.0f) l = 1.0f; if (l < -1.0f) l = -1.0f;
        if (r > 1.0f) r = 1.0f; if (r < -1.0f) r = -1.0f;
        buf[i * 2]     = (int16_t)(l * 32767.0f);
        buf[i * 2 + 1] = (int16_t)(r * 32767.0f);
    }
}

/* ===== Allocate a free slot ===== */

static int alloc_slot(void) {
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audio.slots[i].active && !g_audio.slots[i].data && !g_audio.slots[i].is_oscillator)
            return i;
    }
    /* Second pass: any inactive */
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audio.slots[i].active) return i;
    }
    return -1;
}

/* ===== initAudio(sampleRate?) ===== */

XsValue xs_audio_init(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (g_audio.initialized) return xs_fate(true);
    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.initialized = 1;
    g_audio.master_volume = 1.0;
    g_audio.output_fd = -1;

    /* Try /dev/dsp for OSS-compatible output */
    g_audio.output_fd = open("/dev/dsp", O_WRONLY | O_NONBLOCK);

    return xs_fate(true);
}

/* ===== loadSound(path) -> blade (slot id) ===== */

XsValue xs_audio_loadSound(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_blade(-1);
    if (!g_audio.initialized) return xs_blade(-1);

    SoundData* sd = load_wav_file(args[0].scroll);
    if (!sd) return xs_blade(-1);

    int slot = alloc_slot();
    if (slot < 0) { free(sd->samples); free(sd); return xs_blade(-1); }

    memset(&g_audio.slots[slot], 0, sizeof(SoundSlot));
    g_audio.slots[slot].data = sd;
    g_audio.slots[slot].volume = 1.0;
    g_audio.slots[slot].pan = 0.0;

    return xs_blade((int64_t)slot);
}

/* ===== playSound(slotId, loop?) ===== */

XsValue xs_audio_playSound(int argc, XsValue* args) {
    if (argc < 1) return xs_fate(false);
    int slot = (int)xs_as_spark(args[0]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_fate(false);

    SoundSlot* s = &g_audio.slots[slot];
    s->active = 1;
    s->paused = 0;
    if (!s->is_oscillator) s->position = 0;
    s->loop = (argc >= 2) ? xs_as_fate(args[1]) : 0;

    if (g_audio.output_fd >= 0) {
        int16_t buf[MIX_BUF_FRAMES * 2];
        mix_frames(buf, MIX_BUF_FRAMES);
        ssize_t w = write(g_audio.output_fd, buf, MIX_BUF_FRAMES * 2 * sizeof(int16_t));
        (void)w;
    }
    return xs_fate(true);
}

/* ===== stopSound(slotId) ===== */

XsValue xs_audio_stopSound(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int slot = (int)xs_as_spark(args[0]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_abyss();
    g_audio.slots[slot].active = 0;
    g_audio.slots[slot].paused = 0;
    if (!g_audio.slots[slot].is_oscillator && g_audio.slots[slot].data)
        g_audio.slots[slot].position = 0;
    return xs_abyss();
}

/* ===== pauseSound(slotId) ===== */

XsValue xs_audio_pauseSound(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int slot = (int)xs_as_spark(args[0]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_abyss();
    g_audio.slots[slot].paused = !g_audio.slots[slot].paused;
    return xs_fate(g_audio.slots[slot].paused);
}

/* ===== setVolume(slotId, volume) ===== */

XsValue xs_audio_setVolume(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    int slot = (int)xs_as_spark(args[0]);
    double vol = xs_as_spark(args[1]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_abyss();
    if (vol < 0.0) vol = 0.0;
    if (vol > 1.0) vol = 1.0;
    g_audio.slots[slot].volume = vol;
    return xs_abyss();
}

/* ===== setPan(slotId, pan) ===== */

XsValue xs_audio_setPan(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    int slot = (int)xs_as_spark(args[0]);
    double pan = xs_as_spark(args[1]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_abyss();
    if (pan < -1.0) pan = -1.0;
    if (pan > 1.0) pan = 1.0;
    g_audio.slots[slot].pan = pan;
    return xs_abyss();
}

/* ===== createOscillator(waveType, frequency) -> blade (slot id) ===== */

XsValue xs_audio_createOscillator(int argc, XsValue* args) {
    if (argc < 2) return xs_blade(-1);
    if (!g_audio.initialized) return xs_blade(-1);

    const char* wave_name = (args[0].type == VAL_SCROLL) ? args[0].scroll : "sine";
    double freq = xs_as_spark(args[1]);

    int slot = alloc_slot();
    if (slot < 0) return xs_blade(-1);

    memset(&g_audio.slots[slot], 0, sizeof(SoundSlot));
    g_audio.slots[slot].is_oscillator = 1;
    g_audio.slots[slot].frequency = freq;
    g_audio.slots[slot].volume = 1.0;

    if (strcmp(wave_name, "square") == 0)        g_audio.slots[slot].wave = WAVE_SQUARE;
    else if (strcmp(wave_name, "saw") == 0)       g_audio.slots[slot].wave = WAVE_SAW;
    else if (strcmp(wave_name, "triangle") == 0)  g_audio.slots[slot].wave = WAVE_TRIANGLE;
    else                                          g_audio.slots[slot].wave = WAVE_SINE;

    return xs_blade((int64_t)slot);
}

/* ===== setFrequency(slotId, freq) ===== */

XsValue xs_audio_setFrequency(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    int slot = (int)xs_as_spark(args[0]);
    if (slot < 0 || slot >= MAX_SOUNDS) return xs_abyss();
    if (!g_audio.slots[slot].is_oscillator) return xs_abyss();
    g_audio.slots[slot].frequency = xs_as_spark(args[1]);
    return xs_abyss();
}

/* ===== renderToWav(path, durationMs) -> fate ===== */

XsValue xs_audio_renderToWav(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL) return xs_fate(false);
    const char* path = args[0].scroll;
    int duration_ms = (int)xs_as_spark(args[1]);
    int total_frames = (int)((double)duration_ms / 1000.0 * SAMPLE_RATE);

    FILE* f = fopen(path, "wb");
    if (!f) return xs_fate(false);

    int data_bytes = total_frames * 2 * (int)sizeof(int16_t);

    /* Write WAV header */
    uint32_t file_size = 36 + data_bytes;
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 2;
    uint32_t sr = SAMPLE_RATE;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sr * num_channels * bits_per_sample / 8;
    uint16_t block_align = num_channels * bits_per_sample / 8;
    uint32_t data_sz = (uint32_t)data_bytes;

    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    fwrite(&fmt_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sr, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&data_sz, 4, 1, f);

    int remaining = total_frames;
    while (remaining > 0) {
        int chunk = (remaining < MIX_BUF_FRAMES) ? remaining : MIX_BUF_FRAMES;
        int16_t buf[MIX_BUF_FRAMES * 2];
        mix_frames(buf, chunk);
        fwrite(buf, sizeof(int16_t), chunk * 2, f);
        remaining -= chunk;
    }

    fclose(f);
    return xs_fate(true);
}

/* ===== close() ===== */

XsValue xs_audio_close(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (!g_audio.initialized) return xs_abyss();

    for (int i = 0; i < MAX_SOUNDS; i++) {
        SoundSlot* s = &g_audio.slots[i];
        if (s->data) {
            free(s->data->samples);
            free(s->data);
            s->data = NULL;
        }
        s->active = 0;
    }
    if (g_audio.output_fd >= 0) {
        close(g_audio.output_fd);
        g_audio.output_fd = -1;
    }
    g_audio.initialized = 0;
    return xs_abyss();
}

/* ===== Registration ===== */

void xs_audio_register(VM* vm) {
    vm_register_native(vm, "Audio.init",             xs_audio_init);
    vm_register_native(vm, "Audio.loadSound",        xs_audio_loadSound);
    vm_register_native(vm, "Audio.playSound",        xs_audio_playSound);
    vm_register_native(vm, "Audio.stopSound",        xs_audio_stopSound);
    vm_register_native(vm, "Audio.pauseSound",       xs_audio_pauseSound);
    vm_register_native(vm, "Audio.setVolume",        xs_audio_setVolume);
    vm_register_native(vm, "Audio.setPan",           xs_audio_setPan);
    vm_register_native(vm, "Audio.createOscillator", xs_audio_createOscillator);
    vm_register_native(vm, "Audio.setFrequency",     xs_audio_setFrequency);
    vm_register_native(vm, "Audio.renderToWav",      xs_audio_renderToWav);
    vm_register_native(vm, "Audio.close",            xs_audio_close);
}
