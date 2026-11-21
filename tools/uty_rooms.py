#!/usr/bin/env python3
"""
uty_rooms: lee las salas de Undertale Yellow desde data.win (capas, fondos, baldosas, decorados,
instancias, puertas) y puede dibujarlas a PNG para comprobar que se leen bien.

  python3 tools/uty_rooms.py data.win rm_ruins00 rm_ruins01 --png preview/
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
from gmdis import Data
from extract_assets import Win


class Rooms:
    def __init__(self, path):
        self.d = Data(path)
        self.w = Win(path)
        d = self.d
        self.room_ptr = d.plist('ROOM')
        self.room_names = [d.str_at(d.u32(p)) for p in self.room_ptr]
        self.sprite_names = d.names('SPRT')
        self.bg_names = d.names('BGND')
        self.bg_ptr = d.plist('BGND')
        self.obj_ptr = d.plist('OBJT')
        self.obj_names = [d.str_at(d.u32(p)) for p in self.obj_ptr]
        self._spr_cache = {}
        self._bg_cache = {}

    # ---------------------------------------------------------------- recursos
    def sprite(self, idx):
        """Frames RGBA (lienzo completo) de un sprite por indice."""
        if idx not in self._spr_cache:
            try:
                self._spr_cache[idx] = self.w.sprite(self.sprite_names[idx])
            except Exception:
                self._spr_cache[idx] = ([], (0, 0))
        return self._spr_cache[idx]

    def bg_image(self, idx):
        """Imagen completa de un tileset/fondo BGND."""
        if idx not in self._bg_cache:
            q = self.bg_ptr[idx]
            sx, sy, sw, sh, tx, ty, _, _, _, _, pg = self.w.tpag(self.d.u32(q + 16))
            self._bg_cache[idx] = self.w.page(pg).crop((sx, sy, sx + sw, sy + sh))
        return self._bg_cache[idx]

    def tile_source(self, idx):
        """Las baldosas legacy de GMS2 apuntan a un SPRITE (tileset); se usa su primer frame."""
        if 0 <= idx < len(self.sprite_names):
            fr, _ = self.sprite(idx)
            return fr[0] if fr else None
        return None

    def obj_info(self, oi):
        q = self.obj_ptr[oi]
        return dict(name=self.obj_names[oi], sprite=self.d.i32(q + 4), visible=self.d.u32(q + 8),
                    depth=self.d.i32(q + 20) if False else None)

    # ---------------------------------------------------------------- sala
    def load(self, name):
        d = self.d
        p = self.room_ptr[self.room_names.index(name)]
        room = dict(name=name, w=d.u32(p + 8), h=d.u32(p + 12), layers=[], instances={})
        gp = d.u32(p + 48)
        for i in range(min(d.u32(gp), 5000)):
            q = d.u32(gp + 4 + 4 * i)
            x, y, oi = struct.unpack_from('<iii', d.mm, q)
            iid, cc = d.u32(q + 12), d.i32(q + 16)
            sx, sy = struct.unpack_from('<ff', d.mm, q + 20)
            room['instances'][iid] = dict(x=x, y=y, obj=oi, name=self.obj_names[oi] if 0 <= oi < len(self.obj_names) else '?',
                                          cc=cc, sx=sx, sy=sy)
        lp = d.u32(p + 88)
        for i in range(d.u32(lp)):
            q = d.u32(lp + 4 + 4 * i)
            L = dict(name=d.str_at(d.u32(q)), id=d.u32(q + 4), type=d.u32(q + 8), depth=d.i32(q + 12),
                     xo=struct.unpack_from('<f', d.mm, q + 16)[0], yo=struct.unpack_from('<f', d.mm, q + 20)[0],
                     visible=d.u32(q + 32))
            if L['type'] == 1:                                   # fondo
                L.update(bg_visible=d.u32(q + 48), fg=d.u32(q + 52), sprite=d.i32(q + 56),
                         th=d.u32(q + 60), tv=d.u32(q + 64), color=d.u32(q + 72))
            elif L['type'] == 2:                                 # instancias
                n = d.u32(q + 48)
                L['ids'] = [d.u32(q + 52 + 4 * k) for k in range(min(n, 5000))]
            elif L['type'] == 3:                                 # decorados: baldosas legacy + sprites
                tl, sl = d.u32(q + 48), d.u32(q + 52)
                tiles = []
                for k in range(min(d.u32(tl), 20000)):
                    e = d.u32(tl + 4 + 4 * k)
                    x, y, bg, sx, sy, w, h, dep, iid = struct.unpack_from('<iiiiiIIiI', d.mm, e)
                    scx, scy = struct.unpack_from('<ff', d.mm, e + 36)
                    tiles.append(dict(x=x, y=y, bg=bg, sx=sx, sy=sy, w=w, h=h, depth=dep, scx=scx, scy=scy))
                sprites = []
                for k in range(min(d.u32(sl), 20000)):
                    e = d.u32(sl + 4 + 4 * k)
                    sprites.append(dict(name=d.str_at(d.u32(e)), spr=d.i32(e + 4), x=d.i32(e + 8), y=d.i32(e + 12),
                                        scx=struct.unpack_from('<f', d.mm, e + 16)[0], scy=struct.unpack_from('<f', d.mm, e + 20)[0]))
                L.update(tiles=tiles, sprites=sprites)
            room['layers'].append(L)
        return room

    # ---------------------------------------------------------------- dibujo
    def render(self, room, with_objects=True):
        im = Image.new('RGBA', (room['w'], room['h']), (0, 0, 0, 255))
        items = []   # (profundidad, orden, funcion de dibujo)
        n = 0
        for L in room['layers']:
            if not L['visible']:
                continue
            if L['type'] == 1 and L.get('sprite', -1) >= 0:
                items.append((L['depth'], n, ('bg', L)))
            elif L['type'] == 3:
                for t in L['tiles']:
                    items.append((t['depth'], n, ('tile', t)))
                for s in L['sprites']:
                    items.append((L['depth'], n, ('asset', s)))
            elif L['type'] == 2 and with_objects:
                for iid in L['ids']:
                    ins = room['instances'].get(iid)
                    if ins and ins['name'] not in ('obj_pl', 'obj_pl_follow'):
                        items.append((L['depth'], n, ('inst', ins)))
            n += 1
        items.sort(key=lambda t: (-t[0], t[1]))                  # mayor profundidad = mas al fondo
        for depth, _, (kind, it) in items:
            if kind == 'bg':
                frames, _ = self.sprite(it['sprite'])
                if frames:
                    fr = frames[0]
                    if it['th'] or it['tv']:
                        for yy in range(0, room['h'] if it['tv'] else 1, fr.size[1]):
                            for xx in range(0, room['w'] if it['th'] else 1, fr.size[0]):
                                im.alpha_composite(fr, (int(it['xo'] if False else xx), yy))
                    else:
                        im.alpha_composite(fr, (0, 0))
            elif kind == 'tile':
                src_img = self.tile_source(it['bg'])
                if src_img is not None:
                    src = src_img.crop((it['sx'], it['sy'], it['sx'] + it['w'], it['sy'] + it['h']))
                    if it['scx'] != 1 or it['scy'] != 1:
                        src = src.resize((max(1, int(it['w'] * it['scx'])), max(1, int(it['h'] * it['scy']))), Image.NEAREST)
                    self._paste(im, src, it['x'], it['y'])
            elif kind == 'asset':
                frames, org = self.sprite(it['spr'])
                if frames:
                    fr = frames[0]
                    if it['scx'] != 1 or it['scy'] != 1:
                        fr = fr.resize((max(1, int(fr.size[0] * abs(it['scx']))), max(1, int(fr.size[1] * abs(it['scy'])))), Image.NEAREST)
                    self._paste(im, fr, it['x'] - int(org[0] * it['scx']), it['y'] - int(org[1] * it['scy']))
            elif kind == 'inst':
                oi = it['obj']
                q = self.obj_ptr[oi]
                sp = self.d.i32(q + 4)
                if sp >= 0 and self.d.u32(q + 8):                # visible
                    frames, org = self.sprite(sp)
                    if frames:
                        fr = frames[0]
                        if it['sx'] != 1 or it['sy'] != 1:
                            fr = fr.resize((max(1, int(fr.size[0] * abs(it['sx']))), max(1, int(fr.size[1] * abs(it['sy'])))), Image.NEAREST)
                        self._paste(im, fr, it['x'] - int(org[0] * it['sx']), it['y'] - int(org[1] * it['sy']))
        return im

    @staticmethod
    def _paste(im, src, x, y):
        if src.mode != 'RGBA':
            src = src.convert('RGBA')
        im.alpha_composite(src, (max(0, x), max(0, y)), (max(0, -x), max(0, -y))) if (x < 0 or y < 0) else \
            (im.alpha_composite(src.crop((0, 0, max(0, min(src.size[0], im.size[0] - x)), max(0, min(src.size[1], im.size[1] - y)))), (x, y))
             if x < im.size[0] and y < im.size[1] else None)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    r = Rooms(sys.argv[1])
    outdir = None
    names = []
    args = sys.argv[2:]
    if '--png' in args:
        outdir = args[args.index('--png') + 1]
        args = [a for a in args if a not in ('--png', outdir)]
        os.makedirs(outdir, exist_ok=True)
    names = args
    for nm in names:
        room = r.load(nm)
        print(nm, room['w'], room['h'], 'capas', [(L['type'], L['depth']) for L in room['layers']][:8])
        if outdir:
            r.render(room).convert('RGB').save(os.path.join(outdir, nm + '.png'))


if __name__ == '__main__':
    main()
