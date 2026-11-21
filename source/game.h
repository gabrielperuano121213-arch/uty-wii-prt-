/* Nucleo del juego: logica + dibujo, sin dependencias de la consola (se prueba tambien en PC). */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

/* Undertale solo usa cruceta + 3 botones */
enum {
    IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8,
    IN_Z = 16,   /* confirmar / disparar   -> Wiimote 2 */
    IN_X = 32,   /* cancelar               -> Wiimote 1 */
    IN_C = 64    /* menu                   -> Wiimote B */
};

#define GAME_TICKS_PER_SEC 30     /* igual que Undertale */

int  game_init(const char *datadir);   /* 1 = correcto; 0 = faltan los datos (ver gfx_error) */
void game_tick(uint32_t held);         /* actualiza y dibuja un frame en g_fb (gfx.h) */

#endif
