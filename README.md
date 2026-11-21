# Undertale Yellow — Demo técnica v2 para Nintendo Wii (Homebrew Channel)

> **Esto es código fuente.** El Homebrew Channel solo lista `SD:/apps/<Nombre>/boot.dol` reales: hay que compilarlo.
> **Los datos del juego van aparte** (carpeta `data/`, generada desde TU copia de Undertale Yellow) y **no se suben al repositorio**.

## Qué es
Motor nativo (C, devkitPPC/libogc) que carga **salas reales** de las Ruinas y Dark Ruins leídas de `data.win`:
imagen de cada sala, **colisiones**, **puertas con su destino y posición de aparición** (leídas del bytecode del juego),
movimiento de Clover con la velocidad y la máscara reales (`plspd = 3`, `spr_pl_mask`) y animación a 0,2 por paso.
35 salas: `rm_ruins00`–`04`, `rm_darkruins_01`–`19` (+ variantes) y las de Dalv.

Incluye herramientas de prueba en el menú (botón **B**): **ROOM INFO** (sala, posición), **ROOM WARP** (saltar a cualquier sala
con ←/→, Z para ir) y **BATTLE TEST** (el combate de prueba del demo v1 con el alma amarilla).

**Aún NO incluye:** diálogos, NPCs, puzzles, combates reales, estadísticas, rutas, tienda, guardado ni audio.
Algunas salas pueden verse incompletas (suelos o efectos que el juego dibuja por código, p. ej. `rm_ruins01`) y las
pendientes diagonales aún no están (ver `docs/`).

## Controles (Wiimote HORIZONTAL, como un mando de NES)
Cruceta = mover · **2 = Z** (confirmar) · **1 = X** (cancelar) · **B = C** (menú) · **HOME** = salir al Homebrew Channel.

## 1) Generar los datos (una vez, en tu PC)
```
pip install pillow numpy
python3 tools/uty_pack.py --data data.win --splash splash.png --out data
```
Crea `data/core.pak` y `data/rooms/*.rm` (~15 MB). **No los subas a GitHub.**

## 2) Compilar `boot.dol`
GitHub Actions (subir el código **sin** la carpeta `data/`), Docker (`compilar_docker.bat` / `./compilar_docker.sh`) o devkitPro (`make package`).

## 3) Instalar en la SD (FAT32)
```
SD:/apps/UndertaleYellow/boot.dol
SD:/apps/UndertaleYellow/meta.xml
SD:/apps/UndertaleYellow/icon.png
SD:/apps/UndertaleYellow/data/core.pak
SD:/apps/UndertaleYellow/data/rooms/*.rm  (+ INDEX.txt)
```
Si faltan los datos, el juego lo avisa en pantalla. Verifica la carpeta con `python3 tools/verificar.py <ruta>`.

## Estructura
`source/` main (Wii), game, gfx (RGB565), pack, rooms, player, battle, ui, stats · `tools/` uty_pack, uty_rooms, gmdis (desensamblador), verificar, pc_test · `docs/` informes.
Prueba en PC: `gcc -O2 -o pc_test tools/pc_test.c source/[!m]*.c -Isource && ./pc_test data/ 20: 1:Z 1: 60:D snap:captura`.

Juego original: Team Undertale Yellow (fan game gratuito). Proyecto de fans, uso personal.
