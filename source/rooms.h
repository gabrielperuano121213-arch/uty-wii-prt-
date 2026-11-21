/* Salas reales de Undertale Yellow: imagen RGB565 + colisiones solidas + puertas (con destino y aparicion). */
#ifndef ROOMS_H
#define ROOMS_H
#include <stdint.h>
#include "gfx.h"

typedef struct { int16_t x, y, w, h; } Rect;
typedef struct { Rect r; int16_t xx, yy; char target[32]; } Door;
typedef struct {
    char name[32];
    int w, h, nsolid, ndoor, spawnx, spawny;
    Rect *solid;
    Door *door;
    px_t *pix;
    uint8_t *blob;
} Room;

int  room_load(Room *r, const char *name);            /* 1 = correcto */
void room_free(Room *r);
int  room_free_at(const Room *r, int x, int y, int w, int h);     /* 1 si la caja no toca ningun solido */
const Door *room_door_at(const Room *r, int x, int y, int w, int h);
int  room_find_free(const Room *r, int w, int h, int *ox, int *oy);   /* busca una posicion libre cerca del centro */

#endif
