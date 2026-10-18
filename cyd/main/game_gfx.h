// AGENT RUN - isometric renderer helpers (strip/band rendering into a TFT_eSprite)
#pragma once
#ifdef AR_PLATFORM_DS
  #include "ds_canvas.h"         // TFT_eSprite-compatible subset for the DS framebuffer
#else
  #include <TFT_eSPI.h>
  typedef TFT_eSprite Canvas;
#endif

#ifdef AR_PLATFORM_DS
// DS top screen: 256x192, whole screen is playfield (HUD lives on the bottom screen), 24x12 tiles
static constexpr int SCR_W = 256, SCR_H = 192;
static constexpr int HUD_H = 0, PLAY_H = SCR_H;
static constexpr int STRIP_H = 192, NSTRIPS = 1;
static constexpr int ISO_OX = 72, ISO_OY = 104;
static constexpr float TILE_HW = 12.f, TILE_HH = 6.f;
#else
static constexpr int SCR_W = 480, SCR_H = 320;
static constexpr int HUD_H = 26, PLAY_H = SCR_H - HUD_H;
static constexpr int STRIP_H = 32, NSTRIPS = SCR_H / STRIP_H;
static constexpr int ISO_OX = 160, ISO_OY = 150;   // screen pos of camera-relative (a=0,b=0)
static constexpr float TILE_HW = 16.f, TILE_HH = 8.f; // 32x16 diamond tiles
#endif
static constexpr float ZS = TILE_HW / 16.f;           // sprite heights were authored for 32x16 tiles

extern Canvas* G;   // sprite currently being drawn
extern int YO;           // screen y of the sprite's top row

static inline uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
static inline uint16_t shade(uint16_t c, int num) {  // num/256 brightness
  int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
  r = r * num >> 8; g = g * num >> 8; b = b * num >> 8;
  return (r << 11) | (g << 5) | b;
}
struct SP { int16_t x, y; };
// ra = world a minus camera a
static inline SP P(float ra, float b, float z = 0) {
  return SP{ (int16_t)(ISO_OX + (ra + b) * TILE_HW), (int16_t)(ISO_OY + (b - ra) * TILE_HH - z * ZS) };
}
static inline void quad(SP a, SP b, SP c, SP d, uint16_t col) {
  G->fillTriangle(a.x, a.y - YO, b.x, b.y - YO, c.x, c.y - YO, col);
  G->fillTriangle(a.x, a.y - YO, c.x, c.y - YO, d.x, d.y - YO, col);
}
static inline void quadOutline(SP a, SP b, SP c, SP d, uint16_t col) {
  G->drawLine(a.x, a.y - YO, b.x, b.y - YO, col); G->drawLine(b.x, b.y - YO, c.x, c.y - YO, col);
  G->drawLine(c.x, c.y - YO, d.x, d.y - YO, col); G->drawLine(d.x, d.y - YO, a.x, a.y - YO, col);
}
// Flat-shaded isometric box: top face lit, +b face 62%, -a face 80%
static void isoBox(float a, float b, float L, float Wd, float h, uint16_t top, float z0 = 0, bool outline = true) {
  SP p0 = P(a, b + Wd, z0), p1 = P(a + L, b + Wd, z0), p2 = P(a + L, b + Wd, z0 + h), p3 = P(a, b + Wd, z0 + h);
  quad(p0, p1, p2, p3, shade(top, 160));
  SP q0 = P(a, b, z0), q3 = P(a, b, z0 + h);
  quad(q0, p0, p3, q3, shade(top, 205));
  SP t1 = P(a + L, b, z0 + h);
  quad(q3, t1, p2, p3, top);
  if (outline) {
    quadOutline(p0, p1, p2, p3, TFT_BLACK);
    quadOutline(q0, p0, p3, q3, TFT_BLACK);
    quadOutline(q3, t1, p2, p3, TFT_BLACK);
  }
}
static inline void flatQuad(float a, float b, float L, float Wd, uint16_t c) {
  quad(P(a, b), P(a + L, b), P(a + L, b + Wd), P(a, b + Wd), c);
}
static inline void rect(int x, int y, int w, int h, uint16_t c) { G->fillRect(x, y - YO, w, h, c); }
static inline void txt(const char* s, int x, int y, uint16_t c, int size, int datum = TL_DATUM, int shadowCol = -1) {
  G->setTextFont(1); G->setTextSize(size); G->setTextDatum(datum);
  if (shadowCol >= 0) { G->setTextColor((uint16_t)shadowCol); G->drawString(s, x + size, y - YO + size); }
  G->setTextColor(c); G->drawString(s, x, y - YO);
}
