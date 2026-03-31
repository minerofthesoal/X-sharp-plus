/*
 * X# Standard Library - Audio Module
 * =====================================
 * 10 audio functions: sound loading, playback, oscillator synthesis.
 * Uses software mixing; output can be piped to /dev/dsp or written to WAV.
 */

#ifndef XS_AUDIO_LIB_H
#define XS_AUDIO_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_audio_init(int argc, XsValue* args);
XsValue xs_audio_loadSound(int argc, XsValue* args);
XsValue xs_audio_playSound(int argc, XsValue* args);
XsValue xs_audio_stopSound(int argc, XsValue* args);
XsValue xs_audio_pauseSound(int argc, XsValue* args);
XsValue xs_audio_setVolume(int argc, XsValue* args);
XsValue xs_audio_setPan(int argc, XsValue* args);
XsValue xs_audio_createOscillator(int argc, XsValue* args);
XsValue xs_audio_setFrequency(int argc, XsValue* args);
XsValue xs_audio_close(int argc, XsValue* args);
XsValue xs_audio_renderToWav(int argc, XsValue* args);

void xs_audio_register(VM* vm);

#endif /* XS_AUDIO_LIB_H */
