// AGENT RUN portable synth (ESP32 DAC backend: audio_esp32.cpp, DS backend: ds/source/main.cpp). All music here is original (spy-jazz flavoured: minor key, walking bass, chromatic approaches).
#include "audio.h"
#include <algorithm>
using std::min;
#include <math.h>

static int SR = 22050;

// ---------------- songs: 0 = rest/off, 1 = hold, else MIDI note. drums: 1 hat, 2 snare, 3 kick
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
  {T_LEAD, T_BASS, T_DRUM, 32, 96, true, 64},    // title: slow, lazy swing, A minor
  {G_LEAD, G_BASS, G_DRUM, 64, 152, true, 60},   // in-game: driving, E minor
  {O_LEAD, O_BASS, O_DRUM, 16, 84, false, 50},   // game over: descending, plays once
};

// ---------------- state shared with game thread
static volatile uint8_t reqSong = SONG_NONE, curSongId = 0xFF;
static volatile uint8_t reqSfx = SFX_NONE;
static volatile uint8_t volume = 6;
static volatile bool musicOn = true;

static uint32_t noteInc[128];
static inline uint32_t incFor(float hz) { return (uint32_t)(hz * 4294967296.0 / SR); }

struct Ch { uint32_t ph = 0, inc = 0; int env = 0; };
static Ch lead, bass;
static int drumEnv = 0; static uint8_t drumType = 0;
static uint32_t lfsr = 0xACE1u;
static inline int noise() { lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5; return (int)(lfsr & 0xFF) - 128; }

static const Song* song = &SONGS[0];
static int step = 0, stepLeft = 0; static bool songDone = true;

static uint8_t sfxId = SFX_NONE; static uint32_t sfxT = 0, sfxLen = 0, sfxPh = 0; static int sfxHold = 0;
static const uint8_t SFX_PRIO[] = {0, 1, 2, 2, 3, 4, 4, 5, 6};

static int stepSamples(int i) {
  int eighth = SR * 60 / song->bpm / 2;
  int first = eighth * 2 * song->swing / 100;
  return (i & 1) ? eighth * 2 - first : first;
}
static void startSong(uint8_t id) {
  song = &SONGS[id]; step = 0; stepLeft = 0; songDone = (song->len == 0); lead.env = bass.env = drumEnv = 0; curSongId = id;
}
static void advanceStep() {
  if (step >= song->len) { if (song->loop) step = 0; else { songDone = true; lead.env = bass.env = 0; return; } }
  uint8_t l = song->lead[step], b = song->bass[step], d = song->drum[step & 7];
  if (l == 0) lead.env = 0; else if (l > 1) { lead.inc = noteInc[l]; lead.env = 255; }
  if (b == 0) bass.env = 0; else if (b > 1) { bass.inc = noteInc[b + 12]; bass.env = 255; }   // +1 octave: tiny speaker
  if (d) { drumType = d; drumEnv = 255; }
  stepLeft = stepSamples(step); step++;
}

static int renderSfx() {
  if (sfxId == SFX_NONE) return 0;
  if (sfxT >= sfxLen) { sfxId = SFX_NONE; return 0; }
  float t = (float)sfxT / sfxLen; int env = (int)(255 * (1 - t)); int s = 0;
  switch (sfxId) {
    case SFX_GUN:     sfxPh += incFor(1400 - 900 * t); s = ((sfxPh >> 31) ? 60 : -60) + noise() / 3; break;
    case SFX_BUMP:    sfxPh += incFor(110 - 40 * t); s = (sfxPh >> 31) ? 70 : -70; break;
    case SFX_DROP:    sfxPh += incFor(500 - 350 * t); s = ((sfxPh >> 31) ? 40 : -40) + noise() / 4; break;
    case SFX_MISSILE: sfxPh += incFor(300 + 1400 * t); s = ((sfxPh >> 30) & 1 ? 50 : -50) + noise() / 6; env = 200; if (t > .8f) env = (int)(1000 * (1 - t)); break;
    case SFX_PICKUP: { static const uint8_t arp[] = {72, 76, 79, 84}; sfxPh += noteInc[arp[min(3, (int)(t * 4))]]; s = (sfxPh >> 31) ? 55 : -55; env = 220; break; }
    case SFX_BOAT:    sfxPh += incFor(((sfxT / (SR / 8)) & 1) ? 330 : 220); s = (sfxPh >> 31) ? 60 : -60; env = 200; break;
    case SFX_EXPLODE: if (--sfxHold <= 0) { sfxHold = 1 + (int)(t * 12); sfxPh = noise(); } s = (int)(int32_t)sfxPh; break;
    case SFX_CRASH:   if (--sfxHold <= 0) { sfxHold = 1 + (int)(t * 16); sfxPh = noise(); } s = (int)(int32_t)sfxPh;
                      { static uint32_t p2; p2 += incFor(180 - 140 * t); s = s * 2 / 3 + ((p2 >> 31) ? 40 : -40); } break;
    default: break;
  }
  sfxT++;
  return s * env / 255;
}

static volatile bool lastWantAmp = false;
bool audioWantAmp() { return lastWantAmp; }
void audioSynthInit(int sr) {
  SR = sr;
  for (int n = 0; n < 128; n++) noteInc[n] = incFor(440.f * powf(2.f, (n - 69) / 12.f));
}
// Render n unsigned 8-bit samples (128 = silence)
void audioSynthRender(uint8_t* buf, int n) {
    if (reqSong != curSongId) startSong(reqSong);
    uint8_t rq = reqSfx;
    if (rq != SFX_NONE) {
      reqSfx = SFX_NONE;
      if (sfxId == SFX_NONE || SFX_PRIO[rq] >= SFX_PRIO[sfxId]) {
        static const uint16_t LEN_MS[] = {0, 45, 90, 160, 320, 300, 500, 550, 850};
        sfxId = rq; sfxT = 0; sfxLen = (uint32_t)LEN_MS[rq] * SR / 1000; sfxPh = 0; sfxHold = 0;
      }
    }
    int vol = volume; bool mus = musicOn;
    bool wantAmp = vol > 0 && ((mus && !songDone) || sfxId != SFX_NONE);
    lastWantAmp = wantAmp;
    for (int i = 0; i < n; i++) {
      int m = 0;
      if (!songDone) {
        if (--stepLeft <= 0) advanceStep();
        if (lead.env) { lead.ph += lead.inc; m += ((lead.ph >> 30) == 0 ? 34 : -11) * lead.env / 255;   // 25% pulse
                        if ((i & 31) == 0 && lead.env > 90) lead.env--; }
        if (bass.env) { bass.ph += bass.inc; int t = bass.ph >> 24; int tri = t < 128 ? t * 2 - 128 : 383 - t * 2;
                        m += tri * 40 / 128 * bass.env / 255; if ((i & 63) == 0 && bass.env > 120) bass.env--; }
        if (drumEnv) { int n = noise();
          if (drumType == 3) { bass.ph += 0; m += ((drumEnv & 16) ? 30 : -30) * drumEnv / 255; drumEnv -= 6; }
          else { m += n * (drumType == 2 ? 26 : 10) / 128 * drumEnv / 255; drumEnv -= (drumType == 2 ? 3 : 12); }
          if (drumEnv < 0) drumEnv = 0; }
        if (!mus) m = 0;
      }
      int s = m + renderSfx();
      s = s * vol / 12;
      s += 128; buf[i] = (uint8_t)(s < 0 ? 0 : (s > 255 ? 255 : s));
    }
}
void audioSetSong(SongId s) { reqSong = s; }
void audioSfx(SfxId s) { if (SFX_PRIO[s] >= SFX_PRIO[reqSfx]) reqSfx = s; }
void audioSetVolume(uint8_t v) { volume = v > 10 ? 10 : v; }
void audioSetMusic(bool on) { musicOn = on; }
void audioSynthSetup(uint8_t v, bool m) { volume = v; musicOn = m; }
