/* Undertale Yellow - demo tecnica v2 para Wii (Homebrew Channel)
 *
 * Capa de plataforma: video (XFB), tarjeta SD (libfat), Wiimote horizontal y bucle a 30 ticks/s.
 *   Cruceta = mover | Boton 2 = Z | Boton 1 = X | Boton B = C (menu) | HOME = volver al Homebrew Channel
 */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "game.h"
#include "gfx.h"

static GXRModeObj *rmode;
static void *xfb[2];
static int cur_fb;
static uint32_t lut[65536];                      /* RGB565 -> YCbYCr (2 pixeles iguales) */

static uint32_t make_yuy2(int r, int g, int b)
{
    int y  = ((  66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
    int cb = (( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
    int cr = (( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
    if (y < 16) y = 16;
    if (y > 235) y = 235;
    if (cb < 16) cb = 16;
    if (cb > 240) cb = 240;
    if (cr < 16) cr = 16;
    if (cr > 240) cr = 240;
    return ((uint32_t)y << 24) | ((uint32_t)cb << 16) | ((uint32_t)y << 8) | (uint32_t)cr;
}

static void build_lut(void)
{
    for (uint32_t c = 0; c < 65536; c++) {
        int r = (int)((c >> 11) & 31), g = (int)((c >> 5) & 63), b = (int)(c & 31);
        lut[c] = make_yuy2((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
    }
}

/* 320x240 RGB565 -> XFB 640x480 (x2), centrado si el modo de video es mas alto (PAL) */
static void present(void)
{
    uint32_t *dst = (uint32_t *)xfb[cur_fb];
    const int stride = (int)VIDEO_PadFramebufferWidth(rmode->fbWidth) / 2;
    const int lines = rmode->xfbHeight;
    const int x0 = (stride - SCR_W) / 2 > 0 ? (stride - SCR_W) / 2 : 0;
    int y0 = (lines - 2 * SCR_H) / 2;
    if (y0 < 0) y0 = 0;
    const px_t *src = g_fb;
    for (int y = 0; y < SCR_H; y++) {
        if (y0 + 2 * y + 1 >= lines) break;
        uint32_t *l0 = dst + (y0 + 2 * y) * stride + x0;
        uint32_t *l1 = l0 + stride;
        for (int x = 0; x < SCR_W; x++) {
            uint32_t v = lut[src[x]];
            l0[x] = v;
            l1[x] = v;
        }
        src += SCR_W;
    }
    VIDEO_SetNextFramebuffer(xfb[cur_fb]);
    VIDEO_Flush();
    cur_fb ^= 1;
}

/* Wiimote HORIZONTAL (estilo NES): la cruceta gira 90 grados. Sin modo vertical. */
static uint32_t map_buttons(uint32_t h)
{
    uint32_t in = 0;
    if (h & WPAD_BUTTON_RIGHT) in |= IN_UP;
    if (h & WPAD_BUTTON_LEFT)  in |= IN_DOWN;
    if (h & WPAD_BUTTON_UP)    in |= IN_LEFT;
    if (h & WPAD_BUTTON_DOWN)  in |= IN_RIGHT;
    if (h & WPAD_BUTTON_2) in |= IN_Z;           /* Z = boton 2 */
    if (h & WPAD_BUTTON_1) in |= IN_X;           /* X = boton 1 */
    if (h & WPAD_BUTTON_B) in |= IN_C;           /* C = boton B */
    return in;
}

/* carpeta de datos: la de boot.dol + "data/" (HBC pasa argv[0] = "sd:/apps/UndertaleYellow/boot.dol") */
static void data_dir(int argc, char **argv, char *out, size_t n)
{
#ifdef UYW_DATA_DIR
    (void)argc; (void)argv;
    snprintf(out, n, "%s", UYW_DATA_DIR);
#else
    snprintf(out, n, "sd:/apps/UndertaleYellow/data/");
    if (argc > 0 && argv && argv[0] && strchr(argv[0], ':')) {
        snprintf(out, n, "%s", argv[0]);
        char *slash = strrchr(out, '/');
        if (slash) { slash[1] = 0; strncat(out, "data/", n - strlen(out) - 1); }
        else snprintf(out, n, "sd:/apps/UndertaleYellow/data/");
    }
#endif
}

int main(int argc, char **argv)
{
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(rmode);
    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    build_lut();
    int sd = fatInitDefault();
    char dir[256];
    data_dir(argc, argv, dir, sizeof dir);
    int ok = sd && game_init(dir);
    if (!sd) gfx_error("NO SE PUDO LEER LA TARJETA SD", "USA FAT32 Y COPIA LA CARPETA", "DATA EN APPS/UNDERTALEYELLOW/");

    uint32_t acc = 0;
    uint64_t next = ticks_to_microsecs(gettime());
    const uint64_t step = 1000000ULL / GAME_TICKS_PER_SEC;
    int dirty = 1;

    for (;;) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;       /* vuelve al Homebrew Channel */
        uint32_t in = map_buttons(WPAD_ButtonsHeld(0));
        acc |= in;
        uint64_t now = ticks_to_microsecs(gettime());
        int n = 0;
        while (ok && now >= next && n < 4) {
            game_tick(acc);
            acc = in;
            next += step;
            n++;
            dirty = 1;
        }
        if (now >= next + 250000) next = now;
        if (dirty) { present(); dirty = 0; }
        VIDEO_WaitVSync();
    }
    return 0;
}
