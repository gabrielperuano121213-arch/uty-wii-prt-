/* Movimiento de Clover segun obj_pl / scr_normal_state de data.win (velocidad 3, caja de colision spr_pl_mask). */
#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>
#include "rooms.h"

typedef struct { int x, y; int dir; int anim; int moving; } Player;   /* dir: 0 abajo, 1 arriba, 2 izq, 3 der */

void player_init(Player *p, int x, int y, int dir);
void player_update(Player *p, const Room *r, uint32_t held);
void player_draw(const Player *p, int camx, int camy);
int  player_free(const Player *p, const Room *r, int x, int y);

#endif
