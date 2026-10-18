// AGENT RUN - tiny 8-bit synth: 3 music channels + SFX, ESP32 DAC (GPIO26) via DMA, own task on core 0
#pragma once
#include <stdint.h>
enum SongId : uint8_t { SONG_NONE, SONG_TITLE, SONG_GAME, SONG_OVER };
enum SfxId : uint8_t { SFX_NONE, SFX_GUN, SFX_BUMP, SFX_DROP, SFX_MISSILE, SFX_PICKUP, SFX_BOAT, SFX_EXPLODE, SFX_CRASH };
void audioBegin(uint8_t volume0to10, bool musicOn);
void audioSetSong(SongId s);
void audioSfx(SfxId s);
void audioSetVolume(uint8_t v);
void audioSetMusic(bool on);
// portable synth core (audio_synth.cpp)
void audioSynthInit(int sampleRate);
void audioSynthSetup(uint8_t volume0to10, bool musicOn);
void audioSynthRender(uint8_t* buf, int n);   // unsigned 8-bit, 128 = silence
bool audioWantAmp();                          // true when something audible is playing
