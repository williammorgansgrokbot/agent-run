#include "ds_canvas.h"
#include "glcdfont.h"
void TFT_eSprite::drawChar(int x, int y, unsigned char ch, uint16_t v) {
  const unsigned char* g = font + ch * 5;
  for (int i = 0; i < 5; i++) {
    unsigned char col = g[i];
    for (int j = 0; j < 8; j++) {
      if (!(col & (1 << j))) continue;
      if (tsize == 1) pxV(x + i, y + j, v);
      else for (int a = 0; a < tsize; a++) spanV(x + i * tsize, x + i * tsize + tsize - 1, y + j * tsize + a, v);
    }
  }
}
