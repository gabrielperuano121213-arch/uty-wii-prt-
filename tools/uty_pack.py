#!/usr/bin/env python3
"""
uty_pack: genera el paquete de datos (para la tarjeta SD) a partir de TU copia de Undertale Yellow.

  python3 tools/uty_pack.py --data data.win --splash splash.png --out data

Crea:  data/core.pak            sprites de Clover, interfaz, fuentes, splash, mascara del jugador
       data/rooms/<sala>.rm      imagen RGB565 de la sala + colisiones + puertas (destino y aparicion)
       data/rooms/INDEX.txt      lista de salas incluidas

Requisitos: Python 3, Pillow y numpy (pip install pillow numpy). No modifica tus archivos del juego.
El paquete contiene arte del juego: es para uso personal y NO debe subirse a un repositorio publico.
"""
import argparse
import os
import re
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
from extract_assets import down2
from uty_rooms import Rooms

KEY = 0xF81F          # color transparente (magenta) en RGB565
ROOM_PATTERN = r'(rm_)?(ruins\d|darkruins_|dalv)'


def to565(im, alpha_key=True):
    a = np.array(im.convert('RGBA'))
    r, g, b, al = a[..., 0] >> 3, a[..., 1] >> 2, a[..., 2] >> 3, a[..., 3]
    v = ((r.astype(np.uint16) << 11) | (g.astype(np.uint16) << 5) | b.astype(np.uint16))
    v[v == KEY] = KEY ^ 1
    if alpha_key:
        v[al < 128] = KEY
    return v.astype('<u2')


class Pak:
    def __init__(self):
        self.entries = []

    def add_image(self, name, im, ox=0, oy=0):
        px = to565(im)
        blob = struct.pack('<4h', im.size[0], im.size[1], ox, oy) + px.tobytes()
        self.entries.append((name, 1, blob))

    def add_font(self, name, glyphs, lineh):
        rows, tab = [], []
        for c in range(32, 127):
            r, w, off, adv = glyphs.get(c, ([], 0, 0, glyphs[32][3]))
            tab.append(struct.pack('<BBbBH', w, len(r), off, adv, len(rows)))
            rows += r
        blob = struct.pack('<B', lineh) + b''.join(tab) + struct.pack('<I', len(rows)) + struct.pack('<%dH' % len(rows), *rows)
        self.entries.append((name, 2, blob))

    def add_meta(self, name, vals):
        self.entries.append((name, 4, struct.pack('<%dh' % len(vals), *vals)))

    def write(self, path):
        hdr = 12 + 36 * len(self.entries)
        out, table, off = bytearray(), bytearray(), hdr
        for name, kind, blob in self.entries:
            table += struct.pack('<24sIII', name.encode()[:23], kind, off, len(blob)) [:36]
            out += blob
            off += len(blob)
        with open(path, 'wb') as f:
            f.write(b'UYPK' + struct.pack('<II', 1, len(self.entries)) + bytes(table) + bytes(out))


def sprite_box(r, idx):
    d, p = r.d, r.w.sprites[idx]
    w, h = d.u32(p + 4), d.u32(p + 8)
    ml, mr, mb, mt = d.i32(p + 12), d.i32(p + 16), d.i32(p + 20), d.i32(p + 24)
    ox, oy = d.i32(p + 48), d.i32(p + 52)
    return ml, mr, mt, mb, ox, oy


def inst_rect(r, it, sprite_idx):
    ml, mr, mt, mb, ox, oy = sprite_box(r, sprite_idx)
    x0 = it['x'] + (ml - ox) * it['sx']
    y0 = it['y'] + (mt - oy) * it['sy']
    ww, hh = (mr - ml + 1) * it['sx'], (mb - mt + 1) * it['sy']
    return int(round(min(x0, x0 + ww))), int(round(min(y0, y0 + hh))), int(round(abs(ww))), int(round(abs(hh)))


def cc_vars(r, ci):
    """Evalua un codigo de creacion simple (constante -> variable)."""
    out, last = {}, None
    for line in r.d.disasm(ci):
        m = re.match(r'(?:pushi|push\.[idle]|push\.s) (.+)$', line)
        if m:
            raw = m.group(1)
            try:
                last = int(raw)
            except ValueError:
                try:
                    last = float(raw)
                except ValueError:
                    last = raw.strip("'\"")
            continue
        m = re.match(r'pop\.v\.\w \w+\.(\w+)$', line)
        if m and last is not None:
            out[m.group(1)] = last
            last = None
    return out


def bake_room(r, name, outdir):
    room = r.load(name)
    d = r.d
    solids, doors, spawn, slopes = [], [], None, 0
    for it in room['instances'].values():
        q = r.obj_ptr[it['obj']]
        sp, solid = d.i32(q + 4), d.u32(q + 16)
        if it['name'] == 'obj_pl':
            spawn = (it['x'], it['y'])
        elif it['name'] == 'obj_doorway' and it['cc'] >= 0:
            v = cc_vars(r, it['cc'])
            nr = v.get('nextroom')
            if isinstance(nr, int) and 0 <= nr < len(r.room_names):
                x, y, w, h = inst_rect(r, it, sp)
                doors.append((x, y, w, h, int(v.get('xx', 0)), int(v.get('yy', 0)), r.room_names[nr]))
        elif solid and sp >= 0:
            solids.append(inst_rect(r, it, sp))
        elif re.search(r'slope|diag', it['name'], re.I):
            slopes += 1
    img = r.render(room).convert('RGB')
    px = to565(img, alpha_key=False)
    blob = b'UYRM' + struct.pack('<7i', room['w'], room['h'], len(solids), len(doors),
                                 spawn[0] if spawn else -1, spawn[1] if spawn else -1, 0)
    for s in solids:
        blob += struct.pack('<4h', *[max(-32768, min(32767, v)) for v in s])
    for dr in doors:
        blob += struct.pack('<8h32s', dr[0], dr[1], dr[2], dr[3], dr[4], dr[5], 0, 0, dr[6].encode()[:31])
    blob += px.tobytes()
    with open(os.path.join(outdir, name + '.rm'), 'wb') as f:
        f.write(blob)
    return room['w'], room['h'], len(solids), len(doors), slopes, len(blob)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--data', required=True)
    ap.add_argument('--splash', required=True)
    ap.add_argument('--out', default='data')
    ap.add_argument('--only', default=None, help='expresion regular para empaquetar solo algunas salas')
    a = ap.parse_args()
    r = Rooms(a.data)
    os.makedirs(os.path.join(a.out, 'rooms'), exist_ok=True)

    pak = Pak()
    for d_, n_ in (('down', 4), ('up', 4), ('left', 2), ('right', 2)):
        fr, o = r.w.sprite('spr_pl_' + d_)
        for k in range(n_):
            pak.add_image('clover_%s_%d' % (d_, k), fr[k], o[0], o[1])
    for nm, sp, n_ in (('soul', 'spr_heart_yellow_up', 2), ('shot', 'spr_heart_yellow_shot', 5), ('btn_fight', 'spr_fight_yellow', 2),
                       ('btn_act', 'spr_act', 2), ('btn_item', 'spr_item', 2), ('btn_mercy', 'spr_mercy', 2), ('dummy', 'spr_dummy_training_dummy', 1)):
        fr, o = r.w.sprite(sp)
        for k in range(n_):
            pak.add_image('%s_%d' % (nm, k), down2(fr[k]), o[0] // 2, o[1] // 2)
    sp = Image.open(a.splash).convert('RGBA')
    pak.add_image('splash', sp.resize((320, 240), Image.NEAREST) if sp.size == (640, 480) else sp)
    for cname, gm in (('main', 'fnt_main'), ('stats', 'fnt_stats')):
        glyphs, lineh = r.w.font(gm)
        pak.add_font(cname, glyphs, lineh)
    ml, mr, mt, mb, ox, oy = sprite_box(r, r.sprite_names.index('spr_pl_mask'))
    pak.add_meta('pl_mask', [ml - ox, mt - oy, mr - ml + 1, mb - mt + 1])       # caja de colision relativa al origen
    pak.write(os.path.join(a.out, 'core.pak'))

    names = [n for n in r.room_names if n and re.match(ROOM_PATTERN, n, re.I)]
    if a.only:
        names = [n for n in names if re.search(a.only, n)]
    total = 0
    for nm in names:
        w, h, ns, nd, sl, size = bake_room(r, nm, os.path.join(a.out, 'rooms'))
        total += size
        print('%-22s %4dx%-4d solidos %3d puertas %2d pendiente(pendientes) %d  %7d bytes' % (nm, w, h, ns, nd, sl, size))
    open(os.path.join(a.out, 'rooms', 'INDEX.txt'), 'w').write('\n'.join(names) + '\n')
    print('salas: %d, %.1f MB' % (len(names), total / 1048576))


if __name__ == '__main__':
    main()
