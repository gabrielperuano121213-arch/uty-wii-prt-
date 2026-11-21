/* Paquete de datos (SD:/apps/UndertaleYellow/data/): sprites, fuentes y salas generados desde TU copia del juego. */
#ifndef PACK_H
#define PACK_H
#include <stddef.h>
#include <stdint.h>
#include "gfx.h"

typedef struct {
    Img clover[4][4]; int clover_n[4];             /* [direccion: 0 abajo, 1 arriba, 2 izq, 3 der][frame] */
    Img soul[2], shot[5], btn[4][2], dummy, splash;
    Font main, stats;
    int mask_x, mask_y, mask_w, mask_h;            /* mascara de colision del jugador (relativa a su origen) */
} Assets;

extern Assets A;

int pack_load(const char *dir);                    /* 1 = correcto */
const char *pack_dir(void);
uint8_t *pack_read(const char *rel, size_t *size); /* lee un archivo del paquete; liberar con free() */
int16_t pk_s16(const uint8_t *p);
uint16_t pk_u16(const uint8_t *p);
int32_t pk_s32(const uint8_t *p);

#endif
