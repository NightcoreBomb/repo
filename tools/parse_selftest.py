#!/usr/bin/env python3
"""
parse_selftest.py — автоматичний розбір виводу self-test (формат: App/selftest.h).

  python3 tools/parse_selftest.py serial.log          # текстовий лог UART
  python3 tools/parse_selftest.py docs/captures/x.vcd # або VCD з каналом TX
  ... --json                                          # машинний вивід

Exit code: 0 — усі очікувані пристрої PASS; 1 — є FAIL/відсутні рядки.
"""
import json, os, re, sys

EXPECTED = ['imu', 'oled', 'sd', 'servo', 'pot', 'btn']
LINE = re.compile(r'^ST (\S+) (PASS|FAIL)\b(.*)$')

def lines_from(path):
    if path.endswith('.vcd'):
        sys.path.insert(0, os.path.dirname(__file__))
        import vcdtool as V
        _, ev = V.load(path)
        return [l for _, l in V.uart_lines(V.uart(ev))]
    with open(path, errors='replace') as f:
        return [l.rstrip('\r\n') for l in f]

def parse(lines):
    res, header, summary = {}, None, None
    for l in lines:
        l = l.strip()
        if l.startswith('ST BEGIN'):
            header = dict(re.findall(r'(\w+)=(\S+)', l)); res = {}
        elif l.startswith('ST END'):
            summary = {k: int(v) for k, v in re.findall(r'(\w+)=(\d+)', l)}
        else:
            m = LINE.match(l)
            if m: res[m.group(1)] = {'result': m.group(2), **dict(re.findall(r'(\w+)=(\S+)', m.group(3)))}
    return header, res, summary

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args: print(__doc__); return 2
    header, res, summary = parse(lines_from(args[0]))
    missing = [d for d in EXPECTED if d not in res]
    failed = [d for d, r in res.items() if r['result'] != 'PASS']
    ok = header is not None and summary is not None and not missing and not failed
    if '--json' in sys.argv:
        print(json.dumps({'header': header, 'devices': res, 'summary': summary,
                          'missing': missing, 'failed': failed, 'ok': ok}, indent=2, ensure_ascii=False))
    else:
        print('board: %s' % (header or {}).get('board', '?'))
        for d in EXPECTED:
            r = res.get(d)
            print('  %-6s %-7s %s' % (d, r['result'] if r else 'MISSING',
                  ' '.join('%s=%s' % kv for kv in r.items() if kv[0] != 'result') if r else ''))
        print('RESULT:', 'OK' if ok else 'FAIL')
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
