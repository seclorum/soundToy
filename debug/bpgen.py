#!/usr/bin/env python3
"""Generate Oricutron monitor command files (-r :file) from build symbols.

Usage:
  bpgen.py [-o bps.txt] [--watch-lo SYM --watch-len N] fn-or-addr ...

Names resolve via build/build/symbols (xa "NNNN name" lines, _name or
LtoyNNNname).  Plain $hex/$dec numbers pass through.  Emits:
  bs  $addr ct        - exec breakpoint: printf PC: cycles + regs, continue
  bsm $addr wp        - write watchpoint: printf WB..pc, continue (per byte
                        in --watch-lo .. --watch-lo+len-1)
  <addr> with --callsites: also emit bp at every JSR <name> site (needs
                        --image build/build/final.out --base $addr)
"""
import re, sys, argparse

def load_syms(path):
    addr = {}
    for L in open(path):
        m = re.match(r'\s*([0-9a-fA-F]{4}) (\S+)', L)
        if not m: continue
        v, k = int(m.group(1), 16), m.group(2)
        n = k.split('Ltoy', 1)[-1].lstrip('0123456789') if k.startswith('Ltoy') else k.lstrip('_')
        addr.setdefault(n, v)
        addr.setdefault(k.lstrip('_'), v)
    return addr

def jsr_sites(image, base, target):
    d = open(image, 'rb').read()
    lo, hi = target & 0xff, target >> 8
    return [base + i for i in range(len(d) - 2)
            if d[i] == 0x20 and d[i+1] == lo and d[i+2] == hi]

def main():
    p = argparse.ArgumentParser()
    p.add_argument('-o', default='bps.txt')
    p.add_argument('--symbols', default='build/build/symbols')
    p.add_argument('--image', default='build/build/final.out')
    p.add_argument('--base', default='$600')
    p.add_argument('--watch-lo')
    p.add_argument('--watch-len', type=int, default=16)
    p.add_argument('--arm', action='append', default=[],
                   help='bp that also arms the trace (flags cta)')
    p.add_argument('--trace', nargs=2, metavar=('LO', 'HI'),
                   help='trace range (armed off until an --arm bp hits)')
    p.add_argument('--callsites', action='store_true',
                   help='also place bps at every JSR <name>')
    p.add_argument('names', nargs='+')
    a = p.parse_args()
    base = int(a.base.replace('$', ''), 16)
    addr = load_syms(a.symbols)
    def resolve(n):
        n0 = n.replace('$', '')
        v = addr.get(n0) or (int(n0, 16) if re.fullmatch(r'[0-9a-fA-F]+', n0) else None)
        if v is None: print('UNRESOLVED', n, file=sys.stderr)
        return v
    lines = []
    def emit(n, flags='ct'):
        v = resolve(n)
        if v is None: return
        lines.append('bs $%04x %s' % (v, flags))
        if a.callsites:
            for s in jsr_sites(a.image, base, v):
                lines.append('bs $%04x ct' % s)
    armed = set()
    for n in a.arm:
        v = resolve(n)
        if v is not None:
            lines.append('bs $%04x cta' % v)
            armed.add(n)
    for n in a.names:
        if n not in armed: emit(n)
    if a.trace:
        lo, hi = resolve(a.trace[0]), resolve(a.trace[1])
        lines.append('t $%04x $%04x a' % (lo, hi))
    if a.watch_lo:
        w = int(a.watch_lo.replace('$', ''), 16)
        for i in range(a.watch_len):
            lines.append('bsm $%04x wp' % (w + i))
    open(a.o, 'w').write('\n'.join(lines) + '\n')
    print(len(lines), 'bps ->', a.o)

main()
