#!/usr/bin/env python3
"""
gmdis: desensamblador del bytecode (WAD 17) de data.win de Undertale Yellow.

  python3 tools/gmdis.py data.win --list "doorway"        # busca fragmentos de codigo por nombre
  python3 tools/gmdis.py data.win gml_Object_obj_doorway_Create_0

Es una herramienta de LECTURA (no modifica nada). Sirve para extraer reglas reales del juego
(destinos de puertas, textos, formulas) y citarlas en las fichas tecnicas.
"""
import mmap
import re
import struct
import sys

OPS = {0x07: 'conv', 0x08: 'mul', 0x09: 'div', 0x0A: 'rem', 0x0B: 'mod', 0x0C: 'add', 0x0D: 'sub', 0x0E: 'and',
       0x0F: 'or', 0x10: 'xor', 0x11: 'neg', 0x12: 'not', 0x13: 'shl', 0x14: 'shr', 0x15: 'cmp', 0x45: 'pop',
       0x84: 'pushi', 0x86: 'dup', 0x99: 'callv', 0x9C: 'ret', 0x9D: 'exit', 0x9E: 'popz', 0xB6: 'b', 0xB7: 'bt',
       0xB8: 'bf', 0xBA: 'pushenv', 0xBB: 'popenv', 0xC0: 'push', 0xC1: 'pushloc', 0xC2: 'pushglb',
       0xC3: 'pushbltn', 0xD9: 'call', 0xFF: 'break'}
TYPES = {0: 'd', 1: 'f', 2: 'i', 3: 'l', 4: 'b', 5: 'v', 6: 's', 15: 'e'}
CMP = {1: '<', 2: '<=', 3: '==', 4: '!=', 5: '>=', 6: '>'}
INST = {-1: 'self', -2: 'other', -3: 'all', -4: 'noone', -5: 'global', -6: 'builtin', -7: 'local', -9: 'stacktop', -15: 'arg'}


class Data:
    def __init__(self, path):
        f = open(path, 'rb')
        self.mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        self.chunks = {}
        o, end = 8, 8 + self.u32(4)
        while o < end:
            self.chunks[self.mm[o:o+4].decode('latin1')] = (o + 8, self.u32(o+4))
            o += 8 + self.u32(o+4)
        self.strings = None
        self._vars = None
        self._funcs = None
        self.codes = self._codes()

    def u32(self, o): return struct.unpack_from('<I', self.mm, o)[0]
    def i32(self, o): return struct.unpack_from('<i', self.mm, o)[0]
    def str_at(self, p): return self.mm[p:p + self.u32(p-4)].decode('utf8', 'replace') if p else None
    def plist(self, ch):
        o, _ = self.chunks[ch]
        return [self.u32(o + 4 + 4*i) for i in range(self.u32(o))]

    def string(self, idx):
        if self.strings is None:
            self.strings = self.plist('STRG')
        p = self.strings[idx]
        return self.mm[p+4:p+4+self.u32(p)].decode('utf8', 'replace')

    def names(self, ch):
        return [self.str_at(self.u32(p)) for p in self.plist(ch)]

    def _codes(self):
        out = []
        for p in self.plist('CODE'):
            out.append(dict(name=self.str_at(self.u32(p)), length=self.u32(p+4), start=p + 12 + self.i32(p+12)))
        return out

    # ----- referencias: variable / funcion en cada instruccion (lista enlazada en VARI / FUNC)
    def _chain(self, first, count, as_var):
        refs = {}
        addr = first
        for _ in range(count):
            word = self.u32(addr)
            operand = self.u32(addr + 4)
            refs[addr] = None
            nxt = (operand & 0x07FFFFFF) if as_var else operand
            yield addr, nxt
            addr += nxt

    def variables(self):
        if self._vars is not None:
            return self._vars
        o, ln = self.chunks['VARI']
        vm = {}
        pos = o + 12
        while pos + 20 <= o + ln:
            name = self.str_at(self.u32(pos))
            itype, vid, occ, first = self.i32(pos+4), self.i32(pos+8), self.u32(pos+12), self.u32(pos+16)
            if occ and name is not None:
                addr = first
                for _ in range(occ):
                    vm[addr] = (name, itype)
                    nxt = self.u32(addr + 4) & 0x07FFFFFF
                    if nxt == 0:
                        break
                    addr += nxt
            pos += 20
        self._vars = vm
        return vm

    def functions(self):
        """Mapa direccion-de-instruccion -> nombre. En FUNC la direccion apunta al OPERANDO (instruccion + 4)."""
        if self._funcs is not None:
            return self._funcs
        o, ln = self.chunks['FUNC']
        n = self.u32(o)
        fm = {}
        for i in range(n):
            pos = o + 4 + 12*i
            name, occ, first = self.str_at(self.u32(pos)), self.u32(pos+4), self.u32(pos+8)
            p = first
            for _ in range(occ):
                fm[p - 4] = name
                nxt = self.u32(p) & 0x07FFFFFF
                if nxt == 0:
                    break
                p += nxt
        self._funcs = fm
        return fm

    def disasm(self, idx, with_addr=False):
        c = self.codes[idx]
        vm, fm = self.variables(), self.functions()
        pc, end = c['start'], c['start'] + c['length']
        lines = []
        while pc < end:
            w = self.u32(pc)
            op, t1, t2, val = w >> 24, (w >> 16) & 15, (w >> 20) & 15, w & 0xFFFF
            sval = val - 0x10000 if val & 0x8000 else val
            nm = OPS.get(op, 'op%02X' % op)
            size, txt = 4, nm
            if op in (0xC0, 0xC1, 0xC2, 0xC3):
                t = TYPES.get(t1, '?')
                if t1 == 15:
                    txt = '%s.e %d' % (nm, sval)
                elif t1 == 0:
                    txt = '%s.d %r' % (nm, struct.unpack_from('<d', self.mm, pc+4)[0]); size = 12
                elif t1 == 3:
                    txt = '%s.l %d' % (nm, struct.unpack_from('<q', self.mm, pc+4)[0]); size = 12
                elif t1 == 1:
                    txt = '%s.f %g' % (nm, struct.unpack_from('<f', self.mm, pc+4)[0]); size = 8
                elif t1 == 6:
                    txt = '%s.s %r' % (nm, self.string(self.u32(pc+4))); size = 8
                elif t1 == 5:
                    v = vm.get(pc)
                    txt = '%s.v %s.%s' % (nm, INST.get(sval, sval), v[0] if v else '?'); size = 8
                else:
                    txt = '%s.%s %d' % (nm, t, self.i32(pc+4)); size = 8
            elif op == 0x84:
                txt = 'pushi %d' % sval
            elif op == 0x45:
                v = vm.get(pc)
                txt = 'pop.%s.%s %s.%s' % (TYPES.get(t1, '?'), TYPES.get(t2, '?'), INST.get(sval, sval), v[0] if v else '?'); size = 8
            elif op == 0xD9:
                txt = 'call %s(argc=%d)' % (fm.get(pc, '?'), val); size = 8
            elif op in (0xB6, 0xB7, 0xB8, 0xBA, 0xBB):
                off = w & 0x00FFFFFF
                off = off - 0x1000000 if off & 0x800000 else off
                txt = '%s -> 0x%X' % (nm, (pc + off*4 - c['start']) & 0xFFFFFF)
            elif op == 0x15:
                txt = 'cmp.%s.%s %s' % (TYPES.get(t1, '?'), TYPES.get(t2, '?'), CMP.get((w >> 8) & 255, '?'))
            elif op == 0x07:
                txt = 'conv.%s.%s' % (TYPES.get(t1, '?'), TYPES.get(t2, '?'))
            elif op in (0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14):
                txt = '%s.%s.%s' % (nm, TYPES.get(t1, '?'), TYPES.get(t2, '?'))
            elif op == 0x86:
                txt = 'dup.%s %d' % (TYPES.get(t1, '?'), sval)
            elif op == 0xFF:
                txt = 'break %d' % sval
            lines.append(('%05X  ' % (pc - c['start']) if with_addr else '') + txt)
            pc += size
        return lines

    def find(self, pat):
        r = re.compile(pat, re.I)
        return [i for i, c in enumerate(self.codes) if c['name'] and r.search(c['name'])]


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    d = Data(sys.argv[1])
    if sys.argv[2] == '--list':
        for i in d.find(sys.argv[3]):
            print(i, d.codes[i]['name'], d.codes[i]['length'])
    else:
        for i in d.find('^' + re.escape(sys.argv[2]) + '$'):
            print('# ' + d.codes[i]['name'])
            print('\n'.join(d.disasm(i, with_addr=True)))


if __name__ == '__main__':
    main()
