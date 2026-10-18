// AGENT RUN portable game core: logic + isometric world rendering.
// Shared by the ESP32 CYD build (main/sketch.cpp) and the Nintendo DS build (ds/source/main.cpp).
// Platform must provide: Canvas* G / int YO (game_gfx.h), audio.h functions, and the plat_* hooks below.
#pragma once
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include "game_gfx.h"
#include "audio.h"
using std::min; using std::max;

// ---- platform hooks
void plat_rumble(uint8_t strong, uint16_t ms);
void plat_led(int color /*0 r,1 g,2 b*/, float seconds);
void plat_saveHi(uint32_t hi);
void plat_log(const char* msg);

struct InState { float sx, sy; bool fire, special, start, sel, l1, r1, fireEdge, specialEdge, startEdge, selEdge, l1Edge, r1Edge; };
extern InState in;

// ------------------------------------------------------------ utils
static uint32_t rngState = 0x1234567;
static inline uint32_t rnd() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return rngState; }
static inline float frand(float lo, float hi) { return lo + (hi - lo) * (rnd() & 0xFFFF) / 65535.f; }
static inline uint32_t hash2(int x, int y, uint32_t s) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
}
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ------------------------------------------------------------ scenes
enum Deco : uint8_t { D_TREES, D_NEON, D_BRIDGE, D_DESERT, D_SNOW, D_CITY, D_COAST };
struct Scene {
  const char* name; Deco deco; bool outsideWater; bool boat;
  uint16_t ground, road, road2, shoulder, dash, edge, water, water2, hudBg, hudFg, hudAcc, lights;
};
static Scene SCENES[7];
static void initScenes() {
  SCENES[0] = {"HIGHWAY", D_TREES, false, false, RGB(56,148,56), RGB(96,96,104), RGB(88,88,96), RGB(200,184,120), RGB(248,248,248), RGB(248,220,64), 0, 0, RGB(16,16,24), TFT_WHITE, RGB(248,200,40), RGB(255,255,160)};
  SCENES[1] = {"NIGHT RUN", D_NEON, false, false, RGB(16,16,40), RGB(28,28,44), RGB(36,36,56), RGB(248,0,200), RGB(0,248,248), RGB(248,0,200), 0, 0, RGB(8,0,24), RGB(0,248,248), RGB(248,0,200), TFT_WHITE};
  SCENES[2] = {"RIVER BRIDGE", D_BRIDGE, true, false, RGB(32,96,200), RGB(128,120,112), RGB(116,108,100), RGB(168,160,150), TFT_WHITE, TFT_WHITE, RGB(32,96,200), RGB(96,168,240), RGB(16,16,24), TFT_WHITE, RGB(80,200,248), RGB(255,255,160)};
  SCENES[3] = {"CANYON SUNSET", D_DESERT, false, false, RGB(216,140,72), RGB(72,56,64), RGB(64,48,56), RGB(168,96,56), RGB(248,216,120), RGB(248,216,120), 0, 0, RGB(40,16,24), RGB(248,224,176), RGB(248,140,64), RGB(255,240,160)};
  SCENES[4] = {"SNOW PASS", D_SNOW, false, false, RGB(232,240,248), RGB(88,92,104), RGB(104,108,120), RGB(184,192,208), TFT_WHITE, RGB(248,200,64), 0, 0, RGB(16,24,40), TFT_WHITE, RGB(120,200,248), RGB(255,255,200)};
  SCENES[5] = {"DOWNTOWN", D_CITY, false, false, RGB(72,72,84), RGB(60,60,68), RGB(52,52,60), RGB(140,136,128), TFT_WHITE, RGB(248,220,64), 0, 0, RGB(16,16,24), TFT_WHITE, RGB(248,64,64), RGB(255,255,180)};
  SCENES[6] = {"COAST - BOAT MODE", D_COAST, false, true, RGB(216,192,152), RGB(40,64,128), RGB(48,76,144), RGB(64,104,88), RGB(232,128,120), RGB(216,192,152), RGB(40,64,128), RGB(232,128,120), RGB(24,16,40), RGB(248,232,200), RGB(232,128,120), RGB(255,240,180)};
}
static constexpr float SCENE_LEN = 420.f;   // tiles of travel per scene
static constexpr float ROAD_L = -3.f, ROAD_R = 4.f;

// ------------------------------------------------------------ entities
enum EType : uint8_t { E_NONE, E_CIV, E_RAMMER, E_GUNNER, E_VAN, E_OIL, E_SMOKE, E_BULLET, E_EBULLET,
                       E_MISSILE, E_BOMB, E_HELI, E_EXPL, E_EBOAT, E_TORPEDO };
struct Ent { EType t; float a, b, va, vb, z, timer, spin; uint16_t col; int8_t hp; };
static constexpr int MAX_ENT = 40;
static Ent ents[MAX_ENT];

enum Special : uint8_t { SP_NONE, SP_OIL, SP_SMOKE, SP_MISSILE, SP_TORPEDO };
static const char* SPNAME[] = {"MG", "OIL", "SMOKE", "MISSILE", "TORPEDO"};

struct Player { float a, b, speed, vb, spin, invuln, dead, fireCd; Special sp; int ammo; } pl;
enum GState { ST_TITLE, ST_PLAY, ST_PAUSE, ST_OVER };
static GState state = ST_TITLE;
static float camA = 0, fuel = 100, banner = 0, spawnT = 0, vanT = 20, heliT = 0, noScoreT = 0, tGame = 0;
static uint32_t score = 0, hiScore = 0, nextLife = 15000;
static int lives = 3, sceneIdx = 0;
static bool newHi = false;
static char bannerText[32];

static Scene& SC() { return SCENES[sceneIdx]; }
static Ent* spawn(EType t, float a, float b) {
  for (auto& e : ents) if (e.t == E_NONE) { memset(&e, 0, sizeof(e)); e.t = t; e.a = a; e.b = b; e.hp = 1; return &e; }
  return nullptr;
}
static void setBanner(const char* s, float t = 2.5f) { snprintf(bannerText, sizeof(bannerText), "%s", s); banner = t; }

// ------------------------------------------------------------ game logic
static void resetPlayer() {
  pl.b = 0.1f; pl.vb = 0; pl.speed = 5; pl.spin = 0; pl.invuln = 2.5f; pl.dead = 0; pl.fireCd = 0;
  for (auto& e : ents) if (e.t != E_NONE && fabsf(e.a - pl.a) < 8) e.t = E_NONE;
}
static void newGame() {
  memset(ents, 0, sizeof(ents));
  pl = {}; pl.a = 0; pl.sp = SP_NONE; pl.ammo = 0; resetPlayer();
  camA = 2; fuel = 100; score = 0; lives = 3; nextLife = 15000; sceneIdx = 0; spawnT = 1; vanT = 15; tGame = 0; newHi = false;
  setBanner(SC().name); state = ST_PLAY; audioSetSong(SONG_GAME);
}
static void explode(float a, float b) { if (Ent* e = spawn(E_EXPL, a, b)) e->timer = 0.6f; audioSfx(SFX_EXPLODE); }
static void killPlayer(const char* why) {
  if (pl.dead > 0 || pl.invuln > 0) return;
  explode(pl.a + .8f, pl.b + .4f); pl.dead = 1.6f; plat_rumble(255, 400); audioSfx(SFX_CRASH); plat_led(0, 0.35f);
  lives--; setBanner(why, 1.5f);
  { char m[64]; snprintf(m, sizeof(m), "[GAME] player lost a life: %s (lives=%d)", why, lives); plat_log(m); }
}
static void addScore(int pts) { if (noScoreT <= 0 || pts < 0) { if (pts < 0 && (int)score < -pts) score = 0; else score += pts; } }
static void destroy(Ent& e, bool byPlayer) {
  explode(e.a + .8f, e.b + .4f);
  if (byPlayer) {
    if (e.t == E_CIV) { addScore(-500); noScoreT = 4; setBanner("CIVILIAN HIT! -500", 1.5f); }
    else if (e.t == E_HELI) addScore(2500);
    else if (e.t == E_RAMMER || e.t == E_GUNNER || e.t == E_EBOAT) addScore(e.t == E_GUNNER ? 1000 : 750);
  }
  e.t = E_NONE;
}
static bool isCar(EType t) { return t == E_CIV || t == E_RAMMER || t == E_GUNNER || t == E_VAN || t == E_EBOAT; }
static bool overlap(float a1, float b1, float l1, float w1, float a2, float b2, float l2, float w2) {
  return a1 < a2 + l2 && a2 < a1 + l1 && b1 < b2 + w2 && b2 < b1 + w1;
}
static float laneB(int lane) { return -2.6f + lane * 1.75f; }  // 4 lanes, car width 0.8

static void spawnTraffic() {
  bool boat = SC().boat;
  int lane = rnd() % 4; float b = laneB(lane) + frand(0, .5f);
  uint32_t r = rnd() % 100;
  float diff = clampf(tGame / 240.f, 0, 1);   // difficulty ramps over 4 minutes
  if (boat) {
    if (Ent* e = spawn(E_EBOAT, camA + 21, b)) { e->va = frand(3, 5); e->col = RGB(60,60,60); e->hp = 2; }
    return;
  }
  if (r < 45 - diff * 15) {           // civilian ahead, slow
    if (Ent* e = spawn(E_CIV, camA + 21, b)) { e->va = frand(2.5f, 4); uint16_t cs[] = {RGB(240,200,40), RGB(40,120,220), RGB(64,140,80), RGB(232,232,240), RGB(240,140,40)}; e->col = cs[rnd() % 5]; }
  } else if (r < 75) {                 // rammer from behind
    if (Ent* e = spawn(E_RAMMER, camA - 17, b)) { e->va = pl.speed + frand(1.5f, 3); e->col = RGB(200,40,40); }
  } else {                              // gunner ahead
    if (Ent* e = spawn(E_GUNNER, camA + 21, b)) { e->va = 3; e->col = RGB(30,30,34); e->hp = 2; }
  }
}

static void fireWeapons(float dt) {
  pl.fireCd -= dt;
  if (in.fire && pl.fireCd <= 0) {
    pl.fireCd = 0.14f;
    EType bt = SC().boat ? E_BULLET : E_BULLET;
    if (Ent* e = spawn(bt, pl.a + 1.7f, pl.b + .35f)) { e->va = pl.speed + 14; e->timer = 0.9f; }
    audioSfx(SFX_GUN); plat_led(1, 0.04f);
  }
  if (in.specialEdge && pl.ammo > 0) {
    pl.ammo--;
    switch (pl.sp) {
      case SP_OIL:     if (Ent* e = spawn(E_OIL, pl.a - 1.4f, pl.b - .1f)) { e->timer = 8; e->col = 1; } break;
      case SP_SMOKE:   if (Ent* e = spawn(E_SMOKE, pl.a - 1.5f, pl.b - .2f)) { e->timer = 3.5f; e->va = pl.speed * .6f; } break;
      case SP_MISSILE: if (Ent* e = spawn(E_MISSILE, pl.a + 1.6f, pl.b + .3f)) { e->va = pl.speed + 10; e->timer = 2; e->z = 8; } break;
      case SP_TORPEDO: if (Ent* e = spawn(E_TORPEDO, pl.a + 1.8f, pl.b + .3f)) { e->va = pl.speed + 8; e->timer = 2.5f; } break;
      default: break;
    }
    audioSfx((pl.sp == SP_MISSILE || pl.sp == SP_TORPEDO) ? SFX_MISSILE : SFX_DROP);
    if (pl.ammo == 0) pl.sp = SP_NONE;
  }
}

static void update(float dt) {
  tGame += dt;
  if (noScoreT > 0) noScoreT -= dt;
  bool boat = SC().boat;

  // --- player
  if (pl.dead > 0) {
    pl.dead -= dt; pl.speed = 0;
    if (pl.dead <= 0) { if (lives <= 0) { state = ST_OVER; audioSetSong(SONG_OVER); if (score > hiScore) { hiScore = score; newHi = true; plat_saveHi(hiScore); } return; }
                        fuel = std::max(fuel, 60.f); resetPlayer(); }
  } else {
    if (pl.invuln > 0) pl.invuln -= dt;
    float target = 5.5f + in.sy * 3.5f;          // 2..9 tiles/s
    pl.speed += (target - pl.speed) * clampf(dt * 2.5f, 0, 1);
    float steer = in.sx * 4.2f;
    if (pl.spin > 0) { pl.spin -= dt; steer = sinf(tGame * 13) * 5; }
    pl.b += (steer + pl.vb) * dt;
    pl.vb *= powf(0.02f, dt);
    pl.a += pl.speed * dt;
    addScore((int)(pl.speed * dt * 10 + 0.5f));
    fuel -= dt * 1.4f;
    if (fuel <= 0) { fuel = 0; killPlayer("OUT OF FUEL"); }
    if (pl.b < ROAD_L - 0.7f || pl.b + .8f > ROAD_R + 0.7f) killPlayer(boat ? "RAN AGROUND" : "OFF ROAD!");
    fireWeapons(dt);
  }
  camA = pl.a + 2;
  if (score >= nextLife) { lives = min(lives + 1, 6); nextLife += 15000; setBanner("EXTRA LIFE!", 1.5f); }

  // --- scene progression
  int sIdx = ((int)(pl.a / SCENE_LEN)) % 7;
  if (sIdx != sceneIdx) {
    sceneIdx = sIdx; setBanner(SC().name); fuel = min(100.f, fuel + 40);
    if (SC().boat) { audioSfx(SFX_BOAT); plat_led(2, 0.3f); }
    for (auto& e : ents) if (isCar(e.t) || e.t == E_HELI) e.t = E_NONE;
    if (SC().boat) { if (pl.sp != SP_NONE) pl.sp = SP_TORPEDO; } else if (pl.sp == SP_TORPEDO) pl.sp = SP_MISSILE;
    heliT = 4;
  }

  // --- spawning
  spawnT -= dt;
  if (spawnT <= 0) { spawnTraffic(); spawnT = frand(0.9f, 2.0f) - clampf(tGame / 300.f, 0, .5f); }
  vanT -= dt;
  if (vanT <= 0 && !boat) {
    if (Ent* e = spawn(E_VAN, camA + 21, laneB(rnd() % 4))) { e->va = 3.5f; e->col = RGB(60,60,150); }
    vanT = frand(22, 32);
  }
  if (SC().deco == D_CITY) {
    bool have = false; for (auto& e : ents) if (e.t == E_HELI) have = true;
    heliT -= dt;
    if (!have && heliT <= 0) { if (Ent* e = spawn(E_HELI, camA + 18, pl.b)) { e->z = 70; e->hp = 1; e->timer = 2; } heliT = 12; }
  }

  // --- entities
  for (auto& e : ents) {
    if (e.t == E_NONE) continue;
    float ra = e.a - camA;
    if (e.t != E_EXPL && (ra < -22 || ra > 26)) { e.t = E_NONE; continue; }
    switch (e.t) {
      case E_CIV: case E_VAN: case E_EBOAT:
        if (e.t == E_EBOAT) { e.vb = (pl.b - e.b) * 0.6f; e.timer -= dt; if (e.timer <= 0 && fabsf(e.a - pl.a) < 6 && pl.dead <= 0) { e.timer = 1.6f; if (Ent* bl = spawn(E_EBULLET, e.a - .2f, e.b + .35f)) { bl->va = e.va - 9; bl->timer = 1.2f; } } }
        break;
      case E_RAMMER: {
        float d = pl.b - e.b;
        if (fabsf(e.a - pl.a) < 3) e.vb = clampf(d * 3, -3, 3); else e.vb = clampf(d, -1.5f, 1.5f);
        if (e.a > pl.a + 1.8f) e.va = max(e.va - dt * 2, pl.speed - .5f);
        break; }
      case E_GUNNER: {
        // hold position a few tiles ahead of the player, shoot backwards when lined up
        float want = pl.a + 6;
        e.va = pl.speed + clampf((want - e.a) * 0.8f, -3, 3);
        e.vb = clampf((pl.b - e.b) * 0.8f, -1.2f, 1.2f);
        e.timer -= dt;
        if (e.timer <= 0 && fabsf(pl.b - e.b) < 0.8f && pl.dead <= 0) { e.timer = 1.3f; if (Ent* bl = spawn(E_EBULLET, e.a - .2f, e.b + .35f)) { bl->va = e.va - 9; bl->timer = 1.4f; } }
        break; }
      case E_HELI: {
        e.va = pl.speed + clampf((pl.a + 5 - e.a), -2, 2);
        e.vb = clampf((pl.b - e.b) * 0.7f, -2, 2);
        e.timer -= dt;
        if (e.timer <= 0) { e.timer = 1.4f; if (Ent* bm = spawn(E_BOMB, e.a + .6f, e.b + .3f)) { bm->z = 60; bm->va = e.va * .3f; } }
        break; }
      case E_BOMB:
        e.z -= dt * 70;
        if (e.z <= 0) { explode(e.a, e.b); if (pl.dead <= 0 && overlap(e.a - .5f, e.b - .5f, 1, 1, pl.a, pl.b, 1.6f, .8f)) killPlayer("BOMBED!"); e.t = E_NONE; continue; }
        break;
      case E_EXPL: e.timer -= dt; if (e.timer <= 0) e.t = E_NONE; continue;
      case E_OIL: case E_SMOKE: e.timer -= dt; if (e.timer <= 0) { e.t = E_NONE; continue; } break;
      case E_BULLET: case E_EBULLET: case E_MISSILE: case E_TORPEDO:
        e.timer -= dt; if (e.timer <= 0) { e.t = E_NONE; continue; } break;
      default: break;
    }
    if (e.spin > 0) { e.spin -= dt; e.vb = sinf(tGame * 11 + e.a) * 4; }
    e.a += e.va * dt; e.b += e.vb * dt;
    if (isCar(e.t) && (e.b < ROAD_L - .8f || e.b + .8f > ROAD_R + .8f)) { destroy(e, e.t != E_CIV || e.spin <= 0); continue; }
  }

  // --- collisions
  for (auto& e : ents) {
    if (e.t == E_NONE) continue;
    // projectiles
    if (e.t == E_BULLET || e.t == E_MISSILE || e.t == E_TORPEDO) {
      for (auto& o : ents) {
        if (!isCar(o.t) && o.t != E_HELI) continue;
        if (o.t == E_HELI && e.t != E_MISSILE) continue;
        if (o.t == E_VAN) continue;
        float ob = o.b, oa = o.a;
        if (overlap(e.a, e.b - .1f, .4f, .3f, oa, ob, o.t == E_HELI ? 2 : 1.6f, o.t == E_HELI ? 1.1f : .8f)) {
          if (--o.hp <= 0 || e.t != E_BULLET) destroy(o, true);
          e.t = E_NONE; break;
        }
      }
      continue;
    }
    if (pl.dead > 0) continue;
    if (e.t == E_EBULLET && overlap(e.a, e.b - .1f, .3f, .3f, pl.a, pl.b, 1.6f, .8f)) { e.t = E_NONE; killPlayer("SHOT DOWN!"); continue; }
    if (e.t == E_OIL || e.t == E_SMOKE) {
      for (auto& o : ents) if ((o.t == E_RAMMER || o.t == E_GUNNER || o.t == E_CIV) && o.spin <= 0 && overlap(e.a, e.b, 1.2f, 1.1f, o.a, o.b, 1.6f, .8f)) { o.spin = 1.2f; addScore(100); }
      continue;
    }
    if (isCar(e.t) && overlap(e.a, e.b, 1.6f, .8f, pl.a, pl.b, 1.6f, .8f)) {
      if (e.t == E_VAN) {
        if (e.hp >= 0 && e.a > pl.a + .8f) {  // drove into the van's rear: weapon handoff
          pl.sp = SC().boat ? SP_TORPEDO : (Special)(SP_OIL + rnd() % 3); pl.ammo = 6; fuel = min(100.f, fuel + 30);
          char buf[32]; snprintf(buf, sizeof(buf), "GOT %s!", SPNAME[pl.sp]); setBanner(buf, 1.5f); addScore(300); plat_rumble(120, 120); audioSfx(SFX_PICKUP); plat_led(2, 0.3f);
          e.va = 7; e.hp = -1;  // used: speeds away
          e.a += 0.6f;
        }
        continue;
      }
      // bump: push apart sideways, dent speed
      float dir = (pl.b + .4f) < (e.b + .4f) ? -1.f : 1.f;
      pl.vb = dir * 6; e.vb = -dir * 6; e.spin = (e.t == E_CIV) ? 0.6f : 0.3f;
      if (e.a > pl.a) { pl.speed *= .7f; e.a += .3f; } else e.a -= .3f;
      plat_rumble(200, 150); audioSfx(SFX_BUMP);
      if (e.t == E_RAMMER) addScore(50);
    }
  }
}

// ------------------------------------------------------------ drawing
struct DItem { float depth; int16_t ymin, ymax; uint8_t kind; int16_t idx; float a, b; uint8_t sub; };
enum DK : uint8_t { K_ENT, K_PLAYER, K_TREE, K_PINE, K_CACTUS, K_MESA, K_BLD, K_LAMP, K_RAIL, K_PALM, K_SKY };
static constexpr int MAX_DI = 160;
static DItem dl[MAX_DI]; static int ndl = 0;
static void addDI(uint8_t kind, float a, float b, int16_t idx, float h, uint8_t sub = 0, float L = 1, float Wd = 1) {
  if (ndl >= MAX_DI) return;
  float ra = a - camA;
  SP p = P(ra, b), q = P(ra + L, b + Wd);
  int16_t ymin = min(p.y, q.y) - (int16_t)(L * TILE_HH) - (int16_t)(h * ZS) - 4, ymax = max(p.y, q.y) + (int16_t)(Wd * TILE_HH) + 6;
  if (ymax < 0 || ymin > PLAY_H) return;
  int16_t x0 = P(ra, b + Wd).x, x1 = P(ra + L, b).x;
  if (x1 < -20 || x0 > SCR_W + 20) return;
  dl[ndl++] = DItem{ (b + Wd * .5f) - (ra + L * .5f), ymin, ymax, kind, idx, a, b, sub };
}

static void buildDrawList() {
  ndl = 0;
  const Scene& s = SC();
  int a0 = (int)floorf(camA) - 17, a1 = (int)floorf(camA) + 22;
  uint32_t seed = sceneIdx * 77 + 3;
  for (int ai = a0; ai <= a1; ai++) {
    switch (s.deco) {
      case D_TREES:
        for (int b = -13; b <= 13; b++) { if (b > -6 && b < 6) continue; if (hash2(ai, b, seed) % 100 < 11) addDI(K_TREE, ai, b, 0, 20); } break;
      case D_SNOW:
        for (int b = -13; b <= 13; b++) { if (b > -6 && b < 6) continue; if (hash2(ai, b, seed) % 100 < 16) addDI(K_PINE, ai, b, 0, 26); } break;
      case D_DESERT:
        for (int b = -14; b <= 14; b++) {
          if (b > -6 && b < 6) continue;
          uint32_t h = hash2(ai, b, seed) % 1000;
          if (h < 70) addDI(K_CACTUS, ai, b, 0, 14);
          else if (h < 82 && (b <= -8 || b >= 9) && (ai & 1) == 0) addDI(K_MESA, ai, b, 0, 80, h & 3, 2, 2);
        } break;
      case D_NEON:
        if ((ai % 3) == 0) { addDI(K_LAMP, ai, -4.6f, 0, 26); addDI(K_LAMP, ai + 1, 5.4f, 0, 26); }
        for (int b = -14; b <= 14; b++) { if (b > -7 && b < 7) continue; if (hash2(ai, b, seed) % 100 < 7) addDI(K_BLD, ai, b, 0, 48, hash2(ai, b, 9) & 3, .9f, .9f); } break;
      case D_BRIDGE:
        if ((ai & 1) == 0) { addDI(K_RAIL, ai, -4.2f, 0, 8, 0, 1.8f, .12f); addDI(K_RAIL, ai, 4.8f, 0, 8, 0, 1.8f, .12f); } break;
      case D_CITY:
        if ((ai % 4) == 0) for (int b : {-9, -13, 7, 11}) if (hash2(ai, b, seed) % 100 < 70) addDI(K_SKY, ai, b, 0, 90, hash2(ai, b, 5) & 3, 3, 3);
        break;
      case D_COAST:
        for (int b = -12; b <= 12; b++) { if (b > -6 && b < 6) continue; if (hash2(ai, b, seed) % 100 < 6) addDI(K_PALM, ai, b, 0, 28); } break;
    }
  }
  for (int i = 0; i < MAX_ENT; i++) {
    const Ent& e = ents[i]; if (e.t == E_NONE) continue;
    float L = 1.6f, Wd = .8f, h = 20;
    if (e.t == E_VAN) { L = 2.6f; Wd = 1.1f; h = 24; }
    if (e.t == E_HELI) { L = 2; Wd = 1.1f; h = 110; }
    if (e.t == E_BOMB) h = e.z + 8;
    if (e.t == E_EXPL) { L = 1; Wd = 1; h = 24; }
    if (e.t == E_OIL || e.t == E_SMOKE) { L = 1.2f; Wd = 1.1f; h = e.t == E_SMOKE ? 20 : 2; }
    addDI(K_ENT, e.a - (e.t == E_EXPL ? .5f : 0), e.b - (e.t == E_EXPL ? .5f : 0), i, h, 0, L, Wd);
    if (e.t == E_OIL || e.t == E_SMOKE) dl[ndl - 1].depth -= 30;   // ground-level, draw first
    if (e.t == E_HELI || e.t == E_BOMB) dl[ndl - 1].depth += 60;   // airborne, draw last
  }
  if (pl.dead <= 0) addDI(K_PLAYER, pl.a, pl.b, -1, 20, 0, 1.6f, .8f);
  // insertion sort by depth (list is mostly ordered)
  for (int i = 1; i < ndl; i++) { DItem t = dl[i]; int j = i - 1; while (j >= 0 && dl[j].depth > t.depth) { dl[j + 1] = dl[j]; j--; } dl[j + 1] = t; }
}

static void drawCar(float ra, float b, uint16_t body, uint16_t lights, bool boatMode) {
  uint16_t win = RGB(40,60,90);
  if (boatMode) {
    for (int k = 0; k < 3; k++) { SP w0 = P(ra - .3f - k * .5f, b + .1f + k * .2f), w1 = P(ra - .3f - k * .5f, b + .7f - k * .2f); G->drawLine(w0.x, w0.y - YO, w1.x, w1.y - YO, RGB(220,240,255)); }
    isoBox(ra, b, 1.8f, .8f, 5, body); isoBox(ra + .5f, b + .15f, .7f, .5f, 5, win, 5);
    return;
  }
  flatQuad(ra + .12f, b + .12f, 1.6f, .8f, shade(SC().road, 150));
  isoBox(ra, b, 1.6f, .8f, 6, body);
  isoBox(ra + .35f, b + .1f, .8f, .6f, 5, win, 6);
  isoBox(ra + .45f, b + .15f, .5f, .5f, 1, body, 11, false);
  SP l1 = P(ra + 1.6f, b + .2f, 4), l2 = P(ra + 1.6f, b + .6f, 4);
  rect(l1.x - 1, l1.y - 1, 3, 3, lights); rect(l2.x - 1, l2.y - 1, 3, 3, lights);
}

static void drawItem(const DItem& d) {
  float ra = d.a - camA, b = d.b;
  const Scene& s = SC();
  switch (d.kind) {
    case K_TREE: isoBox(ra + .4f, b + .4f, .2f, .2f, 6, RGB(110,70,30)); isoBox(ra + .1f, b + .1f, .8f, .8f, 12, RGB(30,110,40), 6); break;
    case K_PINE: isoBox(ra + .42f, b + .42f, .16f, .16f, 5, RGB(96,64,40));
      isoBox(ra + .1f, b + .1f, .8f, .8f, 7, RGB(32,96,72), 4); isoBox(ra + .2f, b + .2f, .6f, .6f, 7, RGB(32,96,72), 11);
      isoBox(ra + .31f, b + .31f, .38f, .38f, 7, RGB(32,96,72), 18); isoBox(ra + .36f, b + .36f, .28f, .28f, 1, RGB(240,248,255), 25, false); break;
    case K_CACTUS: isoBox(ra + .45f, b + .45f, .15f, .15f, 12, RGB(48,120,64)); isoBox(ra + .25f, b + .45f, .15f, .15f, 5, RGB(48,120,64), 5); break;
    case K_MESA: { float h = 50 + d.sub * 10;
      quad(P(ra, b + 2), P(ra + 2, b + 2), P(ra + .5f, b + 4.5f), P(ra - 1.5f, b + 4.5f), RGB(176,104,64));
      isoBox(ra, b, 2, 2, h, RGB(176,80,56));
      for (int z = 10; z < h; z += 12) { SP p0 = P(ra, b + 2, z), p1 = P(ra + 2, b + 2, z); G->drawLine(p0.x, p0.y - YO, p1.x, p1.y - YO, RGB(136,56,48)); }
      break; }
    case K_LAMP: { isoBox(ra, b, .15f, .15f, 22, RGB(120,120,140)); SP p = P(ra, b, 24); G->fillEllipse(p.x, p.y - YO, 3, 2, RGB(255,240,160)); break; }
    case K_BLD: { float h = 20 + d.sub * 8; isoBox(ra, b, .9f, .9f, h, RGB(40,30,80));
      for (int z = 6; z < h - 2; z += 6) { SP p = P(ra + .3f, b + .9f, z); if (hash2((int)d.a, z, 3) & 1) rect(p.x, p.y - 2, 3, 2, (z & 4) ? RGB(248,220,80) : RGB(0,248,248)); }
      break; }
    case K_RAIL: isoBox(ra, b, 1.8f, .12f, 4, RGB(220,220,220)); isoBox(ra, b, .15f, .15f, 7, RGB(160,160,160)); break;
    case K_PALM: isoBox(ra + .45f, b + .45f, .12f, .12f, 22, RGB(120,88,56)); isoBox(ra + .1f, b + .1f, .8f, .8f, 3, RGB(40,120,72), 22); break;
    case K_SKY: { float h = 40 + d.sub * 16; isoBox(ra, b, 3, 3, h, RGB(96,104,128));
      for (int z = 8; z < h - 4; z += 8) for (int t = 0; t < 3; t++) { SP p = P(ra + .4f + t * .9f, b + 3, z); rect(p.x, p.y - 3, 5, 3, (hash2((int)d.a + t, z, 7) % 5 < 2) ? RGB(248,232,120) : RGB(56,64,88)); }
      break; }
    case K_PLAYER: {
      bool blink = pl.invuln > 0 && ((int)(pl.invuln * 10) & 1);
      if (!blink) drawCar(ra, b, s.boat ? RGB(232,232,240) : (s.deco == D_NEON ? RGB(0,220,248) : RGB(232,232,240)), s.lights, s.boat);
      break; }
    case K_ENT: {
      const Ent& e = ents[d.idx];
      switch (e.t) {
        case E_CIV: case E_RAMMER: case E_GUNNER: drawCar(ra, b, e.col, s.lights, false);
          if (e.t == E_GUNNER) { SP g = P(ra + .2f, b + .4f, 12); rect(g.x - 3, g.y - 1, 6, 2, RGB(160,160,160)); } break;
        case E_EBOAT: drawCar(ra, b, e.col, s.lights, true); { SP c = P(ra + .7f, b + .3f, 10); rect(c.x - 3, c.y - 3, 6, 4, RGB(200,30,30)); } break;
        case E_VAN: { isoBox(ra, b, 2.6f, 1.1f, 16, e.col); SP p = P(ra + .5f, b + .3f, 12);
          G->setTextFont(1); G->setTextSize(1); G->setTextColor(TFT_WHITE); G->setTextDatum(TL_DATUM); G->drawString("W", p.x, p.y - YO);
          SP r0 = P(ra, b + .2f, 3), r1 = P(ra, b + .9f, 3); G->drawLine(r0.x, r0.y - YO, r1.x, r1.y - YO, RGB(248,220,64)); break; }
        case E_OIL: { SP c = P(ra + .6f, b + .55f); G->fillEllipse(c.x, c.y - YO, 14, 6, RGB(20,20,28)); G->fillEllipse(c.x - 3, c.y - 1 - YO, 4, 2, RGB(70,50,110)); break; }
        case E_SMOKE: { SP c = P(ra + .6f, b + .55f, 8); int r = 8 + (int)((3.5f - e.timer) * 3);
          G->fillCircle(c.x, c.y - YO, r, RGB(200,200,210)); G->fillCircle(c.x + r / 2, c.y - 4 - YO, r * 2 / 3, RGB(230,230,236)); break; }
        case E_BULLET: { SP p = P(ra, b, 4); rect(p.x - 1, p.y - 1, 3, 3, RGB(255,240,80)); break; }
        case E_EBULLET: { SP p = P(ra, b, 4); rect(p.x - 1, p.y - 1, 3, 3, RGB(255,80,80)); break; }
        case E_MISSILE: { SP p = P(ra, b, e.z), q = P(ra - .6f, b, e.z); G->drawLine(q.x, q.y - YO, p.x, p.y - YO, RGB(220,220,220)); G->drawLine(q.x, q.y + 1 - YO, p.x, p.y + 1 - YO, RGB(248,140,40)); break; }
        case E_TORPEDO: { SP p = P(ra, b), q = P(ra - 1, b); G->drawLine(q.x, q.y - YO, p.x, p.y - YO, RGB(220,240,255)); rect(p.x - 2, p.y - 1, 4, 3, RGB(40,40,40)); break; }
        case E_BOMB: { SP sh = P(ra, b); G->fillEllipse(sh.x, sh.y - YO, 4, 2, shade(s.road, 140)); SP p = P(ra, b, e.z); G->fillCircle(p.x, p.y - YO, 3, TFT_BLACK); break; }
        case E_HELI: { SP sh = P(ra + 1, b + .55f); G->fillEllipse(sh.x, sh.y - YO, 30, 12, shade(s.road, 140));
          isoBox(ra, b, 2, 1.1f, 14, RGB(56,88,48), e.z); isoBox(ra + .7f, b + .1f, .9f, .9f, 6, RGB(120,180,220), e.z + 6);
          isoBox(ra - 1.6f, b + .4f, 1.6f, .3f, 5, RGB(56,88,48), e.z + 6);
          SP r = P(ra + 1, b + .55f, e.z + 22); float ang = tGame * 25; int dx = (int)(cosf(ang) * 44), dy = (int)(sinf(ang) * 16);
          G->drawLine(r.x - dx, r.y - dy - YO, r.x + dx, r.y + dy - YO, RGB(220,220,220)); G->drawLine(r.x + dy * 2, r.y - dx / 3 - YO, r.x - dy * 2, r.y + dx / 3 - YO, RGB(170,170,170));
          break; }
        case E_EXPL: { SP c = P(ra + .5f, b + .5f, 6); float t = 1 - e.timer / .6f; int r = 4 + (int)(t * 14);
          G->fillCircle(c.x, c.y - YO, r, RGB(248,136,32)); G->fillCircle(c.x, c.y - YO, r * 2 / 3, RGB(255,240,120)); break; }
        default: break;
      }
      break; }
  }
}

// ground: per-row horizontal spans of constant-b bands (road scrolls along a, so bands are static on screen)
static void drawGround(int y0, int y1) {
  const Scene& s = SC();
  uint16_t outside = s.outsideWater ? s.water : s.ground;
  uint16_t shoulderC = s.shoulder, roadC = s.road;
  if (s.boat) outside = s.ground;      // beach banks either side of the channel
  const float bnd[6] = {ROAD_L - 1, ROAD_L, ROAD_L + .12f, ROAD_R - .12f, ROAD_R, ROAD_R + 1};
  const uint16_t col[7] = {outside, shoulderC, s.edge, roadC, s.edge, shoulderC, outside};
  for (int y = y0; y < y1; y++) {
    int prev = 0;
    for (int i = 0; i <= 6; i++) {
      int x = (i < 6) ? (int)(ISO_OX + 2 * TILE_HW * bnd[i] - 2 * (y - ISO_OY)) : SCR_W;
      if (x > SCR_W) x = SCR_W;
      if (x > prev) { G->drawFastHLine(prev, y - YO, x - prev, col[i]); prev = x; }
      if (prev >= SCR_W) break;
    }
  }
  // scrolling detail: lane dashes, cracks, water glints, grass tufts
  int a0 = (int)floorf(camA) - 18, a1 = (int)floorf(camA) + 22;
  for (int ai = a0; ai <= a1; ai++) {
    float ra = ai - camA;
    if ((ai & 1) == 0 && !s.boat) for (float lb : {laneB(1) - .25f, laneB(2) - .25f, laneB(3) - .25f}) {
      SP p = P(ra, lb), q = P(ra + 1, lb);
      if (max(p.y, q.y) < y0 - 2 || min(p.y, q.y) > y1 + 2) continue;
      G->drawLine(p.x, p.y - YO, q.x, q.y - YO, s.dash); G->drawLine(p.x, p.y + 1 - YO, q.x, q.y + 1 - YO, s.dash);
    }
    uint32_t h = hash2(ai, 1, 99);
    // road detail (cracks / water ripples in boat mode)
    if (h % 3 == 0) { float bb = ROAD_L + .4f + (h >> 8) % 60 / 10.f; SP p = P(ra + .3f, bb);
      if (p.y >= y0 - 4 && p.y < y1 + 4) G->drawLine(p.x, p.y - YO, p.x + 5, p.y - 2 - YO, s.road2); }
    // outside detail
    for (int b = -14; b <= 14; b += 3) {
      if (b > ROAD_L - 2 && b < ROAD_R + 1) continue;
      uint32_t k = hash2(ai, b, 41);
      if (k % 5) continue;
      SP p = P(ra + .5f, b + .5f);
      if (p.y < y0 - 4 || p.y >= y1 + 4 || p.x < -10 || p.x > SCR_W + 10) continue;
      if (s.outsideWater) G->fillTriangle(p.x - 6, p.y - YO, p.x, p.y - 3 - YO, p.x + 6, p.y - YO, s.water2), G->fillTriangle(p.x - 6, p.y - YO, p.x, p.y + 3 - YO, p.x + 6, p.y - YO, s.water2);
      else rect(p.x, p.y - 1, 2, 2, shade(outside, 200));
    }
  }
}

