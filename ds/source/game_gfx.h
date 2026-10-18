// AGENT RUN (DS) - isometric projection + flat-shaded primitives on top of ds_canvas
#pragma once
#include "ds_canvas.h"
typedef TFT_eSprite Canvas;

static constexpr int SCR_W = 256, SCR_H = 192;
static constexpr int HUD_H = 0, PLAY_H = SCR_H;
static constexpr int ISO_OX = 72, ISO_OY = 104;
static constexpr float TILE_HW = 12.f, TILE_HH = 6.f;
static constexpr float ZS = TILE_HW / 16.f;

extern Canvas* G;     // canvas currently being drawn
extern int YO;        // always 0 on DS
extern int camS;      // integer camera: world scroll in pixels / (2,-1)
extern int gOX, gOY;  // projection origin (ISO_OX/OY on screen, or sprite anchor when pre-rendering)

static inline uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
static inline uint16_t shade(uint16_t c, int num) {
  int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
  r = r * num >> 8; g = g * num >> 8; b = b * num >> 8;
  return (r << 11) | (g << 5) | b;
}
struct SP { int16_t x, y; };
static inline int ifloor(float v) { int i = (int)v; return (v < i) ? i - 1 : i; }
// a,b = absolute world tile coords. Camera is applied as an exact integer pixel shift so scrolled
// content drawn on different frames lines up perfectly.
static inline SP P(float a, float b, float z = 0) {
  return SP{ (int16_t)(gOX + ifloor((a + b) * TILE_HW) - 2 * camS), (int16_t)(gOY + ifloor((b - a) * TILE_HH - z * ZS) + camS) };
}
static inline void quad(SP a, SP b, SP c, SP d, uint16_t col) {
  G->fillTriangle(a.x, a.y, b.x, b.y, c.x, c.y, col);
  G->fillTriangle(a.x, a.y, c.x, c.y, d.x, d.y, col);
}
static inline void quadOutline(SP a, SP b, SP c, SP d, uint16_t col) {
  G->drawLine(a.x, a.y, b.x, b.y, col); G->drawLine(b.x, b.y, c.x, c.y, col);
  G->drawLine(c.x, c.y, d.x, d.y, col); G->drawLine(d.x, d.y, a.x, a.y, col);
}
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
static inline void flatQuad(float a, float b, float L, float Wd, uint16_t c) { quad(P(a, b), P(a + L, b), P(a + L, b + Wd), P(a, b + Wd), c); }
static inline void rect(int x, int y, int w, int h, uint16_t c) { G->fillRect(x, y, w, h, c); }
static inline void txt(const char* s, int x, int y, uint16_t c, int size, int datum = TL_DATUM, int shadowCol = -1) {
  G->setTextSize(size); G->setTextDatum(datum);
  if (shadowCol >= 0) { G->setTextColor((uint16_t)shadowCol); G->drawString(s, x + size, y + size); }
  G->setTextColor(c); G->drawString(s, x, y);
}
