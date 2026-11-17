#!/usr/bin/env python3
"""Analyze debug/trace.log from run_trace.sh.

Lines are Oricutron bp prints:
  exec bp:  PPPP: cycles A=aa X=xx Y=yy SP=ss
  mem bp :  WB PPPP -> $AAAA pc:$PPPP cyc:N   (or RB ... <- ...)
Collapses consecutive runs, shows hit counts and the final sequence.
"""
import sys, re
from collections import Counter

log = open(sys.argv[1]).read().splitlines()
hits, mem = [], Counter()
for l in log:
    if re.match(r'^[0-9A-Fa-f]{4}: ', l):
        hits.append(l.strip())
    elif l.startswith(('WB ', 'RB ')):
        mem[l.split()[3].split(':')[1]] += 1   # writer PC

seq, prev, cnt = [], None, 0
for h in hits:
    pc = h.split(':')[0]
    if pc == prev: cnt += 1
    else:
        if prev is not None: seq.append((prev, cnt, last))
        prev, cnt = pc, 1
    last = h
if prev is not None: seq.append((prev, cnt, last))

print('exec bp hits: %d (%d distinct PCs)' % (len(hits), len(set(h.split(':')[0] for h in hits))))
print('write-watch hits by writer PC:', dict(mem) if mem else '-')
print('\nlast 15 collapsed positions:')
for pc, c, sample in seq[-15:]:
    print('  %s x%-5d last: %s' % (pc, c, sample))
trace = [l for l in log if l.startswith('>') or l.startswith('TRACE-ARMED')]
if trace:
    print('\nlast 40 trace lines (%d total):' % len(trace))
    for t in trace[-40:]:
        print(' ', t)

c = Counter(h.split(':')[0] for h in hits)
print('\ntop PCs:', dict(c.most_common(12)))
if len(seq) >= 2:
    tail_pcs = [p for p, _, _ in seq[-6:]]
    print('\nlast-hit PCs in order:', ' '.join(tail_pcs))
