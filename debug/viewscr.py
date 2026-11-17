import sys
d = open('/tmp/stoy_screen.bin', 'rb').read()
print('dump bytes:', len(d))
start = int(sys.argv[1]) if len(sys.argv) > 1 else 176
rows = int(sys.argv[2]) if len(sys.argv) > 2 else 24
for r in range(start, min(start+rows, 200)):
    row = d[r*40:r*40+40]
    px = ''.join('#' if (b & 0x3F) else '.' for b in row)
    attrs = ''.join('.' if (b & 0x40) else (format(b & 0x0F, 'x')) for b in row)
    print('%3d %s  [%s]' % (r, px, attrs))
# classify bottom region $BF40+ (reserved tail)
print('--- tail $BF40+ ---')
for o in range(8000, 8192, 40):
    print(hex(0xA000+o), ' '.join('%02X' % b for b in d[o:o+40]))
