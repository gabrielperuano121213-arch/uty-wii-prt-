# Auditoría (Etapa 1) y plan técnico — Undertale Yellow nativo para Wii

Estado: **solo análisis. No he escrito código del juego.** Espero tu aprobación del plan (sección 9) antes de implementar.
Fecha: 12-nov-2026. Todo lo marcado como verificado sale de tus archivos o de páginas que abrí; lo demás figura como PENDIENTE.

## 1. Qué pude y qué no pude revisar

| Recurso | ¿Lo pude abrir? | Resultado |
|---|---|---|
| `data.win` (160 MB), `Undertale_Yellow.exe`, `splash.png`, `options.ini` | Sí | Análisis detallado (secciones 3 y 4) |
| Archivo HTML del port de navegador | Sí | Es solo el **lanzador**: descarga `game.unx` y `runner.wasm` de un CDN. No contiene lógica, salas ni diálogos, así que **no sirve como referencia de mecánicas** |
| `game.unx`, `runner.js`, `runner.wasm`, `.ogg` del port | **No** | Están alojados fuera y este entorno no tiene internet. No puedo revisarlos |
| Repositorio de la demo | Parcial | Veo README, lista de archivos y `Makefile` (idéntico al mío). GitHub me bloquea `source/` y el workflow: si cambiaste algo, súbelo |
| Gameplays en vídeo | **No** | No puedo ver vídeos. Usaré la wiki de UTY (texto), los datos del juego y tu confirmación en las dudas |
| Música `.ogg` de tu copia del juego | **No** | No la subiste (ver sección 6) |

La referencia real de las mecánicas es el **bytecode de `data.win`** (12 192 fragmentos de código). Pienso leerlo con un desensamblador propio.

## 2. La demo anterior: qué funciona y qué se reutiliza

Confirmado por ti: compila en tu GitHub Actions y se ejecuta. Dato que me falta: si fue en Wii real o en Dolphin.

* **Se reutiliza:** `Makefile` y workflow (probados con el compilador real), `verificar.py`, el bucle a 30 ticks/s, la lectura del Wiimote, el decodificador de texturas QOI+BZip2 y las herramientas de extracción.
* **No se reutiliza (se rehace):** la lógica de `game.c` (hecha a medida para una sala), los recursos incrustados en C y el renderizador por software de 320x240 (sin escalado, alfa ni rendimiento suficiente para 30 salas).
* **Cambio de controles:** la tecla **+** que alterna a Wiimote vertical contradice "solo horizontal"; se elimina.
* **Riesgo legal a decidir:** si subiste `source/assets.c`, tu repositorio público contiene arte y fuentes de UTY. No me consta que el equipo haya autorizado redistribuirlos. Propongo hacerlo privado o quitar ese archivo (ver D2).

## 3. Las Ruinas y Dark Ruins en tus archivos (datos reales)

Tabla completa por sala: `docs/AUDITORIA_RUINAS_DATOS.md` (generada automáticamente de `data.win`).

* **30 salas:** `rm_ruins00`–`04`, `rm_darkruins_01`–`19` con variantes (`08b`, `09b`, `14b`, `16b`), `rm_darkruinsintro` y `ruins_outro`. Tamaños de 320x240 hasta 1024x840 (hay que hacer cámara con scroll).
* **Transiciones:** 52 `obj_doorway` (+ salidas especiales). Los destinos están en código de creación: pendiente de leer.
* **Puntos de guardado:** 4 `obj_determination` (identificados por nombre; confirmar leyendo su código): `rm_darkruins_08` (460,230), `rm_darkruins_10` (500,580), `rm_darkruins_12` (220,180), `rm_darkruins_16` (160,120). Ninguno en `rm_ruins00`–`04`.
* **Puzzles (por objetos):** 7 palancas de 5 tipos, 6 placas de presión (`rm_ruins03`), 21 pinchos, cruces de agua falsa, 3 cajas "corn", 4 "story pillars".
* **Enemigos:** 7 encuentros aleatorios y 5 guionizados; Decibat (4 objetos: normal, cutscene, dormido, zzz); Dalv (4 apariciones); 1 disparador de Flowey; NPCs Crispy, Rorrim, Dust, Flier, Penilla y otro; 1 puesto de limonada.
* **Toriel:** existen 4 objetos suyos pero 0 instancias colocadas en estas salas, así que aparecería por código. La wiki de UTY indica que se la encuentra en las Ruinas. **El orden exacto de su intervención está PENDIENTE de verificar** (bytecode y tu confirmación).
* Estas salas comparten sus cinco primeras con Undertale según la wiki, pero el contenido de UTY es propio: no mezclaré historias.

## 4. Mecánicas: fuentes de verdad y estado

| Sistema | Dónde vive (verificado por nombres) | Estado |
|---|---|---|
| Estadísticas, LV, EXP | 46 scripts, 229 fragmentos | Fórmulas y tablas: **PENDIENTE** |
| Objetos, armas, armaduras | 58 scripts, 158 fragmentos | PENDIENTE |
| GOLD y tiendas | 30 scripts, 185 fragmentos | PENDIENTE |
| Rutas y banderas | 159 scripts, 680 fragmentos (93 objetos "geno", 52 "pacif") | PENDIENTE; la wiki tiene una página de banderas como segunda fuente |
| Combate, alma, dash, TP | 1 146 scripts, 4 992 fragmentos | PENDIENTE |
| Guardado | 37 scripts, 58 fragmentos | PENDIENTE |

Método: (1) construir `gmdis`, un desensamblador del bytecode WAD 17 y validarlo contra código conocido; (2) escribir una **ficha técnica por sistema** (fórmulas, banderas, textos) citando el fragmento de origen; (3) implementar solo lo que esté en una ficha. No inventaré cifras.

## 5. Butterscotch en Wii: evidencia

* **A favor:** existe un port real de AM2R para Wii (RetroRGB, 4–8 nov 2026) hecho sobre Butterscotch con renderizador GX nativo: 480p/240p, partidas persistentes, primer arranque de ~4 min y luego <20 s.
* **Límites:** ese port requiere mando Classic o GameCube (el Wiimote solo no alcanza para AM2R; UTY usa 3 botones). El README de Butterscotch dice soportar WAD 8–17 y lo muestra con DELTARUNE y UNDERTALE en escritorio, pero **Undertale Yellow no figura** y no hay evidencia de UTY en Wii.
* **Señal de alerta:** Cinnamon (descartado) deriva de Butterscotch; que fallara con UTY es un aviso. Me ayudaría saber qué síntoma tuvo (pantalla negra, error, sin memoria).
* **Conclusión:** no la recomiendo como vía principal. Sirve como referencia (renderizador GX, carga perezosa, guardado en Wii). Puedo reevaluarla tras la Etapa 3.

## 6. Arquitectura propuesta (nativa C, devkitPPC/libogc/GX)

```
SD:/apps/UndertaleYellow/
  boot.dol  meta.xml  icon.png          (solo código; sin arte protegido)
  data/rooms/*.bin  gfx/*.tex  font/*.fnt  text/*.bin  sfx/*.pcm  mus/*.ogg   (pack generado desde TU copia del juego)
  save/slot0.sav  slot0.bak
```

Módulos (según tu lista): `main`, `input`, `gfx` (GX), `assets/pack` (cargador), `rooms`, `collision`, `player`, `dialogue`, `battle`, `soul`, `enemy`, `boss`, `stats`, `routes`, `inventory`, `shop`, `save`, `audio`, `game`. Añado `pack` (lectura del paquete) y `script` (tablas de eventos por sala).

Decisiones que necesito que apruebes:

* **D1 Render:** pasar de software a GX (quads texturizados, alfa y escala), manteniendo coordenadas lógicas de GameMaker.
* **D2 Recursos fuera del repositorio:** una herramienta local (`uty_pack`) convierte tu `data.win` en un paquete para la SD. El repositorio y el `.dol` solo llevan código. Resuelve la redistribución y la memoria (carga por sala).
* **D3 Lógica:** reimplementar en C guiado por el bytecode (sin intérprete), con datos de sala en el paquete.
* **D4 Audio:** efectos desde los 448 WAV de `data.win` (101,5 MB; se convierten a PCM mono); **la música no está en `data.win`**, está en `.ogg` externos. Necesito esos archivos y confirmar `libvorbisidec` en la imagen de devkitPro.
* **D5 Guardado:** libfat; formato versionado con cabecera, CRC32 y dos copias; escritura a archivo temporal y renombrado para no perder la partida anterior.
* **D6 Entrada:** solo Wiimote horizontal (2=Z, 1=X, B=C, HOME sale); sin modo vertical.
* **D7 CI:** workflow actual + verificación; el paquete de datos no pasa por CI.

Memoria: la Wii tiene 24 MB (MEM1) + 64 MB (MEM2). Las texturas completas del juego ocupan 1 091 MB en RGBA8, así que se cargará por sala con presupuesto. Pistas candidatas de las Ruinas por nombre en el port: `ruins_yellow`, `darkruins`, `menu_ruins`, `menu_darkruins`, `dalvopening_yellow`, `dalvbattle_yellow`, `dalventertainer`, `dalv_diary`, `decibat_yellow`, `floweynew_yellow`, `funsized_yellow`, `fallendownyellow`, `honeydew_ruins`, `honeydew_dalv`, `gameover_yellow`, `prebattle1_yellow` (qué usa cada sala: pendiente).

## 7. Plan por etapas (cada una funcional antes de la siguiente)

| Etapa | Entrega | Criterio de aceptación |
|---|---|---|
| 1b | `gmdis` + fichas técnicas (stats, objetos, combate, rutas, guardado) | Cada dato cita su fragmento de origen |
| 2 | Motor base: GX, input, `pack`, cargador de salas, cámara, colisiones, puertas | Recorrer `rm_ruins00`–`04` |
| 3 | Diálogos, interactuables, NPCs, puzzles (palancas, placas, pinchos, agua) | Resolver los puzzles de `rm_ruins03`/`04` |
| 4 | Combate base, enemigos aleatorios/guionizados, Decibat, dash donde corresponda | Combates esquivables y con victoria/derrota |
| 5 | Estadísticas, EXP/LV, GOLD, inventario, puesto de limonada | Valores iguales a las fichas |
| 6 | Rutas y banderas | Condiciones iguales a las fichas |
| 7 | Guardado: puntos `obj_determination` y archivos en SD | Guardar/cargar y recuperación ante corte |
| 8 | Completar Dark Ruins y Dalv hasta el final de esta etapa | Recorrido completo |
| 9 | Pruebas en Wii física y optimización | Informe de pruebas |

Pruebas: yo escribo y pruebo la lógica en PC; tú compilas con Actions y pruebas en la consola. Una etapa solo se declara "probada en Wii" con tu confirmación.

## 8. Riesgos principales

* Fidelidad de combate/jefes: 4 992 fragmentos que hay que leer antes de implementar.
* Memoria y rendimiento (salas de hasta 1024x840 con muchas instancias de agua).
* Música: dependo de que me subas los `.ogg`.
* Recursos protegidos: ver D2.

## 9. Lo que necesito de ti

1. Aprobación del plan y de D1–D7.
2. Repositorio: ¿lo haces privado o quito `assets.c`?
3. Sube los `.ogg` de las pistas de las Ruinas (o la carpeta de música de tu copia).
4. ¿Dónde termina exactamente la etapa 1 (por ejemplo, al salir de las Dark Ruins)?
5. ¿La demo la probaste en Wii real o Dolphin? Y si cambiaste algo en `source/` o el workflow, súbelo.
