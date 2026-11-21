#include "game.h"
#include "gfx.h"
#include "pack.h"
#include "player.h"

#define PL_SPEED 3                                   /* plspd = 3 (obj_pl Create) */

static int sgn(int v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }

void player_init(Player *p, int x, int y, int dir)
{
    p->x = x; p->y = y; p->dir = dir; p->anim = 0; p->moving = 0;
}

int player_free(const Player *p, const Room *r, int x, int y)
{
    (void)p;
    return room_free_at(r, x + A.mask_x, y + A.mask_y, A.mask_w, A.mask_h);
}

void player_update(Player *p, const Room *r, uint32_t held)
{
    int rt = !!(held & IN_RIGHT), lt = !!(held & IN_LEFT), dn = !!(held & IN_DOWN), up = !!(held & IN_UP);
    int hsp = sgn(rt - 2 * lt) * PL_SPEED;           /* igual que scr_normal_state: izquierda/arriba ganan */
    int vsp = sgn(dn - 2 * up) * PL_SPEED;
    /* ayuda en esquinas (aproximacion de scr_normal_state): si el paso esta bloqueado pero libre 2 px mas arriba/abajo, se desplaza 2 px */
    if (hsp && !player_free(p, r, p->x + hsp, p->y) && vsp == 0) {
        if (player_free(p, r, p->x + hsp, p->y - 2)) vsp = -2;
        else if (player_free(p, r, p->x + hsp, p->y + 2)) vsp = 2;
    }
    if (vsp && !player_free(p, r, p->x, p->y + vsp) && hsp == 0) {
        if (player_free(p, r, p->x + 2, p->y + vsp)) hsp = 2;
        else if (player_free(p, r, p->x - 2, p->y + vsp)) hsp = -2;
    }
    int ox = p->x, oy = p->y;
    if (hsp) {                                       /* movimiento hasta el contacto, por eje */
        if (!player_free(p, r, p->x + hsp, p->y)) {
            while (player_free(p, r, p->x + sgn(hsp), p->y)) p->x += sgn(hsp);
            hsp = 0;
        }
        p->x += hsp;
    }
    if (vsp) {
        if (!player_free(p, r, p->x, p->y + vsp)) {
            while (player_free(p, r, p->x, p->y + sgn(vsp))) p->y += sgn(vsp);
            vsp = 0;
        }
        p->y += vsp;
    }
    if (rt || lt) p->dir = lt ? 2 : 3;
    else if (up || dn) p->dir = up ? 1 : 0;
    p->moving = (p->x != ox || p->y != oy);
    if (p->moving) p->anim++; else p->anim = 0;      /* image_speed 0,2 al andar: 5 ticks por frame */
}

void player_draw(const Player *p, int camx, int camy)
{
    int n = A.clover_n[p->dir] ? A.clover_n[p->dir] : 1;
    const Img *im = &A.clover[p->dir][p->moving ? (p->anim / 5) % n : 0];
    gfx_img(im, p->x - im->ox - camx, p->y - im->oy - camy);
}
