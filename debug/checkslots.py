import re
d = open('build/build/final.out', 'rb').read()
base = 0x600
sym = {}
for L in open('build/build/symbols'):
    m = re.match(r'\s*([0-9a-fA-F]{4}) (\S+)', L)
    if m:
        sym.setdefault(m.group(2).lstrip('_'), int(m.group(1), 16))
for nm in ('shm_rl', 'shm_rh', 'shm_sl1', 'shm_sh1', 'shm_sl2', 'shm_sh2'):
    a = sym.get(nm)
    if a is None:
        print(nm, 'MISSING')
        continue
    print('%-9s $%04X: ' % (nm, a), ' '.join('%02X' % b for b in d[a-base:a-base+3]))
