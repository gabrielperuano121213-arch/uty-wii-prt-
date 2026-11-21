#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pack.h"
#include "rooms.h"

int room_load(Room *r, const char *name)
{
    char rel[96];
    snprintf(rel, sizeof rel, "rooms/%s.rm", name);
    size_t size = 0;
    uint8_t *b = pack_read(rel, &size);
    memset(r, 0, sizeof *r);
    if (!b || size < 32 || memcmp(b, "UYRM", 4) != 0) { free(b); return 0; }
    r->w = pk_s32(b + 4); r->h = pk_s32(b + 8); r->nsolid = pk_s32(b + 12); r->ndoor = pk_s32(b + 16);
    r->spawnx = pk_s32(b + 20); r->spawny = pk_s32(b + 24);
    size_t need = 32 + (size_t)r->nsolid * 8 + (size_t)r->ndoor * 48 + (size_t)r->w * r->h * 2;
    if (r->w <= 0 || r->h <= 0 || r->nsolid < 0 || r->ndoor < 0 || size < need) { free(b); return 0; }
    snprintf(r->name, sizeof r->name, "%s", name);
    r->solid = (Rect *)malloc(sizeof(Rect) * (size_t)(r->nsolid ? r->nsolid : 1));
    r->door = (Door *)malloc(sizeof(Door) * (size_t)(r->ndoor ? r->ndoor : 1));
    const uint8_t *p = b + 32;
    for (int i = 0; i < r->nsolid; i++, p += 8) {
        r->solid[i].x = pk_s16(p); r->solid[i].y = pk_s16(p + 2); r->solid[i].w = pk_s16(p + 4); r->solid[i].h = pk_s16(p + 6);
    }
    for (int i = 0; i < r->ndoor; i++, p += 48) {
        Door *d = &r->door[i];
        d->r.x = pk_s16(p); d->r.y = pk_s16(p + 2); d->r.w = pk_s16(p + 4); d->r.h = pk_s16(p + 6);
        d->xx = pk_s16(p + 8); d->yy = pk_s16(p + 10);
        memcpy(d->target, p + 16, 31); d->target[31] = 0;
    }
    /* los pixeles estan en little-endian: se convierten en sitio al orden de la maquina (la Wii es big-endian) */
    size_t n = (size_t)r->w * r->h;
    r->pix = (px_t *)(b + (p - b));
    for (size_t i = 0; i < n; i++) {
        const uint8_t *q = (const uint8_t *)&r->pix[i];
        uint16_t v = (uint16_t)(q[0] | (q[1] << 8));
        r->pix[i] = v;
    }
    r->blob = b;
    return 1;
}

void room_free(Room *r)
{
    free(r->solid); free(r->door); free(r->blob);
    memset(r, 0, sizeof *r);
}

int room_free_at(const Room *r, int x, int y, int w, int h)
{
    for (int i = 0; i < r->nsolid; i++) {
        const Rect *s = &r->solid[i];
        if (x < s->x + s->w && s->x < x + w && y < s->y + s->h && s->y < y + h) return 0;
    }
    return 1;
}

const Door *room_door_at(const Room *r, int x, int y, int w, int h)
{
    for (int i = 0; i < r->ndoor; i++) {
        const Rect *s = &r->door[i].r;
        if (x < s->x + s->w && s->x < x + w && y < s->y + s->h && s->y < y + h) return &r->door[i];
    }
    return NULL;
}

int room_find_free(const Room *r, int w, int h, int *ox, int *oy)
{
    int cx = r->w / 2, cy = r->h / 2;
    for (int rad = 0; rad < 600; rad += 4)
        for (int dy = -rad; dy <= rad; dy += 4)
            for (int dx = -rad; dx <= rad; dx += 4) {
                if (rad && dx > -rad && dx < rad && dy > -rad && dy < rad) continue;
                if (room_free_at(r, cx + dx - w / 2, cy + dy - h / 2, w, h)) { *ox = cx + dx; *oy = cy + dy; return 1; }
            }
    *ox = cx; *oy = cy;
    return 0;
}
