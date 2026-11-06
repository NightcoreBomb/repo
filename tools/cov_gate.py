#!/usr/bin/env python3
"""cov_gate.py <lcov.info> <min_line%> [<min_branch%>] — підсумок покриття по файлах і пороги."""
import sys
path, minimum = sys.argv[1], float(sys.argv[2])
bmin = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
files, cur = {}, None
for line in open(path):
    line = line.strip()
    if line.startswith('SF:'):
        cur = line[3:].split('firmware/')[-1]; files[cur] = [0, 0, 0, 0, 0, 0]
    elif line.startswith('LF:'): files[cur][0] = int(line[3:])
    elif line.startswith('LH:'): files[cur][1] = int(line[3:])
    elif line.startswith('BRF:'): files[cur][2] = int(line[4:])
    elif line.startswith('BRH:'): files[cur][3] = int(line[4:])
    elif line.startswith('FNF:'): files[cur][4] = int(line[4:])
    elif line.startswith('FNH:'): files[cur][5] = int(line[4:])
tot = [0] * 6
print('%-24s %8s %10s %8s' % ('file', 'lines', 'branches', 'funcs'))
for f in sorted(files):
    v = files[f]; tot = [a + b for a, b in zip(tot, v)]
    pct = lambda h, n: (100.0 * h / n) if n else 100.0
    print('%-24s %7.1f%% %9.1f%% %7.1f%%' % (f, pct(v[1], v[0]), pct(v[3], v[2]), pct(v[5], v[4])))
lp = 100.0 * tot[1] / tot[0]; bp = 100.0 * tot[3] / tot[2] if tot[2] else 100.0
print('%-24s %7.1f%% %9.1f%% %7.1f%%' % ('TOTAL', lp, bp, 100.0 * tot[5] / tot[4]))
if lp < minimum or bp < bmin:
    print('COVERAGE GATE FAILED: lines %.1f%% (min %.1f), branches %.1f%% (min %.1f)' % (lp, minimum, bp, bmin))
    sys.exit(1)
print('COVERAGE GATE OK: lines %.1f%% >= %.1f%%, branches %.1f%% >= %.1f%%' % (lp, minimum, bp, bmin))
