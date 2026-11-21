/* Demo tecnica v2: titulo -> salas REALES de las Ruinas / Dark Ruins -> menu (C) con herramientas de prueba.
 * Movimiento, colisiones y puertas salen de los datos del juego (ver tools/uty_pack.py). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle.h"
#include "game.h"
#include "gfx.h"
#include "pack.h"
#include "player.h"
#include "rooms.h"
#include "stats.h"
#include "ui.h"

enum { S_TITLE, S_ROOM, S_MENU, S_BATTLE, S_GAMEOVER };

static int state, tick;
static uint32_t held, pressed, prev_held;
static Room room;
static int room_ok;
static Player pl;
static int camx, camy;
static char names[96][32];
static int nnames;
static int menu_sel, menu_view, warp_sel;
static int fade, door_cd, notice_t, go_timer;
static char notice[72], tb[200], title_err[64];

static void load_index(void)
{
    size_t n = 0;
    uint8_t *b = pack_read("rooms/INDEX.txt", &n);
    nnames = 0;
    if (!b) return;
    size_t i = 0;
    while (i < n && nnames < 96) {
        size_t j = 0;
        while (i < n && b[i] != '\n' && b[i] != '\r') { if (j < 31) names[nnames][j++] = (char)b[i]; i++; }
        names[nnames][j] = 0;
        while (i < n && (b[i] == '\n' || b[i] == '\r')) i++;
        if (j) nnames++;
    }
    free(b);
}

static void update_camera(void)
{
    int mx = room.w - SCR_W, my = room.h - SCR_H;
    if (mx > 0) { camx = pl.x - SCR_W / 2; if (camx < 0) camx = 0; if (camx > mx) camx = mx; } else camx = mx / 2;
    if (my > 0) { camy = pl.y - SCR_H / 2; if (camy < 0) camy = 0; if (camy > my) camy = my; } else camy = my / 2;
}

/* entra en una sala; x,y < 0 = usar la aparicion de la sala (o una posicion libre) */
static int enter_room(const char *name, int x, int y)
{
    Room nr;
    if (!room_load(&nr, name)) return 0;
    if (room_ok) room_free(&room);
    room = nr;
    room_ok = 1;
    if (x < 0 || y < 0) {
        x = room.spawnx; y = room.spawny;
        if (x < 0 || y < 0 || !room_free_at(&room, x + A.mask_x, y + A.mask_y, A.mask_w, A.mask_h)) room_find_free(&room, A.mask_w, A.mask_h, &x, &y);
    }
    pl.x = x; pl.y = y; pl.moving = 0; pl.anim = 0;
    update_camera();
    fade = 2;
    door_cd = 12;
    return 1;
}

static void new_game(void)
{
    stats_reset();
    player_init(&pl, 0, 0, 0);
    if (!enter_room("rm_ruins00", -1, -1)) { snprintf(title_err, sizeof title_err, "FALTA rooms/rm_ruins00.rm"); return; }
    state = S_ROOM;
}

static void room_update(void)
{
    if (notice_t > 0) notice_t--;
    if (fade > 0) { fade--; return; }
    if (pressed & IN_C) { state = S_MENU; menu_sel = 0; menu_view = -1; return; }
    player_update(&pl, &room, held);
    if (door_cd > 0) door_cd--;
    const Door *d = room_door_at(&room, pl.x + A.mask_x, pl.y + A.mask_y, A.mask_w, A.mask_h);
    if (d && door_cd == 0) {
        char target[32];
        snprintf(target, sizeof target, "%s", d->target);
        if (!enter_room(target, d->xx, d->yy)) {
            snprintf(notice, sizeof notice, "SALA NO INCLUIDA: %s", target);
            notice_t = 75; door_cd = 30;
        }
    }
    update_camera();
}

static void menu_update(void)
{
    if (menu_view >= 0) {
        if (menu_view == 1) {                                  /* WARP */
            if (pressed & IN_LEFT) warp_sel = (warp_sel + nnames - 1) % (nnames ? nnames : 1);
            if (pressed & IN_RIGHT) warp_sel = (warp_sel + 1) % (nnames ? nnames : 1);
            if (pressed & IN_UP) warp_sel = (warp_sel + nnames - 5) % (nnames ? nnames : 1);
            if (pressed & IN_DOWN) warp_sel = (warp_sel + 5) % (nnames ? nnames : 1);
            if ((pressed & IN_Z) && nnames) {
                char target[32];
                snprintf(target, sizeof target, "%s", names[warp_sel]);
                if (enter_room(target, -1, -1)) { state = S_ROOM; return; }
            }
        }
        if (pressed & (IN_X | IN_C) || (menu_view != 1 && (pressed & IN_Z))) menu_view = -1;
        return;
    }
    if (pressed & (IN_X | IN_C)) { state = S_ROOM; return; }
    if (pressed & IN_UP) menu_sel = (menu_sel + 2) % 3;
    if (pressed & IN_DOWN) menu_sel = (menu_sel + 1) % 3;
    if (pressed & IN_Z) {
        if (menu_sel == 2) { battle_start(pl.x - camx, pl.y - camy - 10); state = S_BATTLE; }
        else menu_view = menu_sel;
    }
}

static void draw_world(void)
{
    gfx_world(room.pix, room.w, room.h, camx, camy);
    player_draw(&pl, camx, camy);
    if (fade > 0) gfx_clear(C_BLACK);
}

static void draw_menu(void)
{
    ui_box(16, 14, 112, 78);
    gfx_text(&A.main, 26, 20, "ROOM INFO", C_WHITE, 1);
    gfx_text(&A.main, 26, 38, "ROOM WARP", C_WHITE, 1);
    gfx_text(&A.main, 26, 56, "BATTLE TEST", C_WHITE, 1);
    gfx_sprite(&A.soul[0], 24, 20 + 8 + 18 * menu_sel);
    if (menu_view == 0) {
        ui_box(8, 100, 304, 100);
        snprintf(tb, sizeof tb, "%s  %dx%d\nSOLIDS %d  DOORS %d\nPOS %d,%d  DIR %d", room.name, room.w, room.h, room.nsolid, room.ndoor, pl.x, pl.y, pl.dir);
        gfx_text(&A.main, 20, 108, tb, C_WHITE, 1);
    } else if (menu_view == 1) {
        ui_box(8, 100, 304, 100);
        gfx_text(&A.main, 20, 108, "< ROOM WARP >  (Z = go)", C_WHITE, 1);
        gfx_text(&A.main, 20, 132, nnames ? names[warp_sel] : "(no rooms)", C_YELLOW, 1);
        snprintf(tb, sizeof tb, "%d / %d", warp_sel + 1, nnames);
        gfx_text(&A.main, 20, 156, tb, C_GRAY, 1);
    }
}

static void draw_title(void)
{
    gfx_img(&A.splash, 0, 0);
    if ((tick / 15) % 2 == 0) ui_text_center(&A.stats, 186, "PRESS 2 TO START", C_WHITE, 2);
    if (title_err[0]) ui_text_center(&A.stats, 160, title_err, C_RED, 1);
    ui_text_center(&A.stats, 218, "TECH DEMO V2 - REAL RUINS ROOMS", C_YELLOW, 1);
    ui_text_center(&A.stats, 227, "FAN PROJECT - GAME BY TEAM UNDERTALE YELLOW", C_GRAY, 1);
}

static void draw_gameover(void)
{
    gfx_clear(C_BLACK);
    ui_text_center(&A.main, 56, "GAME OVER", C_WHITE, 3);
    ui_text_center(&A.main, 130, "Stay determined...", C_WHITE, 1);
    if (go_timer > 60 && (tick / 15) % 2 == 0) ui_text_center(&A.stats, 190, "PRESS 2 TO TRY AGAIN", C_GRAY, 2);
}

static void render(void)
{
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_ROOM:
        draw_world();
        if (notice_t > 0) { ui_box(16, 196, 288, 32); gfx_text(&A.main, 26, 204, notice, C_WHITE, 1); }
        break;
    case S_MENU: draw_world(); draw_menu(); break;
    case S_BATTLE: battle_draw(); break;
    default: draw_gameover(); break;
    }
}

int game_init(const char *datadir)
{
    state = S_TITLE; tick = 0; prev_held = 0; room_ok = 0; title_err[0] = 0; warp_sel = 0;
    if (!pack_load(datadir)) {
        gfx_error("NO SE ENCUENTRAN LOS DATOS", "COPIA LA CARPETA DATA A", "SD:/APPS/UNDERTALEYELLOW/DATA/");
        return 0;
    }
    load_index();
    player_init(&pl, 0, 0, 0);
    render();
    return 1;
}

void game_tick(uint32_t h)
{
    held = h; pressed = h & ~prev_held; prev_held = h; tick++;
    switch (state) {
    case S_TITLE: if (pressed & IN_Z) new_game(); break;
    case S_ROOM: room_update(); break;
    case S_MENU: menu_update(); break;
    case S_BATTLE: {
        int r = battle_tick(held, pressed);
        if (r == BR_WIN || r == BR_FLEE) state = S_ROOM;
        else if (r == BR_DEAD) { state = S_GAMEOVER; go_timer = 0; }
        break;
    }
    default:
        go_timer++;
        if (go_timer > 60 && (pressed & IN_Z)) new_game();
        break;
    }
    render();
}
