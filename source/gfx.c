#include <string.h>
#include "gfx.h"

px_t g_fb[SCR_W * SCR_H];

void gfx_clear(px_t c)
{
    for (int i = 0; i < SCR_W * SCR_H; i++) g_fb[i] = c;
}

void gfx_rect(int x, int y, int w, int h, px_t c)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCR_W ? SCR_W : x + w, y1 = y + h > SCR_H ? SCR_H : y + h;
    if (x1 <= x0) return;
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) g_fb[yy * SCR_W + xx] = c;
}

void gfx_img(const Img *im, int x, int y)
{
    for (int j = 0; j < im->h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= SCR_H) continue;
        const px_t *src = im->px + j * im->w;
        px_t *dst = &g_fb[yy * SCR_W];
        for (int i = 0; i < im->w; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= SCR_W) continue;
            if (src[i] != PX_KEY) dst[xx] = src[i];
        }
    }
}

void gfx_sprite(const Img *im, int x, int y)
{
    gfx_img(im, x - im->ox, y - im->oy);
}

void gfx_text_n(const Font *f, int x, int y, const char *s, int maxch, px_t c, int scale)
{
    int cx = x;
    for (int n = 0; s[n] && n < maxch; n++) {
        unsigned char ch = (unsigned char)s[n];
        if (ch == '\n') { cx = x; y += f->lineh * scale; continue; }
        if (ch < 32 || ch > 126) ch = ' ';
        const Glyph *g = &f->g[ch - 32];
        for (int r = 0; r < g->h; r++) {
            uint16_t bits = f->rows[g->off + r];
            for (int col = 0; col < g->w; col++)
                if (bits & (0x8000 >> col))
                    gfx_rect(cx + (g->xo + col) * scale, y + r * scale, scale, scale, c);
        }
        cx += g->adv * scale;
    }
}

void gfx_text(const Font *f, int x, int y, const char *s, px_t c, int scale)
{
    gfx_text_n(f, x, y, s, 1 << 30, c, scale);
}

int gfx_text_w(const Font *f, const char *s, int scale)
{
    int w = 0;
    for (; *s && *s != '\n'; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch < 32 || ch > 126) ch = ' ';
        w += f->g[ch - 32].adv * scale;
    }
    return w;
}

void gfx_world(const px_t *src, int sw, int sh, int camx, int camy)
{
    int x0 = camx < 0 ? -camx : 0;
    int x1 = (sw - camx) < SCR_W ? (sw - camx) : SCR_W;
    for (int y = 0; y < SCR_H; y++) {
        px_t *dst = &g_fb[y * SCR_W];
        int sy = camy + y;
        if (sy < 0 || sy >= sh || x1 <= x0) {
            for (int x = 0; x < SCR_W; x++) dst[x] = C_BLACK;
            continue;
        }
        for (int x = 0; x < x0; x++) dst[x] = C_BLACK;
        memcpy(dst + x0, src + sy * sw + camx + x0, (size_t)(x1 - x0) * sizeof(px_t));
        for (int x = x1; x < SCR_W; x++) dst[x] = C_BLACK;
    }
}

/* ---- pantalla de error con una fuente minima 3x5 (para cuando faltan los recursos) ---- */
static const char FONT35_CH[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:/-_, ";
static const uint16_t FONT35[] = {                       /* 15 bits: filas de 3 bits, de arriba abajo */
    0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B, 0x5BED, 0x7497, 0x126A,
    0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A, 0x6BA4, 0x2B7B, 0x6BAD, 0x388E, 0x7492,
    0x5B6F, 0x5B6A, 0x5BFD, 0x5AAD, 0x5A92, 0x72A7, 0x7B6F, 0x2C97, 0x62A7, 0x628E,
    0x5BC9, 0x798E, 0x39EF, 0x7292, 0x7BEF, 0x7BCE, 0x0002, 0x0410, 0x12A4, 0x01C0,
    0x0007, 0x0014, 0x0000
};

void gfx_error(const char *l1, const char *l2, const char *l3)
{
    gfx_clear(RGB565(96, 0, 0));
    const char *lines[3] = { l1, l2, l3 };
    for (int k = 0; k < 3; k++) {
        const char *s = lines[k];
        for (int i = 0; s && s[i]; i++) {
            char ch = s[i];
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            const char *p = strchr(FONT35_CH, ch);
            uint16_t bits = p ? FONT35[p - FONT35_CH] : 0;
            for (int r = 0; r < 5; r++)
                for (int c = 0; c < 3; c++)
                    if (bits & (1 << (14 - (r * 3 + c))))
                        gfx_rect(8 + i * 12 + c * 3, 40 + k * 40 + r * 3, 3, 3, C_WHITE);
        }
    }
}
