/* Renderizador 2D por software, framebuffer RGB565 de 320x240 (Undertale usa 320x240 ampliado x2). */
#ifndef GFX_H
#define GFX_H
#include <stdint.h>

#define SCR_W 320
#define SCR_H 240

typedef uint16_t px_t;
#define PX_KEY 0xF81F                                   /* color transparente en sprites */
#define RGB565(r, g, b) ((px_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_YELLOW RGB565(255, 255, 0)
#define C_RED    RGB565(255, 0, 0)
#define C_ORANGE RGB565(252, 166, 0)
#define C_GREEN  RGB565(0, 255, 0)
#define C_GRAY   RGB565(128, 128, 128)
#define C_DGRAY  RGB565(64, 64, 64)
#define C_LGRAY  RGB565(192, 192, 192)

typedef struct { int16_t w, h, ox, oy; px_t *px; } Img;                     /* sprite RGB565 + origen */
typedef struct { uint8_t w, h; int8_t xo; uint8_t adv; uint16_t off; } Glyph;
typedef struct { Glyph g[95]; uint16_t *rows; uint8_t lineh; } Font;        /* ASCII 32..126, 1 bit por pixel */

extern px_t g_fb[SCR_W * SCR_H];

void gfx_clear(px_t c);
void gfx_rect(int x, int y, int w, int h, px_t c);
void gfx_img(const Img *im, int x, int y);                  /* x,y = esquina superior izquierda */
void gfx_sprite(const Img *im, int x, int y);               /* x,y = punto de origen del sprite */
void gfx_text_n(const Font *f, int x, int y, const char *s, int maxch, px_t c, int scale);
void gfx_text(const Font *f, int x, int y, const char *s, px_t c, int scale);
int  gfx_text_w(const Font *f, const char *s, int scale);
void gfx_world(const px_t *src, int sw, int sh, int camx, int camy);   /* ventana de una imagen grande */
void gfx_error(const char *l1, const char *l2, const char *l3);        /* pantalla de error sin recursos */

#endif
