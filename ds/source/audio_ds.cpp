// AGENT RUN (DS) audio: same original songs/SFX as the ESP32 synth, but voiced on hardware channels.
// The ARM9 only runs a step sequencer + envelopes a few times per frame (no per-sample work).
#include <nds.h>
#include <math.h>
#include "audio.h"

// ---------------- songs (unchanged): 0 = rest/off, 1 = hold, else MIDI note. drums: 1 hat, 2 snare, 3 kick
static const uint8_t G_LEAD[64] = {
  71,1,1,76, 79,1,78,76,   75,1,76,1, 71,1,1,0,   72,1,76,81, 79,1,76,72,   75,78,81,1, 83,1,1,0,
  79,78,76,71, 1,1,76,0,   79,1,82,1, 83,1,79,76, 81,1,79,76, 72,1,69,1,   71,72,75,78, 76,1,1,0 };
static const uint8_t G_BASS[64] = {
  40,1,43,1,45,1,46,1,  47,1,50,1,49,1,48,1,  45,1,48,1,52,1,51,1,  47,1,45,1,42,1,39,1,
  40,1,35,1,38,1,39,1,  48,1,47,1,46,1,45,1,  45,1,43,1,42,1,41,1,  47,1,45,1,42,1,39,1 };
static const uint8_t G_DRUM[8] = {3,1,2,1,3,1,2,1};
static const uint8_t T_LEAD[32] = { 76,1,1,1,1,1,0,0,  77,1,76,1,74,1,1,0,  72,1,1,1,69,1,0,0,  71,1,68,1,64,1,1,0 };
static const uint8_t T_BASS[32] = { 45,1,48,1,52,1,51,1, 50,1,53,1,52,1,47,1, 45,1,43,1,41,1,40,1, 44,1,47,1,52,1,44,1 };
static const uint8_t T_DRUM[8] = {3,1,0,1,0,1,0,1};
static const uint8_t O_LEAD[16] = { 76,1,75,1,74,1,73,1,72,1,1,1,1,1,0,0 };
static const uint8_t O_BASS[16] = { 45,1,44,1,43,1,42,1,40,1,1,1,1,1,0,0 };
static const uint8_t O_DRUM[8] = {0,0,0,0,0,0,0,0};
struct Song { const uint8_t *lead, *bass, *drum; uint16_t len; uint16_t bpm; bool loop; uint8_t swing; };
static const Song SONGS[] = {
  {nullptr, nullptr, nullptr, 0, 120, false, 50},
  {T_LEAD, T_BASS, T_DRUM, 32, 96, true, 64},
  {G_LEAD, G_BASS, G_DRUM, 64, 152, true, 60},
  {O_LEAD, O_BASS, O_DRUM, 16, 84, false, 50},
};

enum { CH_BASS = 0, CH_LEAD = 8, CH_KICK = 9, CH_SFXSQ = 10, CH_DRUM = 14, CH_SFXN = 15 };
static int8_t tri[32] __attribute__((aligned(32)));
static uint32_t noteHz16[128];            // Hz * 16
static uint8_t volume = 6; static bool musicOn = true;
static uint8_t reqSong = SONG_NONE, curSong = 0xFF, reqSfx = SFX_NONE;
static const Song* song = &SONGS[0];
static int step = 0; static int32_t stepLeft = 0; static bool songDone = true;
static int32_t leadEnv = 0, bassEnv = 0, drumT = 0; static uint8_t drumType = 0;   // env 8.8 fixed, drumT in us
static uint8_t sfxId = SFX_NONE; static int32_t sfxT = 0, sfxLen = 0;            // us
static uint32_t lastTick = 0;
static const uint8_t SFX_PRIO[] = {0, 1, 2, 2, 3, 4, 4, 5, 6};
static const uint16_t LEN_MS[] = {0, 45, 90, 160, 320, 300, 500, 550, 850};
static int lastV[16];

static inline unsigned psgTimer(uint32_t hz16) { if (hz16 < 16) hz16 = 16; return soundTimerFromHz((hz16 * 8 + 8) >> 4); }
static inline int scaleV(int v) { int r = v * volume / 10; return r > 2047 ? 2047 : r; }
static void setVol(int ch, int v) { if (v != lastV[ch]) { soundChSetVolume(ch, v); lastV[ch] = v; } }
static void playSq(int ch, uint32_t hz16, int v, SoundDuty d) { soundPreparePsg(ch | SOUND_START, v, 64, psgTimer(hz16), d); lastV[ch] = v; }
static void playNoise(int ch, uint32_t clockHz, int v) { soundPreparePsg(ch | SOUND_START, v, 64, soundTimerFromHz(clockHz), SoundDuty_50); lastV[ch] = v; }
static void stopCh(int ch) { soundStop(1u << ch); lastV[ch] = -1; }

static int32_t stepTicks(int i) {   // bus-clock ticks
  int32_t eighth = (int32_t)(BUS_CLOCK / song->bpm * 60 / 2);
  int32_t first = eighth / 100 * 2 * song->swing;
  return (i & 1) ? eighth * 2 - first : first;
}
static void startSong(uint8_t id) {
  song = &SONGS[id]; step = 0; stepLeft = 0; songDone = (song->len == 0); curSong = id;
  leadEnv = bassEnv = 0; stopCh(CH_LEAD); stopCh(CH_BASS);
}
static void advanceStep() {
  if (step >= song->len) {
    if (song->loop) step = 0;
    else { songDone = true; leadEnv = bassEnv = 0; stopCh(CH_LEAD); stopCh(CH_BASS); return; }
  }
  uint8_t l = song->lead[step], b = song->bass[step], d = song->drum[step & 7];
  int mv = musicOn ? 1 : 0;
  if (l == 0) { leadEnv = 0; stopCh(CH_LEAD); }
  else if (l > 1) { leadEnv = 255 << 8; playSq(CH_LEAD, noteHz16[l], scaleV(560) * mv, SoundDuty_25); }
  if (b == 0) { bassEnv = 0; stopCh(CH_BASS); }
  else if (b > 1) {
    bassEnv = 255 << 8;
    uint32_t hz = noteHz16[b + 12] * 32 / 16;     // 32-sample triangle, +1 octave like the original
    soundPreparePcm(CH_BASS | SOUND_START, scaleV(1100) * mv, 64, soundTimerFromHz(hz), SoundMode_Repeat, SoundFmt_Pcm8, tri, 0, 8);
    lastV[CH_BASS] = scaleV(1100) * mv;
  }
  if (d && musicOn) {
    drumType = d; drumT = 0;
    if (d == 3) playSq(CH_KICK, 150 * 16, scaleV(900), SoundDuty_50);
    else playNoise(CH_DRUM, d == 2 ? 16384 : 30000, scaleV(d == 2 ? 750 : 260));
  }
  stepLeft += stepTicks(step); step++;
}

static void sfxStart(uint8_t id) {
  sfxId = id; sfxT = 0; sfxLen = LEN_MS[id] * 1000;
  stopCh(CH_SFXSQ); stopCh(CH_SFXN);
  if (id == SFX_GUN || id == SFX_DROP || id == SFX_EXPLODE || id == SFX_CRASH) playNoise(CH_SFXN, 16384, 0);
  if (id != SFX_EXPLODE) playSq(CH_SFXSQ, 440 * 16, 0, SoundDuty_50);
}
static void sfxUpdate() {
  if (sfxId == SFX_NONE) return;
  if (sfxT >= sfxLen) { sfxId = SFX_NONE; stopCh(CH_SFXSQ); stopCh(CH_SFXN); return; }
  int t256 = (int)((int64_t)sfxT * 256 / sfxLen);              // 0..255
  int env = 255 - t256, sq = 0, nz = 0; uint32_t f16 = 0, nclk = 16384;
  switch (sfxId) {
    case SFX_GUN:     f16 = (1400 - 900 * t256 / 256) * 16; sq = 60; nz = 40; break;
    case SFX_BUMP:    f16 = (110 - 40 * t256 / 256) * 16; sq = 70; break;
    case SFX_DROP:    f16 = (500 - 350 * t256 / 256) * 16; sq = 40; nz = 30; break;
    case SFX_MISSILE: f16 = (600 + 2800 * t256 / 256) * 16; sq = 50; nz = 20; env = t256 > 204 ? (255 - t256) * 4 : 200; break;
    case SFX_PICKUP: { static const uint8_t arp[] = {72, 76, 79, 84}; f16 = noteHz16[arp[t256 >> 6]]; sq = 55; env = 220; break; }
    case SFX_BOAT:    f16 = ((sfxT / 125000) & 1) ? 330 * 16 : 220 * 16; sq = 60; env = 200; break;
    case SFX_EXPLODE: nz = 128; nclk = 16384 * 256 / (256 + 12 * t256); break;
    case SFX_CRASH:   nz = 85; nclk = 16384 * 256 / (256 + 16 * t256); f16 = (180 - 140 * t256 / 256) * 16; sq = 40; break;
    default: break;
  }
  if (sq) { soundChSetTimer(CH_SFXSQ, psgTimer(f16)); setVol(CH_SFXSQ, scaleV(sq * 14 * env / 255)); }
  if (nz) { soundChSetTimer(CH_SFXN, soundTimerFromHz(nclk)); setVol(CH_SFXN, scaleV(nz * 12 * env / 255)); }
}

void audioBegin(uint8_t v, bool m) {
  volume = v; musicOn = m;
  int c0 = soundPlayPSG(DutyCycle_0, 440, 0, 64);   // libnds path performs the sound driver init handshake
  soundKill(c0); soundEnable();
  for (int n = 0; n < 128; n++) noteHz16[n] = (uint32_t)(16.f * 440.f * powf(2.f, (n - 69) / 12.f));
  for (int i = 0; i < 32; i++) tri[i] = (int8_t)(i < 16 ? -127 + i * 16 : 127 - (i - 16) * 16);
  DC_FlushRange(tri, sizeof(tri));
  for (int i = 0; i < 16; i++) lastV[i] = -1;
  lastTick = cpuGetTiming();
}
void audioTick() {
  uint32_t now = cpuGetTiming(), el = now - lastTick; lastTick = now;
  if (el > BUS_CLOCK / 8) el = BUS_CLOCK / 8;
  int32_t us = (int32_t)(el / 34);                           // ~33.5 bus ticks per microsecond
  if (reqSong != curSong) startSong(reqSong);
  if (reqSfx != SFX_NONE) { uint8_t r = reqSfx; reqSfx = SFX_NONE; if (sfxId == SFX_NONE || SFX_PRIO[r] >= SFX_PRIO[sfxId]) sfxStart(r); }
  if (!songDone) {
    stepLeft -= (int32_t)el;
    int guard = 0;
    while (stepLeft <= 0 && !songDone && guard++ < 8) advanceStep();
    int mv = musicOn ? 1 : 0;
    // envelopes (same decay rates as the PCM synth: lead 1/32 samples to 90, bass 1/64 samples to 120 @16364Hz)
    if (leadEnv) { leadEnv -= us * 131 / 1000; if (leadEnv < (90 << 8)) leadEnv = 90 << 8; setVol(CH_LEAD, scaleV(560 * (leadEnv >> 8) / 255) * mv); }
    if (bassEnv) { bassEnv -= us * 65 / 1000; if (bassEnv < (120 << 8)) bassEnv = 120 << 8; setVol(CH_BASS, scaleV(1100 * (bassEnv >> 8) / 255) * mv); }
  }
  if (drumType) {
    drumT += us;
    int len = drumType == 3 ? 30000 : drumType == 2 ? 45000 : 15000;
    if (drumT >= len) { stopCh(drumType == 3 ? CH_KICK : CH_DRUM); drumType = 0; }
    else if (drumType == 3) { soundChSetTimer(CH_KICK, psgTimer((150 - 100 * drumT / len) * 16)); setVol(CH_KICK, scaleV(900 * (len - drumT) / len)); }
    else setVol(CH_DRUM, scaleV((drumType == 2 ? 750 : 260) * (len - drumT) / len));
  }
  sfxT += us;
  sfxUpdate();
}
void audioSetSong(SongId s) { reqSong = s; }
void audioSfx(SfxId s) { if (SFX_PRIO[s] >= SFX_PRIO[reqSfx]) reqSfx = s; }
void audioSetVolume(uint8_t v) { volume = v > 10 ? 10 : v; }
void audioSetMusic(bool on) { musicOn = on; if (!on) { stopCh(CH_LEAD); stopCh(CH_BASS); } }
