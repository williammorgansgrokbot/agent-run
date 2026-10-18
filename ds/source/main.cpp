// AGENT RUN - Nintendo DS port, hardware-accelerated renderer.
//  Top screen (main engine, mode 5):
//    BG3  8bpp 512x256 wrapping bitmap = ground + scenery. Scrolled by hardware; each frame only the
//         newly exposed 1-4px edge strips are rendered (into a RAM shadow, then copied to VRAM).
//    OBJ  cars, bullets, heli, explosions... = 8bpp hardware sprites pre-rendered per scene.
//    BG0  8bpp tiled text overlay (title / banner / game over), redrawn only when its text changes.
//  Bottom screen (sub engine): 16bpp bitmap HUD, dynamic fields redrawn every other frame.
//  Audio: hardware PSG/noise/PCM channels sequenced by audio_ds.cpp (no software synthesis).
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "game_core.h"

Canvas* G = nullptr;
int YO = 0, camS = 0, gOX = ISO_OX, gOY = ISO_OY;
InState in;

// ---------------------------------------------------------------- state
static uint8_t shadow[512 * 256] __attribute__((aligned(32)));   // BG3 contents (wrapped)
static uint8_t scratch[64 * 64] __attribute__((aligned(32)));    // sprite pre-render
static uint8_t overlay[256 * 192] __attribute__((aligned(32)));  // BG0 overlay, linear
static ColorMap bgMap, objMap;
static Canvas bgC, sprC, ovC, hudC;
static int bg3;
static uint16_t* bg0Tiles; static uint16_t* bg0Map;
static int lastS = 0, viewScene = -1; static bool needFull = true;
static int renderedS = 0, jobR = -1, jobPart = 0;
static uint8_t volume = 7; static bool musicOn = true;
static char savePath[40] = "";
static float shakeT = 0; static int flashT = 0;
static uint32_t vblanks = 0;   // derived from the CPU timer (59.8261 Hz frames)
static uint32_t tBase = 0;
static void updVbl() { vblanks = (uint32_t)((uint64_t)(cpuGetTiming() - tBase) * 598261 / ((uint64_t)BUS_CLOCK * 10000)); }

// ---------------------------------------------------------------- platform hooks
void plat_rumble(uint8_t strong, uint16_t ms) { shakeT = ms / 1000.f; if (strong > 150) flashT = 6; }
void plat_led(int, float) {}
void plat_log(const char*) {}
static void saveAll() {
  if (!savePath[0]) return;
  FILE* f = fopen(savePath, "wb");
  if (!f) return;
  uint8_t d[8] = {'A', 'R', 1, volume, (uint8_t)musicOn, 0, 0, 0};
  fwrite(d, 1, 8, f); fwrite(&hiScore, 4, 1, f); fclose(f);
}
void plat_saveHi(uint32_t) { saveAll(); }
static void initSave() {
  if (!fatInitDefault()) return;
  const char* order[2] = { isDSiMode() ? "sd:/agentrun.sav" : "fat:/agentrun.sav", isDSiMode() ? "fat:/agentrun.sav" : "sd:/agentrun.sav" };
  for (const char* p : order) {
    FILE* f = fopen(p, "rb");
    if (f) {
      uint8_t d[8];
      if (fread(d, 1, 8, f) == 8 && d[0] == 'A' && d[1] == 'R') { volume = d[3] > 10 ? 7 : d[3]; musicOn = d[4]; if (fread(&hiScore, 4, 1, f) != 1) hiScore = 0; }
      fclose(f); snprintf(savePath, sizeof(savePath), "%s", p); return;
    }
  }
  for (const char* p : order) {
    FILE* f = fopen(p, "ab");
    if (f) { fclose(f); snprintf(savePath, sizeof(savePath), "%s", p); saveAll(); return; }
  }
}

// ---------------------------------------------------------------- ground (integer, span based)
static uint8_t giOut, giSh, giEdge, giRoad, giDash, giDet;
static inline int fdiv24(int v) { return (v + 24000) / 24 - 1000; }
static void groundRect(int x0, int y0, int x1, int y1) {
  const Scene& s = SC();
  const int B[6] = {-96, -72, -69, 93, 96, 120};          // 24*b of the band edges
  const uint8_t col[7] = {giOut, giSh, giEdge, giRoad, giEdge, giSh, giOut};
  const int lanes[3] = {-27, 15, 57};                       // 24*(laneB(i)-.25)
  const int r = s.outsideWater ? 6 : 1;
  for (int y = y0; y < y1; y++) {
    int c = -ISO_OX + 2 * (y - ISO_OY);                     // U = x + c   (U = 24*b)
    int vc = 4 * camS - ISO_OX - 2 * (y - ISO_OY);          // V = x + vc  (V = 24*a)
    int prev = x0;
    for (int i = 0; i < 7; i++) {
      int xe = i < 6 ? B[i] - c : x1;
      if (xe > x1) xe = x1;
      if (xe > prev) { bgC.spanV(prev, xe - 1, y, col[i]); prev = xe; }
    }
    if (!s.boat) for (int L : lanes) for (int U = L; U < L + 3; U++) {
      int x = U - c; if (x < x0 || x >= x1) continue;
      if ((fdiv24(x + vc) & 1) == 0) bgC.pxV(x, y, giDash);
    }
    for (int bi = -14; bi <= 13; bi += 3) {
      if (bi > -5 && bi < 5) continue;
      int Uc = 24 * bi + 12;
      for (int U = Uc - r; U <= Uc + r; U++) {
        int x = U - c; if (x < x0 || x >= x1) continue;
        int V = x + vc, ai = fdiv24(V), dV = V - (24 * ai + 12);
        if (dV >= -r && dV <= r && hash2(ai, bi, 41) % 5 == 0) bgC.pxV(x, y, giDet);
      }
    }
  }
}

// ---------------------------------------------------------------- BG3 streaming
static void flushRect(int x0, int y0, int x1, int y1) {
  uint16_t* vram = (uint16_t*)bgGetGfxPtr(bg3);
  int ox = bgC.ox, oy = bgC.oy;
  x0 &= ~1; x1 = (x1 + 1) & ~1;
  for (int y = y0; y < y1; y++) {
    int row = ((y + oy) & 255) * 512;
    for (int x = x0; x < x1; x += 2) {
      int t = row + ((x + ox) & 511);
      vram[t >> 1] = shadow[t] | (shadow[t + 1] << 8);
    }
  }
}
static void drawRegion(int x0, int y0, int x1, int y1) {
  if (x1 <= x0 || y1 <= y0) return;
  G = &bgC; bgC.setClip(x0, y0, x1, y1);
  groundRect(x0, y0, x1, y1);
  clipX0 = x0; clipY0 = y0; clipX1 = x1; clipY1 = y1;
  buildDrawList();
  for (int i = 0; i < ndl; i++) drawItem(dl[i]);
}
static void setView() { bgC.ox = 2 * camS; bgC.oy = -camS; }
static void sceneColors() {
  const Scene& s = SC();
  bgMap.reset(BG_PALETTE);
  uint16_t outside = s.outsideWater ? s.water : s.ground;
  if (s.boat) outside = s.ground;
  giOut = bgMap.get(outside); giSh = bgMap.get(s.shoulder); giEdge = bgMap.get(s.edge); giRoad = bgMap.get(s.road);
  giDash = bgMap.get(s.dash); giDet = bgMap.get(s.outsideWater ? s.water2 : shade(outside, 200));
  BG_PALETTE[0] = bgr555(outside);
}
static void buildSprites();
// Render-ahead: content is kept complete up to camera position renderedS >= camS. When the lead drops
// below LEAD_LOW, the next LEAD_STEP scroll units are rendered in 5 chunks over 5 frames (4 x 48-row
// pieces of the right-hand strip + the top strip), so no single frame pays for a whole strip.
// Safe because the 512x256 torus has 256px of horizontal and 64px of vertical off-screen slack.
static constexpr int LEAD_LOW = 12, LEAD_STEP = 16;
static void drawAhead(int R, int x0, int y0, int x1, int y1) {
  int save = camS; camS = R; setView();
  drawRegion(x0, y0, x1, y1); flushRect(x0, y0, x1, y1);
  camS = save; setView();
}
static void updateView() {
  int s = ifloor(camA * 6);
  if (sceneIdx != viewScene) { viewScene = sceneIdx; sceneColors(); buildSprites(); needFull = true; }
  if (needFull || s < lastS || s > renderedS) {
    camS = s; setView();
    drawRegion(0, 0, SCR_W, SCR_H);
    // render the whole lead now too, otherwise the next frame's movement would force another full redraw
    { const int R = s + LEAD_STEP, d = LEAD_STEP; int save = camS; camS = R; setView();
      drawRegion(SCR_W - 2 * d, 0, SCR_W, SCR_H); drawRegion(0, 0, SCR_W - 2 * d, d);
      camS = save; setView(); renderedS = R; }
    DC_FlushRange(shadow, sizeof(shadow));
    dmaCopy(shadow, bgGetGfxPtr(bg3), sizeof(shadow));
    needFull = false; jobR = -1;
  }
  camS = s; setView(); lastS = s;
  if (jobR < 0 && renderedS - camS < LEAD_LOW) { jobR = renderedS + LEAD_STEP; jobPart = 0; }
  if (jobR >= 0) {
    const int d = jobR - renderedS;
    if (jobPart < 4) drawAhead(jobR, SCR_W - 2 * d, jobPart * 48, SCR_W, jobPart * 48 + 48);
    else drawAhead(jobR, 0, 0, SCR_W - 2 * d, d);
    if (++jobPart == 5) { renderedS = jobR; jobR = -1; }
  }
}

// ---------------------------------------------------------------- sprites
enum { SPR_PLAYER, SPR_PBOAT, SPR_CIV0, SPR_CIV4 = SPR_CIV0 + 4, SPR_RAMMER, SPR_GUNNER, SPR_EBOAT, SPR_VAN, SPR_OIL,
       SPR_SMOKE0, SPR_SMOKE2 = SPR_SMOKE0 + 2, SPR_BULLET, SPR_EBULLET, SPR_MISSILE, SPR_TORPEDO, SPR_BOMB, SPR_BOMBSH,
       SPR_HELI0, SPR_HELI3 = SPR_HELI0 + 3, SPR_HELISH, SPR_EXPL0, SPR_EXPL3 = SPR_EXPL0 + 3, SPR_COUNT };
struct SprDef { SpriteSize size; uint8_t w, h, ax, ay; uint16_t* gfx; };
static SprDef spr[SPR_COUNT];
static const uint16_t CIVCOL[5] = {RGB(240,200,40), RGB(40,120,220), RGB(64,140,80), RGB(232,232,240), RGB(240,140,40)};
static void defSpr(int id, SpriteSize sz, int w, int h, int ax, int ay) {
  spr[id] = SprDef{sz, (uint8_t)w, (uint8_t)h, (uint8_t)ax, (uint8_t)ay, oamAllocateGfx(&oamMain, sz, SpriteColorFormat_256Color)};
}
static void allocSprites() {
  defSpr(SPR_PLAYER, SpriteSize_64x32, 64, 32, 20, 22); defSpr(SPR_PBOAT, SpriteSize_64x32, 64, 32, 20, 22);
  for (int i = 0; i < 5; i++) defSpr(SPR_CIV0 + i, SpriteSize_64x32, 64, 32, 20, 22);
  defSpr(SPR_RAMMER, SpriteSize_64x32, 64, 32, 20, 22); defSpr(SPR_GUNNER, SpriteSize_64x32, 64, 32, 20, 22);
  defSpr(SPR_EBOAT, SpriteSize_64x32, 64, 32, 20, 22); defSpr(SPR_VAN, SpriteSize_64x64, 64, 64, 6, 44);
  defSpr(SPR_OIL, SpriteSize_32x32, 32, 32, 2, 16);
  for (int i = 0; i < 3; i++) defSpr(SPR_SMOKE0 + i, SpriteSize_32x32, 32, 32, 0, 22);
  defSpr(SPR_BULLET, SpriteSize_8x8, 8, 8, 4, 7); defSpr(SPR_EBULLET, SpriteSize_8x8, 8, 8, 4, 7);
  defSpr(SPR_MISSILE, SpriteSize_16x16, 16, 16, 12, 10); defSpr(SPR_TORPEDO, SpriteSize_16x16, 16, 16, 13, 4);
  defSpr(SPR_BOMB, SpriteSize_8x8, 8, 8, 4, 4); defSpr(SPR_BOMBSH, SpriteSize_16x8, 16, 8, 8, 4);
  for (int i = 0; i < 4; i++) defSpr(SPR_HELI0 + i, SpriteSize_64x64, 64, 64, 15, 40);
  defSpr(SPR_HELISH, SpriteSize_64x32, 64, 32, 12, 16);
  for (int i = 0; i < 4; i++) defSpr(SPR_EXPL0 + i, SpriteSize_32x32, 32, 32, 16, 20);
}
template <class F> static void renderSpr(int id, F draw) {
  SprDef& d = spr[id];
  memset(scratch, 0, sizeof(scratch));
  sprC.setClip(0, 0, d.w, d.h);
  int saveS = camS, sx = gOX, sy = gOY;
  camS = 0; gOX = d.ax; gOY = d.ay; G = &sprC;
  draw();
  camS = saveS; gOX = sx; gOY = sy;
  static uint16_t tiles[64 * 64 / 2] __attribute__((aligned(4)));
  int k = 0;
  for (int ty = 0; ty < d.h / 8; ty++) for (int tx = 0; tx < d.w / 8; tx++)
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x += 2) {
      const uint8_t* p = scratch + (ty * 8 + y) * 64 + tx * 8 + x;
      tiles[k++] = p[0] | (p[1] << 8);
    }
  for (int i = 0; i < k; i++) d.gfx[i] = tiles[i];
}
static void buildSprites() {
  const Scene& s = SC();
  objMap.reset(SPRITE_PALETTE);
  uint16_t pbody = s.deco == D_NEON ? RGB(0,220,248) : RGB(232,232,240);
  renderSpr(SPR_PLAYER, [&] { drawCar(0, 0, pbody, s.lights, false); });
  renderSpr(SPR_PBOAT, [&] { drawCar(0, 0, RGB(232,232,240), s.lights, true); });
  for (int i = 0; i < 5; i++) renderSpr(SPR_CIV0 + i, [&] { drawCar(0, 0, CIVCOL[i], s.lights, false); });
  renderSpr(SPR_RAMMER, [&] { drawCar(0, 0, RGB(200,40,40), s.lights, false); });
  renderSpr(SPR_GUNNER, [&] { drawCar(0, 0, RGB(30,30,34), s.lights, false); SP g = P(.2f, .4f, 12); rect(g.x - 3, g.y - 1, 6, 2, RGB(160,160,160)); });
  renderSpr(SPR_EBOAT, [&] { drawCar(0, 0, RGB(60,60,60), s.lights, true); SP c = P(.7f, .3f, 10); rect(c.x - 3, c.y - 3, 6, 4, RGB(200,30,30)); });
  renderSpr(SPR_VAN, [&] { isoBox(0, 0, 2.6f, 1.1f, 16, RGB(60,60,150)); SP p = P(.5f, .3f, 12);
    G->setTextSize(1); G->setTextColor(TFT_WHITE); G->setTextDatum(TL_DATUM); G->drawString("W", p.x, p.y);
    SP r0 = P(0, .2f, 3), r1 = P(0, .9f, 3); G->drawLine(r0.x, r0.y, r1.x, r1.y, RGB(248,220,64)); });
  renderSpr(SPR_OIL, [&] { SP c = P(.6f, .55f); G->fillEllipse(c.x, c.y, (int)(14 * ZS), (int)(6 * ZS), RGB(20,20,28)); G->fillEllipse(c.x - 3, c.y - 1, 4, 2, RGB(70,50,110)); });
  for (int i = 0; i < 3; i++) renderSpr(SPR_SMOKE0 + i, [&] { SP c = P(.6f, .55f, 8); int r = (int)((8 + (0.5f + i * 1.25f) * 3) * ZS);
    G->fillCircle(c.x, c.y, r, RGB(200,200,210)); G->fillCircle(c.x + r / 2, c.y - 4, r * 2 / 3, RGB(230,230,236)); });
  renderSpr(SPR_BULLET, [&] { SP p = P(0, 0, 4); rect(p.x - 1, p.y - 1, 3, 3, RGB(255,240,80)); });
  renderSpr(SPR_EBULLET, [&] { SP p = P(0, 0, 4); rect(p.x - 1, p.y - 1, 3, 3, RGB(255,80,80)); });
  renderSpr(SPR_MISSILE, [&] { SP p = P(0, 0, 8), q = P(-.6f, 0, 8); G->drawLine(q.x, q.y, p.x, p.y, RGB(220,220,220)); G->drawLine(q.x, q.y + 1, p.x, p.y + 1, RGB(248,140,40)); });
  renderSpr(SPR_TORPEDO, [&] { SP p = P(0, 0), q = P(-1, 0); G->drawLine(q.x, q.y, p.x, p.y, RGB(220,240,255)); rect(p.x - 2, p.y - 1, 4, 3, RGB(40,40,40)); });
  renderSpr(SPR_BOMB, [&] { SP p = P(0, 0); G->fillCircle(p.x, p.y, 3, RGB(8,8,8)); });
  renderSpr(SPR_BOMBSH, [&] { SP p = P(0, 0); G->fillEllipse(p.x, p.y, 4, 2, shade(s.road, 140)); });
  for (int i = 0; i < 4; i++) renderSpr(SPR_HELI0 + i, [&] {
    isoBox(0, 0, 2, 1.1f, 14, RGB(56,88,48)); isoBox(.7f, .1f, .9f, .9f, 6, RGB(120,180,220), 6);
    isoBox(-1.6f, .4f, 1.6f, .3f, 5, RGB(56,88,48), 6);
    SP r = P(1, .55f, 22); float ang = i * 3.14159f / 8; int dx = (int)(cosf(ang) * 44 * ZS), dy = (int)(sinf(ang) * 16 * ZS);
    G->drawLine(r.x - dx, r.y - dy, r.x + dx, r.y + dy, RGB(220,220,220)); G->drawLine(r.x + dy * 2, r.y - dx / 3, r.x - dy * 2, r.y + dx / 3, RGB(170,170,170)); });
  renderSpr(SPR_HELISH, [&] { SP c = P(1, .55f); G->fillEllipse(c.x, c.y, (int)(30 * ZS), (int)(12 * ZS), shade(s.road, 140)); });
  for (int i = 0; i < 4; i++) renderSpr(SPR_EXPL0 + i, [&] { SP c = P(0, 0, 6); float t = .1f + i * .3f; int r = (int)((4 + t * 14) * ZS);
    G->fillCircle(c.x, c.y, r, RGB(248,136,32)); G->fillCircle(c.x, c.y, r * 2 / 3, RGB(255,240,120)); });
}
struct OS { float depth; int16_t x, y; uint8_t id; };
static OS os[128]; static int nos = 0;
static int shx = 0, shy = 0;
static void addSpr(int id, float a, float b, float z, float depth) {
  if (nos >= 128) return;
  SP p = P(a, b, z);
  int x = p.x - spr[id].ax + shx, y = p.y - spr[id].ay + shy;
  if (x <= -64 || x >= 256 || y <= -64 || y >= 192) return;
  os[nos++] = OS{depth, (int16_t)x, (int16_t)y, (uint8_t)id};
}
static void buildOam() {
  nos = 0;
  shx = shy = 0;
  if (shakeT > 0) { shx = rand() % 5 - 2; shy = rand() % 3 - 1; }
  const Scene& s = SC();
  uint32_t fc = vblanks;
  if (pl.dead <= 0 && !(pl.invuln > 0 && ((int)(pl.invuln * 10) & 1)))
    addSpr(s.boat ? SPR_PBOAT : SPR_PLAYER, pl.a, pl.b, 0, (pl.b + .4f) - (pl.a + .8f));
  for (int i = 0; i < MAX_ENT; i++) {
    const Ent& e = ents[i];
    float dep = (e.b + .4f) - (e.a + .8f);
    switch (e.t) {
      case E_CIV: { int k = 0; for (int j = 0; j < 5; j++) if (CIVCOL[j] == e.col) k = j; addSpr(SPR_CIV0 + k, e.a, e.b, 0, dep); break; }
      case E_RAMMER: addSpr(SPR_RAMMER, e.a, e.b, 0, dep); break;
      case E_GUNNER: addSpr(SPR_GUNNER, e.a, e.b, 0, dep); break;
      case E_EBOAT: addSpr(SPR_EBOAT, e.a, e.b, 0, dep); break;
      case E_VAN: addSpr(SPR_VAN, e.a, e.b, 0, (e.b + .55f) - (e.a + 1.3f)); break;
      case E_OIL: addSpr(SPR_OIL, e.a, e.b, 0, dep - 30); break;
      case E_SMOKE: addSpr(SPR_SMOKE0 + (e.timer > 2.4f ? 0 : e.timer > 1.1f ? 1 : 2), e.a, e.b, 0, dep - 30); break;
      case E_BULLET: addSpr(SPR_BULLET, e.a, e.b, 0, dep); break;
      case E_EBULLET: addSpr(SPR_EBULLET, e.a, e.b, 0, dep); break;
      case E_MISSILE: addSpr(SPR_MISSILE, e.a, e.b, 0, dep); break;
      case E_TORPEDO: addSpr(SPR_TORPEDO, e.a, e.b, 0, dep); break;
      case E_BOMB: addSpr(SPR_BOMBSH, e.a, e.b, 0, dep - 30); addSpr(SPR_BOMB, e.a, e.b, e.z, dep + 60); break;
      case E_HELI: addSpr(SPR_HELISH, e.a, e.b, 0, dep - 30); addSpr(SPR_HELI0 + (fc >> 1 & 3), e.a, e.b, e.z, dep + 60); break;
      case E_EXPL: { float t = 1 - e.timer / .6f; addSpr(SPR_EXPL0 + (t < .25f ? 0 : t < .5f ? 1 : t < .75f ? 2 : 3), e.a, e.b, 0, dep + 1); break; }
      default: break;
    }
  }
  for (int i = 1; i < nos; i++) { OS t = os[i]; int j = i - 1; while (j >= 0 && os[j].depth < t.depth) { os[j + 1] = os[j]; j--; } os[j + 1] = t; }
  for (int i = 0; i < 128; i++) {
    if (i < nos) { const SprDef& d = spr[os[i].id];
      oamSet(&oamMain, i, os[i].x, os[i].y & 255, 0, 0, d.size, SpriteColorFormat_256Color, d.gfx, -1, false, false, false, false, false); }
    else oamSetHidden(&oamMain, i, true);
  }
}

// ---------------------------------------------------------------- BG0 overlay (text)
static char ovSig[96] = "?";
static void overlayUpdate() {
  char sig[96]; bool blink = ((int)(tGame * 2)) & 1;
  snprintf(sig, sizeof(sig), "%d|%d|%d|%d|%s|%d", state, blink && state == ST_TITLE, newHi, sceneIdx, (banner > 0 && state == ST_PLAY) ? bannerText : "", 0);
  if (!strcmp(sig, ovSig)) return;
  strcpy(ovSig, sig);
  // only the tile rows used by the previous or the new overlay are cleared/converted
  static uint32_t prevMask = 0xFFFFFF;
  uint32_t mask = 0;
  auto rows = [&](int y0, int y1) { for (int r = y0 / 8; r <= (y1 - 1) / 8 && r < 24; r++) mask |= 1u << r; };
  if (state == ST_TITLE) rows(50, 136);
  else if (state == ST_OVER) rows(70, 125);
  else if (state == ST_PAUSE) rows(84, 100);
  if (banner > 0 && state == ST_PLAY) rows(20, 29);
  uint32_t touch = mask | prevMask; prevMask = mask;
  for (int r = 0; r < 24; r++) if (touch >> r & 1) memset(overlay + r * 8 * 256, 0, 8 * 256);
  G = &ovC; ovC.setClip(0, 0, 256, 192);
  if (state == ST_TITLE) {
    G->fillRect(20, 50, 216, 86, RGB(10,10,20)); G->drawRect(20, 50, 216, 86, SC().hudAcc);
    txt("AGENT RUN", 128, 60, TFT_YELLOW, 3, TC_DATUM, TFT_BLACK);
    txt("A/Y fire  B/X special", 128, 92, TFT_WHITE, 1, TC_DATUM);
    txt("D-pad steer & speed", 128, 104, TFT_WHITE, 1, TC_DATUM);
    if (blink) txt("PRESS START", 128, 120, SC().hudAcc, 1, TC_DATUM);
  } else if (state == ST_OVER) {
    txt("GAME OVER", 128, 70, RGB(248,64,64), 3, TC_DATUM, TFT_BLACK);
    if (newHi) txt("NEW HIGH SCORE!", 128, 100, TFT_YELLOW, 1, TC_DATUM, TFT_BLACK);
    txt("PRESS START", 128, 116, TFT_WHITE, 1, TC_DATUM, TFT_BLACK);
  } else if (state == ST_PAUSE) txt("PAUSED", 128, 84, TFT_WHITE, 2, TC_DATUM, TFT_BLACK);
  if (banner > 0 && state == ST_PLAY) txt(bannerText, 128, 20, TFT_YELLOW, 1, TC_DATUM, TFT_BLACK);
  for (int ty = 0; ty < 24; ty++) {
    if (!(touch >> ty & 1)) continue;
    for (int tx = 0; tx < 32; tx++) {
      uint16_t* t = bg0Tiles + (ty * 32 + tx) * 32;
      for (int y = 0; y < 8; y++) { const uint8_t* p = overlay + (ty * 8 + y) * 256 + tx * 8;
        for (int x = 0; x < 8; x += 2) *t++ = p[x] | (p[x + 1] << 8); }
    }
  }
}

// ---------------------------------------------------------------- input
static void readInput() {
  scanKeys();
  uint32_t h = keysHeld(), d = keysDown();
#ifdef AR_DEMO
  static int fr = 0; fr++;
  h = KEY_A | KEY_UP; d = (fr % 30 == 0) ? KEY_A : 0;
  if (state != ST_PLAY && fr % 90 == 0) d |= KEY_START;
  float c = (ROAD_L + ROAD_R) / 2 - .4f + sinf(fr * .02f) * 2.2f;
  if (pl.b < c - .3f) h |= KEY_RIGHT;
  if (pl.b > c + .3f) h |= KEY_LEFT;
  if (fr % 200 == 100) d |= KEY_B;
#endif
  in.sx = (h & KEY_RIGHT) ? 1 : (h & KEY_LEFT) ? -1 : 0;
  in.sy = (h & KEY_UP) ? 1 : (h & KEY_DOWN) ? -1 : 0;
  in.fire = h & (KEY_A | KEY_Y); in.special = h & (KEY_B | KEY_X);
  in.start = h & KEY_START; in.sel = h & KEY_SELECT; in.l1 = h & KEY_L; in.r1 = h & KEY_R;
  in.fireEdge = d & (KEY_A | KEY_Y); in.specialEdge = d & (KEY_B | KEY_X);
  in.startEdge = d & KEY_START; in.selEdge = d & KEY_SELECT; in.l1Edge = d & KEY_L; in.r1Edge = d & KEY_R;
}

// ---------------------------------------------------------------- bottom screen HUD
static int hudScene = -1, hudState = -1;
static char hudCache[10][64];
static uint32_t tLogic, tBg, tSpr, tHud, tAud, tFrame, tPeak; static int fps = 0;
static inline unsigned us(uint32_t t) { return (unsigned)((uint64_t)t * 1000000 / BUS_CLOCK); }
static void hudStatic() {
  const Scene& s = SC();
  G = &hudC;
  G->fillSprite(s.hudBg);
  txt("AGENT RUN", 4, 4, s.hudAcc, 2);
  txt(s.name, 4, 24, s.hudFg, 1);
  txt("FUEL", 4, 40, s.hudFg, 1);
  G->drawRect(32, 39, 102, 9, s.hudFg);
  G->drawRect(64, 68, 128, 104, s.hudAcc);
  memset(hudCache, 0, sizeof(hudCache));
}

static void field(int k, int x, int y, int w, int h, const char* str, uint16_t c, int size, int datum) {
  if (!strncmp(hudCache[k], str, 63)) return;
  strncpy(hudCache[k], str, 63); hudCache[k][63] = 0;
  const Scene& s = SC();
  G->fillRect(x, y, w, h, s.hudBg);
  int tx = datum == TR_DATUM ? x + w : datum == TC_DATUM ? x + w / 2 : x;
  txt(str, tx, y, c, size, datum);
}
static void hudDynamic(int frame) {
  const Scene& s = SC();
  G = &hudC;
  char b[96];
  snprintf(b, sizeof(b), "%07lu", (unsigned long)score); field(0, 130, 4, 122, 15, b, s.hudFg, 2, TR_DATUM);
  snprintf(b, sizeof(b), "HI %07lu", (unsigned long)hiScore); field(1, 130, 22, 122, 8, b, s.hudFg, 1, TR_DATUM);
  int f = (int)fuel; if (f < 0) f = 0; if (f > 100) f = 100;
  snprintf(b, sizeof(b), "%d", f);
  if (strcmp(hudCache[2], b)) { strcpy(hudCache[2], b); G->fillRect(33 + f, 40, 100 - f, 7, s.hudBg); G->fillRect(33, 40, f, 7, fuel < 25 ? RGB(248,48,48) : RGB(64,220,64)); }
  snprintf(b, sizeof(b), "LIVES %d", lives); field(3, 190, 40, 62, 8, b, s.hudFg, 1, TR_DATUM);
  if (pl.sp != SP_NONE) snprintf(b, sizeof(b), "WEAPON %-7s x%d", SPNAME[pl.sp], pl.ammo); else snprintf(b, sizeof(b), "WEAPON %s", SPNAME[pl.sp]);
  field(4, 4, 54, 180, 8, b, s.hudAcc, 1, TL_DATUM);
  snprintf(b, sizeof(b), "%3d km/h", (int)(pl.speed * 22)); field(5, 190, 54, 62, 8, b, s.hudFg, 1, TR_DATUM);
  snprintf(b, sizeof(b), "VOL %d  MUSIC %s", volume, musicOn ? "ON" : "OFF"); field(6, 0, 175, 256, 8, b, s.hudFg, 1, TC_DATUM);
  if (frame % 30 == 0) {
    snprintf(b, sizeof(b), "FPS %2d %2u.%u pk%u.%u L%u.%u B%u.%u S%u.%u H%u.%u A%u.%u", fps, us(tFrame) / 1000, us(tFrame) / 100 % 10, us(tPeak) / 1000, us(tPeak) / 100 % 10,
             us(tLogic) / 1000, us(tLogic) / 100 % 10, us(tBg) / 1000, us(tBg) / 100 % 10, us(tSpr) / 1000, us(tSpr) / 100 % 10,
             us(tHud) / 1000, us(tHud) / 100 % 10, us(tAud) / 1000, us(tAud) / 100 % 10);
    field(7, 0, 184, 256, 8, b, RGB(160,255,160), 1, TL_DATUM);
  }
  if (frame % 4) return;
  // radar (15 Hz): road strip, player at bottom, 40 tiles ahead
  const int rx = 65, ry = 69, rw = 126, rh = 102;
  const int span = (int)(ROAD_R - ROAD_L) + 4;
  G->fillRect(rx, ry, 2 * rw / span, rh, RGB(8,24,8));
  G->fillRect(rx + 2 * rw / span, ry, (int)(ROAD_R - ROAD_L) * rw / span, rh, RGB(24,56,24));
  G->fillRect(rx + 2 * rw / span + (int)(ROAD_R - ROAD_L) * rw / span, ry, rw - 2 * rw / span - (int)(ROAD_R - ROAD_L) * rw / span, rh, RGB(8,24,8));
  for (auto& e : ents) {
    if (!isCar(e.t) && e.t != E_HELI) continue;
    int da = (int)(e.a - pl.a); if (da < -1 || da > 40) continue;
    int x = rx + (int)((e.b - ROAD_L + 2) * rw) / span, y = ry + rh - 8 - da * rh / 44;
    if (y < ry || y > ry + rh - 5) continue;
    uint16_t c = e.t == E_CIV ? RGB(200,200,200) : e.t == E_VAN ? RGB(80,120,248) : RGB(248,64,64);
    G->fillRect(x, y, 4, 5, c);
  }
  G->fillRect(rx + (int)((pl.b - ROAD_L + 2) * rw) / span, ry + rh - 8, 4, 5, RGB(255,255,64));
}

// ---------------------------------------------------------------- main
int main() {
  cpuStartTiming(2); tBase = cpuGetTiming();
  audioBegin(volume, musicOn);
  videoSetMode(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D);
  videoSetModeSub(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
  vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
  vramSetBankC(VRAM_C_SUB_BG);
  vramSetBankE(VRAM_E_MAIN_SPRITE);
  bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_512x256, 0, 0);
  bgWrapOn(bg3); bgSetPriority(bg3, 3);
  // BG0: 8bpp text BG living in bank B: DISPCNT char/screen base = 128KB, tiles at +0, map at +62KB
  REG_DISPCNT = (REG_DISPCNT & ~((7u << 24) | (7u << 27))) | (2u << 24) | (2u << 27);
  REG_BG0CNT = BG_PRIORITY(0) | BG_COLOR_256 | BG_32x32 | BG_TILE_BASE(0) | BG_MAP_BASE(31);
  bg0Tiles = (uint16_t*)0x06020000; bg0Map = (uint16_t*)(0x06020000 + 31 * 2048);
  for (int i = 0; i < 32 * 32; i++) bg0Map[i] = (i < 24 * 32) ? i : 0;
  int bgBot = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
  oamInit(&oamMain, SpriteMapping_1D_128, false);
  bgC.attach8(shadow, 512, 256, &bgMap);
  sprC.attach8(scratch, 64, 64, &objMap);
  ovC.attach8(overlay, 256, 256, &bgMap); ovC.setClip(0, 0, 256, 192);
  hudC.attach16((uint16_t*)bgGetGfxPtr(bgBot), 256, 192);
  allocSprites(); 
  initScenes();
  initSave(); 
  audioSetSong(SONG_TITLE); 
  srand(1234);
  uint32_t lastV = vblanks, fpsV = vblanks; int frames = 0; uint32_t tPrev = cpuGetTiming();
  while (1) {
    uint32_t t0 = cpuGetTiming();
    readInput(); updVbl();
    uint32_t nv = vblanks; int el = (int)(nv - lastV); lastV = nv;
    if (el < 1) el = 1;
    if (el > 4) el = 4;
    const float dt = el / 60.f;
    if (in.selEdge) { musicOn = !musicOn; audioSetMusic(musicOn); saveAll(); }
    if (in.l1Edge && volume > 0) { volume--; audioSetVolume(volume); saveAll(); }
    if (in.r1Edge && volume < 10) { volume++; audioSetVolume(volume); saveAll(); }
    if (shakeT > 0) shakeT -= dt;
    switch (state) {
      case ST_TITLE: tGame += dt; camA += dt * 3; if (in.startEdge) { newGame(); state = ST_PLAY; audioSetSong(SONG_GAME);
#ifdef AR_START_SCENE
        pl.a += AR_START_SCENE * SCENE_LEN; camA = pl.a + 2;
#endif
      } break;
      case ST_PLAY: if (banner > 0) banner -= dt; update(dt); if (in.startEdge && state == ST_PLAY) state = ST_PAUSE; break;
      case ST_PAUSE: tGame += 0; if (in.startEdge) state = ST_PLAY; break;
      case ST_OVER: tGame += dt; if (in.startEdge) { state = ST_TITLE; audioSetSong(SONG_TITLE); } break;
    }
    uint32_t t1 = cpuGetTiming();
    updateView(); 
    overlayUpdate();
    uint32_t t2 = cpuGetTiming();
    buildOam(); 
    uint32_t t3 = cpuGetTiming();
    if (sceneIdx != hudScene || state != hudState) { hudScene = sceneIdx; hudState = state; hudStatic(); }
    hudDynamic(frames);
    uint32_t t4 = cpuGetTiming();
    audioTick(); 
    uint32_t t5 = cpuGetTiming();
    static uint32_t aL, aB, aS, aH, aA, aF, mF; static int an = 0;
    aL += t1 - t0; aB += t2 - t1; aS += t3 - t2; aH += t4 - t3; aA += t5 - t4; aF += t5 - t0; if (t5 - t0 > mF) mF = t5 - t0;
    if (++an == 60) { tLogic = aL / 60; tBg = aB / 60; tSpr = aS / 60; tHud = aH / 60; tAud = aA / 60; tFrame = aF / 60; tPeak = mF; aL = aB = aS = aH = aA = aF = mF = 0; an = 0; }
    (void)tPrev;
    frames++;
    updVbl();
    if (vblanks - fpsV >= 60) { fps = frames * 60 / (int)(vblanks - fpsV); frames = frames & 1; fpsV = vblanks; static int fc = 0; fc = 0; (void)fc; }
    swiWaitForVBlank();
    bgSetScroll(bg3, (2 * camS) & 511, (-camS) & 255);
    bgUpdate();
    oamUpdate(&oamMain);
    if (flashT > 0) { flashT--; setBrightness(1, flashT * 2); }
  }
}
