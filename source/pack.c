#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pack.h"

Assets A;
static char g_dir[256];

int16_t pk_s16(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
uint16_t pk_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
int32_t pk_s32(const uint8_t *p) { return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24)); }
static uint32_t pk_u32(const uint8_t *p) { return (uint32_t)pk_s32(p); }

const char *pack_dir(void) { return g_dir; }

uint8_t *pack_read(const char *rel, size_t *size)
{
    char path[320];
    snprintf(path, sizeof path, "%s%s", g_dir, rel);
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return NULL; }
    fclose(f);
    if (size) *size = (size_t)n;
    return buf;
}

static int load_img(Img *im, const uint8_t *b, uint32_t size)
{
    if (size < 8) return 0;
    im->w = pk_s16(b); im->h = pk_s16(b + 2); im->ox = pk_s16(b + 4); im->oy = pk_s16(b + 6);
    size_t n = (size_t)im->w * (size_t)im->h;
    if (n == 0 || size < 8 + n * 2) return 0;
    im->px = (px_t *)malloc(n * sizeof(px_t));
    if (!im->px) return 0;
    for (size_t i = 0; i < n; i++) im->px[i] = pk_u16(b + 8 + 2 * i);
    return 1;
}

static int load_font(Font *f, const uint8_t *b, uint32_t size)
{
    if (size < 1 + 95 * 6 + 4) return 0;
    f->lineh = b[0];
    for (int i = 0; i < 95; i++) {
        const uint8_t *g = b + 1 + 6 * i;
        f->g[i].w = g[0]; f->g[i].h = g[1]; f->g[i].xo = (int8_t)g[2]; f->g[i].adv = g[3]; f->g[i].off = pk_u16(g + 4);
    }
    uint32_t n = pk_u32(b + 1 + 570);
    if (size < 1 + 570 + 4 + n * 2) return 0;
    f->rows = (uint16_t *)malloc((n ? n : 1) * sizeof(uint16_t));
    if (!f->rows) return 0;
    for (uint32_t i = 0; i < n; i++) f->rows[i] = pk_u16(b + 1 + 570 + 4 + 2 * i);
    return 1;
}

int pack_load(const char *dir)
{
    snprintf(g_dir, sizeof g_dir, "%s", dir);
    size_t n = strlen(g_dir);
    if (n && g_dir[n - 1] != '/') { g_dir[n] = '/'; g_dir[n + 1] = 0; }
    size_t size = 0;
    uint8_t *buf = pack_read("core.pak", &size);
    if (!buf || size < 12 || memcmp(buf, "UYPK", 4) != 0) { free(buf); return 0; }
    uint32_t count = pk_u32(buf + 8);
    static const char *DIRS[4] = { "down", "up", "left", "right" };
    static const char *BTN[4] = { "btn_fight_", "btn_act_", "btn_item_", "btn_mercy_" };
    memset(&A, 0, sizeof A);
    int got = 0;
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *e = buf + 12 + 36 * i;
        char name[25];
        memcpy(name, e, 24); name[24] = 0;
        uint32_t kind = pk_u32(e + 24), off = pk_u32(e + 28), sz = pk_u32(e + 32);
        if (off + sz > size) continue;
        const uint8_t *b = buf + off;
        if (kind == 1) {
            Img *dst = NULL;
            for (int d = 0; d < 4 && !dst; d++) {
                char pre[32];
                snprintf(pre, sizeof pre, "clover_%s_", DIRS[d]);
                if (!strncmp(name, pre, strlen(pre))) {
                    int k = atoi(name + strlen(pre));
                    if (k >= 0 && k < 4) { dst = &A.clover[d][k]; if (k + 1 > A.clover_n[d]) A.clover_n[d] = k + 1; }
                }
            }
            for (int d = 0; d < 4 && !dst; d++)
                if (!strncmp(name, BTN[d], strlen(BTN[d]))) { int k = atoi(name + strlen(BTN[d])); if (k >= 0 && k < 2) dst = &A.btn[d][k]; }
            if (!dst && !strncmp(name, "soul_", 5)) { int k = atoi(name + 5); if (k >= 0 && k < 2) dst = &A.soul[k]; }
            if (!dst && !strncmp(name, "shot_", 5)) { int k = atoi(name + 5); if (k >= 0 && k < 5) dst = &A.shot[k]; }
            if (!dst && !strcmp(name, "dummy_0")) dst = &A.dummy;
            if (!dst && !strcmp(name, "splash")) dst = &A.splash;
            if (dst && load_img(dst, b, sz)) got++;
        } else if (kind == 2) {
            if (!strcmp(name, "main") && load_font(&A.main, b, sz)) got++;
            if (!strcmp(name, "stats") && load_font(&A.stats, b, sz)) got++;
        } else if (kind == 4 && !strcmp(name, "pl_mask") && sz >= 8) {
            A.mask_x = pk_s16(b); A.mask_y = pk_s16(b + 2); A.mask_w = pk_s16(b + 4); A.mask_h = pk_s16(b + 6);
            got++;
        }
    }
    free(buf);
    return A.clover[0][0].px && A.main.rows && A.stats.rows && A.splash.px && A.mask_w > 0 && got > 20;
}
