#include <string.h>
#include "pack.h"
#include "ui.h"

static const char *ty_s = "";
static int ty_n, ty_len, ty_wait;

void ty_set(const char *s) { ty_s = s; ty_n = 0; ty_len = (int)strlen(s); ty_wait = 0; }
int ty_done(void) { return ty_n >= ty_len; }
void ty_finish(void) { ty_n = ty_len; }
void ty_step(void)
{
    if (ty_n >= ty_len) return;
    if (ty_wait > 0) { ty_wait--; return; }
    char c = ty_s[ty_n++];
    if (c == ',' || c == '.' || c == '!' || c == '?') ty_wait = 3;
}
void ty_draw(int x, int y) { gfx_text_n(&A.main, x, y, ty_s, ty_n, C_WHITE, 1); }

void ui_box(int x, int y, int w, int h)
{
    gfx_rect(x, y, w, h, C_WHITE);
    gfx_rect(x + 3, y + 3, w - 6, h - 6, C_BLACK);
}

void ui_text_center(const Font *f, int y, const char *s, px_t c, int scale)
{
    gfx_text(f, (SCR_W - gfx_text_w(f, s, scale)) / 2, y, s, c, scale);
}
