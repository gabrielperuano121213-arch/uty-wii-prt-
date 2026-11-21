/* Combate de PRUEBA con el alma amarilla (heredado de la demo v1). El combate fiel de Undertale Yellow
 * se reimplementara en la Etapa 4 a partir del bytecode del juego. */
#ifndef BATTLE_H
#define BATTLE_H
#include <stdint.h>
enum { BR_RUNNING = 0, BR_WIN = 1, BR_FLEE = 2, BR_DEAD = 3 };
void battle_start(int px, int py);                      /* px,py = posicion en pantalla del alma al empezar */
int  battle_tick(uint32_t held, uint32_t pressed);
void battle_draw(void);
#endif
