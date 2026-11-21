/* Prueba en PC (sin Wii): ejecuta el nucleo con una "partitura" de botones y guarda capturas PPM.
 *
 *   gcc -O2 -o pc_test tools/pc_test.c source/[!m]*.c -Isource      (todo menos main.c)
 *   ./pc_test data/ 20: 1:Z 1: 60:D snap:captura
 *
 * Argumentos: primero la carpeta de datos; luego "N:TECLAS" (mantener TECLAS N ticks; U D L R Z X C) o "snap:nombre".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "gfx.h"

static void snap(const char *name)
{
    char path[256];
    snprintf(path, sizeof path, "%s.ppm", name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        unsigned c = g_fb[i];
        unsigned r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
        fputc((int)((r << 3) | (r >> 2)), f); fputc((int)((g << 2) | (g >> 4)), f); fputc((int)((b << 3) | (b >> 2)), f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc < 2 || !game_init(argv[1])) { fprintf(stderr, "no se pudo cargar los datos\n"); snap("error"); return 1; }
    for (int a = 2; a < argc; a++) {
        if (!strncmp(argv[a], "snap:", 5)) { snap(argv[a] + 5); continue; }
        char *colon = strchr(argv[a], ':');
        if (!colon) continue;
        int n = atoi(argv[a]);
        uint32_t keys = 0;
        for (char *p = colon + 1; *p; p++)
            switch (*p) {
            case 'U': keys |= IN_UP; break;    case 'D': keys |= IN_DOWN; break;
            case 'L': keys |= IN_LEFT; break;  case 'R': keys |= IN_RIGHT; break;
            case 'Z': keys |= IN_Z; break;     case 'X': keys |= IN_X; break;
            case 'C': keys |= IN_C; break;
            }
        for (int i = 0; i < n; i++) game_tick(keys);
    }
    return 0;
}
