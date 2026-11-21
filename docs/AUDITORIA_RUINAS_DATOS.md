# Datos de las Ruinas y Dark Ruins (extraídos de tu `data.win`)

Generado automáticamente: cuenta de instancias colocadas en cada sala según el nombre de su objeto. Es evidencia de *qué hay en cada sala*, no de *cómo se comporta* (eso está en el bytecode).

| Sala | Tamaño | Instancias | guardado (obj_determination) | palancas | placas de presión | pinchos | enc. aleatorios | enc. guionizados | Decibat | Dalv | Flowey | carteles/interact. | puertas/salidas | agua |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| rm_darkruins_01 | 500x360 | 25 | · | · | · | · | · | · | · | · | · | 1 | 1 | · |
| rm_darkruins_02 | 1024x840 | 5 | · | · | · | · | · | · | · | · | · | · | 2 | · |
| rm_darkruins_03 | 320x460 | 17 | · | · | · | · | · | · | · | · | 1 | · | 2 | · |
| rm_darkruins_04 | 320x240 | 25 | · | 3 | · | 6 | 1 | 1 | · | · | · | 2 | 2 | · |
| rm_darkruins_05 | 320x800 | 30 | · | · | · | · | · | · | · | · | · | · | 2 | · |
| rm_darkruins_06 | 320x320 | 28 | · | 1 | · | 2 | 1 | · | · | · | · | 2 | 2 | 2 |
| rm_darkruins_07 | 1100x300 | 103 | · | · | · | · | · | · | · | 1 | · | 3 | 2 | 50 |
| rm_darkruins_08 | 520x420 | 22 | (460,230) | · | · | · | 1 | 1 | · | · | · | · | 3 | · |
| rm_darkruins_08b | 320x240 | 9 | · | · | · | · | · | · | · | · | · | · | 1 | · |
| rm_darkruins_09 | 680x520 | 61 | · | 1 | · | 2 | 1 | 1 | · | · | · | 1 | 3 | · |
| rm_darkruins_09b | 320x400 | 18 | · | · | · | · | · | · | · | · | · | · | 1 | · |
| rm_darkruins_10 | 760x640 | 56 | (500,580) | 1 | · | 6 | 1 | 1 | · | 1 | · | 7 | 3 | · |
| rm_darkruins_11 | 320x1200 | 26 | · | · | · | · | · | · | 2 | · | · | · | 2 | · |
| rm_darkruins_12 | 1080x500 | 42 | (220,180) | 1 | · | 3 | 1 | 1 | · | · | · | · | 2 | · |
| rm_darkruins_13 | 520x300 | 11 | · | · | · | · | 1 | · | · | · | · | · | 1 | · |
| rm_darkruins_14 | 540x900 | 32 | · | · | · | · | · | · | · | · | · | 3 | 3 | · |
| rm_darkruins_14b | 940x300 | 18 | · | · | · | · | · | · | · | · | · | · | 1 | · |
| rm_darkruins_15 | 540x240 | 16 | · | · | · | · | · | · | 2 | · | · | 1 | 2 | · |
| rm_darkruins_16 | 700x580 | 72 | (160,120) | · | · | · | · | · | · | 1 | · | 2 | 4 | 31 |
| rm_darkruins_16b | 320x420 | 19 | · | 1 | · | · | · | · | · | · | · | · | 1 | · |
| rm_darkruins_17 | 320x240 | 15 | · | · | · | · | · | · | · | · | · | 2 | 2 | · |
| rm_darkruins_18 | 320x595 | 22 | · | · | · | · | · | · | · | 1 | · | · | 2 | 16 |
| rm_darkruins_19 | 320x600 | 12 | · | · | · | · | · | · | · | · | · | · | 4 | · |
| rm_darkruinsintro | 640x480 | 2 | · | · | · | · | · | · | · | · | · | · | · | · |
| rm_ruins00 | 684x264 | 21 | · | · | · | · | · | · | · | · | · | · | 1 | · |
| rm_ruins01 | 320x400 | 9 | · | · | · | · | · | · | · | · | · | · | 2 | · |
| rm_ruins02 | 320x480 | 41 | · | · | · | · | · | · | · | · | · | · | 2 | · |
| rm_ruins03 | 320x240 | 29 | · | · | 6 | · | · | · | · | · | · | 1 | 3 | · |
| rm_ruins04 | 740x240 | 31 | · | · | · | 2 | · | · | · | · | · | 2 | 1 | 2 |
| ruins_outro | 320x240 | 1 | · | · | · | · | · | · | · | · | · | · | · | · |

**Puntos de guardado hallados (objeto `obj_determination`, identificado por nombre; hay que confirmarlo leyendo su código):** rm_darkruins_08 (460,230); rm_darkruins_10 (500,580); rm_darkruins_12 (220,180); rm_darkruins_16 (160,120). Ninguno en `rm_ruins00`–`04`.

**Toriel:** objetos `obj_flashback_02_toriel_mini_cutscene, obj_toriel_npc, obj_toriel_intro_yellow, obj_flowey_world_toriel_fire`; instancias colocadas en estas 30 salas: 0 (si aparece, se crea por código).

## Dónde viven los sistemas (conteo por nombre en todo el juego)

| Sistema | Scripts | Objetos | Fragmentos de código |
|---|---|---|---|
| Estadísticas / LV / EXP | 46 | 60 | 229 |
| Objetos / armas / armaduras | 58 | 40 | 158 |
| GOLD / tiendas | 30 | 61 | 185 |
| Guardado / carga | 37 | 6 | 58 |
| Rutas / banderas | 159 | 180 | 680 |
| Combate | 1146 | 1276 | 4992 |

## Audio embebido en `data.win`

| Formato | Entradas | Tamaño |
|---|---|---|
| WAV | 448 | 101.5 MB |
