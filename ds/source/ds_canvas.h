// TFT_eSprite-compatible software canvas for the DS port.
//  mode 0: 8bpp palette indices into a power-of-two wrapped buffer (scrolling BG shadow / sprite scratch)
//  mode 1: 16bpp BGR555 direct (sub screen bitmap in VRAM; 16-bit writes only)
// Colours are passed as RGB565 and converted once per primitive, never per pixel.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#define TFT_BLACK  0x0000
#define TFT_WHITE  0xFFFF
#define TFT_NAVY   0x000F
#define TFT_YELLOW 0xFFE0
#define TFT_RED    0xF800
#define TFT_GREEN  0x07E0
enum { TL_DATUM = 0, TC_DATUM = 1, TR_DATUM = 2, ML_DATUM = 3, MC_DATUM = 4, MR_DATUM = 5 };

static inline uint16_t bgr555(uint16_t c) { return 0x8000 | ((c & 0x1F) << 10) | (((c >> 6) & 0x1F) << 5) | (c >> 11); }

// RGB565 -> palette index allocator writing into a hardware palette (index 0 reserved/transparent)
struct ColorMap {
  uint16_t key[512]; uint8_t val[512]; bool used[512];
  volatile uint16_t* hw = nullptr; int n = 1;
  void reset(volatile uint16_t* pal) { hw = pal; n = 1; memset(used, 0, sizeof(used)); }
  uint8_t get(uint16_t c) {
    unsigned h = (c * 40503u) >> 7 & 511;
    while (used[h]) { if (key[h] == c) return val[h]; h = (h + 1) & 511; }
    if (n >= 256) return 255;
    used[h] = true; key[h] = c; val[h] = (uint8_t)n; hw[n] = bgr555(c);
    return (uint8_t)n++;
  }
};

class TFT_eSprite {
public:
  uint8_t mode = 0;
  uint8_t* b8 = nullptr; uint16_t* b16 = nullptr;
  int bw = 0, bh = 0;            // buffer size (power of two in mode 0)
  int ox = 0, oy = 0;            // canvas -> buffer offset (wrapped)
  int cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;   // clip rect, canvas coords, [x0,x1)
  ColorMap* cm = nullptr;
  int tsize = 1, datum = 0; uint16_t tcol = 0xFFFF;

  void attach8(uint8_t* b, int W, int H, ColorMap* map) { mode = 0; b8 = b; bw = W; bh = H; cm = map; ox = oy = 0; setClip(0, 0, W, H); }
  void attach16(uint16_t* b, int W, int H) { mode = 1; b16 = b; bw = W; bh = H; ox = oy = 0; setClip(0, 0, W, H); }
  void setClip(int x0, int y0, int x1, int y1) { cx0 = x0; cy0 = y0; cx1 = x1; cy1 = y1; }
  inline uint16_t cv(uint16_t c) { return mode == 0 ? cm->get(c) : bgr555(c); }

  // raw span, v already converted, coordinates unclipped
  void spanV(int x0, int x1, int y, uint16_t v) {      // inclusive x1
    if (y < cy0 || y >= cy1) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < cx0) x0 = cx0;
    if (x1 >= cx1) x1 = cx1 - 1;
    if (x0 > x1) return;
    if (mode == 1) {
      uint16_t* p = b16 + y * bw + x0; int n = x1 - x0 + 1;
      if ((uintptr_t)p & 2) { *p++ = v; n--; }
      uint32_t vv = v | ((uint32_t)v << 16); uint32_t* q = (uint32_t*)p;
      for (; n >= 2; n -= 2) *q++ = vv;
      if (n) *(uint16_t*)q = v;
      return;
    }
    uint8_t* row = b8 + ((y + oy) & (bh - 1)) * bw;
    int tx = (x0 + ox) & (bw - 1), n = x1 - x0 + 1;
    int first = n < bw - tx ? n : bw - tx;
    memset(row + tx, v, first);
    if (n > first) memset(row, v, n - first);
  }
  inline void pxV(int x, int y, uint16_t v) {
    if (x < cx0 || x >= cx1 || y < cy0 || y >= cy1) return;
    if (mode == 1) b16[y * bw + x] = v;
    else b8[((y + oy) & (bh - 1)) * bw + ((x + ox) & (bw - 1))] = (uint8_t)v;
  }
  void hspan(int x0, int x1, int y, uint16_t c) { spanV(x0, x1, y, cv(c)); }
  void fillSprite(uint16_t c) { uint16_t v = cv(c); for (int y = cy0; y < cy1; y++) spanV(cx0, cx1 - 1, y, v); }
  void drawFastHLine(int x, int y, int l, uint16_t c) { if (l > 0) spanV(x, x + l - 1, y, cv(c)); }
  void drawFastVLine(int x, int y, int l, uint16_t c) { uint16_t v = cv(c); for (int i = 0; i < l; i++) pxV(x, y + i, v); }
  void drawPixel(int x, int y, uint16_t c) { pxV(x, y, cv(c)); }
  void fillRect(int x, int y, int W, int H, uint16_t c) {
    if (W <= 0) return;
    uint16_t v = cv(c);
    int ys = y < cy0 ? cy0 : y, ye = y + H > cy1 ? cy1 : y + H;
    for (int j = ys; j < ye; j++) spanV(x, x + W - 1, j, v);
  }
  void drawRect(int x, int y, int W, int H, uint16_t c) {
    drawFastHLine(x, y, W, c); drawFastHLine(x, y + H - 1, W, c);
    drawFastVLine(x, y, H, c); drawFastVLine(x + W - 1, y, H, c);
  }
  void drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    // trivial reject against clip rect
    if ((x0 < cx0 && x1 < cx0) || (x0 >= cx1 && x1 >= cx1) || (y0 < cy0 && y1 < cy0) || (y0 >= cy1 && y1 >= cy1)) return;
    uint16_t v = cv(c);
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (int n = 0; n < 1024; n++) {
      pxV(x0, y0, v);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
  // 16.16 fixed-point edge walking: 2 divides per triangle instead of 2 per row
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    if (y0 > y1) { int t; t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { int t; t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { int t; t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y2 < cy0 || y0 >= cy1) return;
    int mnx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int mxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    if (mxx < cx0 || mnx >= cx1) return;
    uint16_t v = cv(c);
    if (y0 == y2) { spanV(mnx, mxx, y0, v); return; }
    int32_t dA = ((x2 - x0) << 16) / (y2 - y0);
    int32_t dB = (y1 > y0) ? ((x1 - x0) << 16) / (y1 - y0) : 0;
    int32_t dC = (y2 > y1) ? ((x2 - x1) << 16) / (y2 - y1) : 0;
    int ys = y0 < cy0 ? cy0 : y0, ye = y2 >= cy1 ? cy1 - 1 : y2;
    for (int y = ys; y <= ye; y++) {
      int xa = x0 + ((dA * (y - y0) + 0x8000) >> 16), xb;
      if (y < y1) xb = x0 + ((dB * (y - y0) + 0x8000) >> 16);
      else if (y2 > y1) xb = x1 + ((dC * (y - y1) + 0x8000) >> 16);
      else xb = x1;
      spanV(xa, xb, y, v);
    }
  }
  void fillEllipse(int cx, int cy, int rx, int ry, uint16_t c) {
    uint16_t v = cv(c);
    if (rx <= 0 || ry <= 0) { pxV(cx, cy, v); return; }
    if (cy + ry < cy0 || cy - ry >= cy1 || cx + rx < cx0 || cx - rx >= cx1) return;
    int dx = rx; int32_t rx2 = rx * rx, ry2 = ry * ry;
    for (int dy = 0; dy <= ry; dy++) {
      while (dx > 0 && (int32_t)dx * dx * ry2 + (int32_t)dy * dy * rx2 > rx2 * ry2) dx--;
      spanV(cx - dx, cx + dx, cy + dy, v);
      if (dy) spanV(cx - dx, cx + dx, cy - dy, v);
    }
  }
  void fillCircle(int cx, int cy, int r, uint16_t c) { fillEllipse(cx, cy, r, r, c); }
  // text (Adafruit 5x7, 6px advance)
  void setTextFont(int) {}
  void setTextSize(int s) { tsize = s < 1 ? 1 : s; }
  void setTextColor(uint16_t c) { tcol = c; }
  void setTextColor(uint16_t c, uint16_t) { tcol = c; }
  void setTextDatum(int d) { datum = d; }
  int textWidth(const char* s) { return (int)strlen(s) * 6 * tsize; }
  void drawChar(int x, int y, unsigned char ch, uint16_t v);
  int drawString(const char* s, int x, int y) {
    int W = textWidth(s), H = 7 * tsize;
    if (datum % 3 == 1) x -= W / 2;
    else if (datum % 3 == 2) x -= W;
    if (datum / 3 == 1) y -= H / 2;
    uint16_t v = cv(tcol);
    for (; *s; s++) { drawChar(x, y, (unsigned char)*s, v); x += 6 * tsize; }
    return W;
  }
};
