#include <stdio.h>
#include <string.h>
#include "battle.h"
#include "game.h"
#include "gfx.h"
#include "pack.h"
#include "stats.h"
#include "ui.h"

enum { B_INTRO, B_MENU, B_FIGHT, B_ATTACK, B_STRIKE, B_ACT, B_ITEM, B_MERCY, B_TEXT, B_SHRINK, B_DODGE, B_GROW };
enum { NEXT_TURN, NEXT_WIN, NEXT_FLEE };

#define E_HPMAX 30
#define BOX_X 19
#define BOX_Y 128
#define BOX_W 282
#define BOX_H 64
#define DODGE_X 128
#define DODGE_W 64
#define BTN_Y 216
static const int BTN_X[4] = { 16, 92, 172, 250 };

static unsigned rng_s = 0x2545F491u;
static int rnd(int n) { rng_s = rng_s * 1664525u + 1013904223u; return (int)((rng_s >> 12) % (unsigned)n); }
static int iabs(int v) { return v < 0 ? -v : v; }

static int bst, bsel, bsub, btimer, next_after;
static int e_hp, e_gone, spareable, turn_no;
static int bx, by, bw, bh, sx, sy, inv, cd, bar_x, dmg, shake, dodge_t;
static int intro_x, intro_y;
typedef struct { int x, y, vx, vy, on; } Obj;
static Obj pel[24], shot[6];
static char tb[128], msg[96];

static const char *const FLAVOR[3] = {
    "* The dummy stands there,\n  doing dummy things.",
    "* Smells like cotton and\n  determination.",
    "* The dummy is... staring?"
};

void battle_start(int px, int py)
{
    bst = B_INTRO; btimer = 0; e_hp = E_HPMAX; e_gone = 0; spareable = 0; turn_no = 0; bsel = 0;
    bx = BOX_X; by = BOX_Y; bw = BOX_W; bh = BOX_H; shake = 0; inv = 0; cd = 0; intro_x = px; intro_y = py;
    memset(pel, 0, sizeof pel); memset(shot, 0, sizeof shot);
}

static void btext(const char *s, int next) { ty_set(s); next_after = next; bst = B_TEXT; }

static void strike(int d)
{
    dmg = d; bst = B_STRIKE; btimer = 0;
    e_hp -= d; if (e_hp < 0) e_hp = 0;
    shake = d > 0 ? 20 : 0;
}

static int dodge_update(uint32_t held, uint32_t pressed)
{
    int sp = (held & IN_X) ? 1 : 2;
    int dx = !!(held & IN_RIGHT) - !!(held & IN_LEFT), dy = !!(held & IN_DOWN) - !!(held & IN_UP);
    sx += dx * sp; sy += dy * sp;
    if (sx < bx + 5) sx = bx + 5;
    if (sx > bx + bw - 5) sx = bx + bw - 5;
    if (sy < by + 5) sy = by + 5;
    if (sy > by + bh - 5) sy = by + bh - 5;
    if (inv > 0) inv--;
    if (cd > 0) cd--;
    if ((pressed & IN_Z) && cd == 0) {
        for (int i = 0; i < 6; i++)
            if (!shot[i].on) { shot[i].x = sx * 16; shot[i].y = (sy - 6) * 16; shot[i].vx = 0; shot[i].vy = -56; shot[i].on = 1; cd = 7; break; }
    }
    if (dodge_t > 14 && dodge_t % 9 == 0) {
        for (int i = 0; i < 24; i++)
            if (!pel[i].on) {
                pel[i].x = (bx + 6 + rnd(bw - 12)) * 16; pel[i].y = (by + 1) * 16;
                pel[i].vx = (rnd(7) - 3) * 2; pel[i].vy = 24 + rnd(16); pel[i].on = 1; break;
            }
    }
    for (int i = 0; i < 6; i++) if (shot[i].on) {
        shot[i].y += shot[i].vy;
        if (shot[i].y < (by - 6) * 16) shot[i].on = 0;
    }
    for (int i = 0; i < 24; i++) if (pel[i].on) {
        pel[i].x += pel[i].vx; pel[i].y += pel[i].vy;
        int x = pel[i].x / 16, y = pel[i].y / 16;
        if (y > by + bh + 4 || x < bx - 4 || x > bx + bw + 4) { pel[i].on = 0; continue; }
        for (int j = 0; j < 6; j++)
            if (shot[j].on) {
                int ddx = shot[j].x / 16 - x, ddy = shot[j].y / 16 - y;
                if (ddx * ddx + ddy * ddy < 36) { shot[j].on = 0; pel[i].on = 0; break; }
            }
        if (pel[i].on && inv == 0 && iabs(x - sx) < 5 && iabs(y - sy) < 5) {
            pel[i].on = 0; g_hp -= 3; inv = 30;
            if (g_hp <= 0) { g_hp = 0; return BR_DEAD; }
        }
    }
    if (--dodge_t <= 0) {
        memset(pel, 0, sizeof pel); memset(shot, 0, sizeof shot);
        bst = B_GROW; btimer = 0;
    }
    return BR_RUNNING;
}

int battle_tick(uint32_t held, uint32_t pressed)
{
    switch (bst) {
    case B_INTRO:
        if (++btimer >= 36) { bst = B_MENU; ty_set("* TRAINING DUMMY blocks the\n  way!"); }
        break;
    case B_MENU:
        ty_step();
        if (pressed & IN_LEFT)  bsel = (bsel + 3) & 3;
        if (pressed & IN_RIGHT) bsel = (bsel + 1) & 3;
        if (pressed & IN_Z) {
            bsub = 0;
            if (bsel == 0) bst = B_FIGHT;
            else if (bsel == 1) bst = B_ACT;
            else if (bsel == 2) { if (g_snacks > 0) bst = B_ITEM; }
            else bst = B_MERCY;
        }
        break;
    case B_FIGHT:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_Z) { bst = B_ATTACK; bar_x = 22; }
        break;
    case B_ATTACK:
        bar_x += 6;
        if (pressed & IN_Z) { int d = iabs(bar_x - 160); strike(d > 140 ? 0 : 1 + (140 - d) * 19 / 140); }
        else if (bar_x > 298) strike(0);
        break;
    case B_STRIKE:
        if (shake > 0) shake--;
        if (++btimer >= 32) {
            if (e_hp <= 0) { e_gone = 1; btext("* YOU WON!\n* You earned 0 EXP and 0 gold.", NEXT_WIN); }
            else { bst = B_SHRINK; btimer = 0; }
        }
        break;
    case B_ACT:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_UP) bsub = 0;
        else if (pressed & IN_DOWN) bsub = 1;
        else if (pressed & IN_Z) {
            if (bsub == 0) btext("* TRAINING DUMMY - ATK 0 DEF 0\n* A fake foe for practice.\n* Dodge its pellets with care.", NEXT_TURN);
            else { spareable = 1; btext("* You cheer for the dummy.\n* It seems... content.", NEXT_TURN); }
        }
        break;
    case B_ITEM:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_Z) {
            int before = g_hp;
            g_snacks--; g_hp += 10; if (g_hp > g_hpmax) g_hp = g_hpmax;
            if (g_hp == before) snprintf(msg, sizeof msg, "* You ate the SNACK.\n* Your HP was maxed out.");
            else snprintf(msg, sizeof msg, "* You ate the SNACK.\n* You recovered %d HP.", g_hp - before);
            btext(msg, NEXT_TURN);
        }
        break;
    case B_MERCY:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_UP) bsub = 0;
        else if (pressed & IN_DOWN) bsub = 1;
        else if (pressed & IN_Z) {
            if (bsub == 1) btext("* Got away safely...", NEXT_FLEE);
            else if (spareable) { e_gone = 1; btext("* YOU WON!\n* You earned 0 EXP and 0 gold.", NEXT_WIN); }
            else btext("* You offer mercy, but the\n  dummy isn't ready.", NEXT_TURN);
        }
        break;
    case B_TEXT:
        ty_step();
        if (held & IN_X) ty_step();
        if ((pressed & IN_Z) && ty_done()) {
            if (next_after == NEXT_TURN) { bst = B_SHRINK; btimer = 0; }
            else return next_after == NEXT_WIN ? BR_WIN : BR_FLEE;
        }
        break;
    case B_SHRINK: {
        int t = ++btimer;
        bx = BOX_X + (DODGE_X - BOX_X) * t / 6; bw = BOX_W + (DODGE_W - BOX_W) * t / 6;
        if (t >= 6) { bst = B_DODGE; dodge_t = 150; sx = bx + bw / 2; sy = by + bh / 2; inv = 0; cd = 0; }
        break;
    }
    case B_DODGE:
        return dodge_update(held, pressed);
    case B_GROW: {
        int t = ++btimer;
        bx = DODGE_X + (BOX_X - DODGE_X) * t / 6; bw = DODGE_W + (BOX_W - DODGE_W) * t / 6;
        if (t >= 6) { bst = B_MENU; turn_no++; ty_set(FLAVOR[turn_no % 3]); }
        break;
    }
    }
    return BR_RUNNING;
}

static void draw_hp_row(void)
{
    gfx_text(&A.stats, 16, 199, "CLOVER", C_WHITE, 2);
    gfx_text(&A.stats, 96, 199, "LV 1", C_WHITE, 2);
    gfx_text(&A.stats, 150, 203, "HP", C_WHITE, 1);
    gfx_rect(168, 198, g_hpmax, 11, C_RED);
    if (g_hp > 0) gfx_rect(168, 198, g_hp, 11, C_YELLOW);
    snprintf(tb, sizeof tb, "%d / %d", g_hp, g_hpmax);
    gfx_text(&A.stats, 168 + g_hpmax + 8, 199, tb, C_WHITE, 2);
}

static void list_row(int i, const char *s, px_t c)
{
    gfx_text(&A.main, bx + 28, by + 8 + 18 * i, s, c, 1);
    if (i == bsub) gfx_sprite(&A.soul[0], bx + 17, by + 8 + 18 * i + 8);
}

void battle_draw(void)
{
    gfx_clear(C_BLACK);
    if (bst == B_INTRO) {
        int t = btimer, x = intro_x, y = intro_y;
        if (t < 18) { if ((t / 3) % 2 == 0) gfx_sprite(&A.soul[0], x, y); }
        else {
            int k = t - 18, tx = BTN_X[0] + 8, ty = BTN_Y + 11;
            gfx_sprite(&A.soul[0], x + (tx - x) * k / 18, y + (ty - y) * k / 18);
        }
        return;
    }
    if (!e_gone) gfx_sprite(&A.dummy, 160 + (shake > 0 ? ((shake & 1) ? 4 : -4) : 0), 112);
    gfx_rect(bx - 3, by - 3, bw + 6, bh + 6, C_WHITE);
    gfx_rect(bx, by, bw, bh, C_BLACK);
    switch (bst) {
    case B_MENU: case B_TEXT: ty_draw(bx + 12, by + 10); break;
    case B_FIGHT: list_row(0, "* TRAINING DUMMY", spareable ? C_YELLOW : C_WHITE); break;
    case B_ACT: list_row(0, "* Check", C_WHITE); list_row(1, "* Cheer", C_WHITE); break;
    case B_ITEM: list_row(0, "* Snack", C_WHITE); break;
    case B_MERCY: list_row(0, "* Spare", spareable ? C_YELLOW : C_WHITE); list_row(1, "* Flee", C_WHITE); break;
    case B_ATTACK: case B_STRIKE:
        gfx_rect(bx + 6, by + 8, bw - 12, bh - 16, C_WHITE);
        gfx_rect(bx + 9, by + 11, bw - 18, bh - 22, C_BLACK);
        gfx_rect(159, by + 11, 2, bh - 22, C_YELLOW);
        gfx_rect(bar_x - 3, by + 5, 7, bh - 10, C_BLACK);
        gfx_rect(bar_x - 2, by + 6, 5, bh - 12, (bst == B_STRIKE && (btimer / 2) % 2) ? C_YELLOW : C_WHITE);
        break;
    case B_DODGE:
        for (int i = 0; i < 6; i++) if (shot[i].on) gfx_sprite(&A.shot[3], shot[i].x / 16, shot[i].y / 16);
        for (int i = 0; i < 24; i++) if (pel[i].on) {
            int x = pel[i].x / 16, y = pel[i].y / 16;
            gfx_rect(x - 1, y - 3, 3, 7, C_WHITE); gfx_rect(x - 3, y - 1, 7, 3, C_WHITE); gfx_rect(x - 2, y - 2, 5, 5, C_WHITE);
        }
        gfx_sprite(&A.soul[(inv > 0 && (inv / 2) % 2) ? 1 : 0], sx, sy);
        break;
    default: break;
    }
    if (bst == B_STRIKE && btimer > 6) {
        if (dmg > 0) snprintf(tb, sizeof tb, "%d", dmg); else snprintf(tb, sizeof tb, "MISS");
        gfx_text(&A.stats, (SCR_W - gfx_text_w(&A.stats, tb, 3)) / 2, 36, tb, dmg > 0 ? C_RED : C_LGRAY, 3);
        gfx_rect(130, 62, 60, 8, C_GRAY);
        gfx_rect(130, 62, 60 * e_hp / E_HPMAX, 8, C_GREEN);
    }
    draw_hp_row();
    {
        int active = (bst == B_MENU || bst == B_FIGHT || bst == B_ACT || bst == B_ITEM || bst == B_MERCY || bst == B_TEXT);
        for (int i = 0; i < 4; i++) gfx_img(&A.btn[i][(active && i == bsel) ? 1 : 0], BTN_X[i], BTN_Y);
        if (bst == B_MENU) gfx_sprite(&A.soul[0], BTN_X[bsel] + 8, BTN_Y + 11);
    }
}
