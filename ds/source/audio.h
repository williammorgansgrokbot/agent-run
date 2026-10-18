// AGENT RUN (DS) - music + SFX on the DS hardware sound channels (PSG square/noise + tiny looped PCM)
#pragma once
#include <stdint.h>
enum SongId : uint8_t { SONG_NONE, SONG_TITLE, SONG_GAME, SONG_OVER };
enum SfxId : uint8_t { SFX_NONE, SFX_GUN, SFX_BUMP, SFX_DROP, SFX_MISSILE, SFX_PICKUP, SFX_BOAT, SFX_EXPLODE, SFX_CRASH };
void audioBegin(uint8_t volume0to10, bool musicOn);
void audioTick();                 // call a few times per frame; cheap (a handful of register messages)
void audioSetSong(SongId s);
void audioSfx(SfxId s);
void audioSetVolume(uint8_t v);
void audioSetMusic(bool on);
