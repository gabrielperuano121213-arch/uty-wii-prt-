#ifndef UI_H
#define UI_H
#include "gfx.h"
void ui_box(int x, int y, int w, int h);
void ui_text_center(const Font *f, int y, const char *s, px_t c, int scale);
void ty_set(const char *s);
int  ty_done(void);
void ty_finish(void);
void ty_step(void);
void ty_draw(int x, int y);
#endif
