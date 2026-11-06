#!/usr/bin/env python3
"""
check_pinout.py — три джерела правди про підключення мають збігатися:
  docs/pinout.md (таблиця між <!-- PINOUT-TABLE --> ... <!-- /PINOUT-TABLE -->)
  diagram.json   (монтажна схема Wokwi = «стенд» для Форми B)
  firmware/Platform/board.h (номери пінів, з якими працює прошивка)

Перевіряється:
  1. кожен вивід пристрою з колонки «Wokwi» з'єднаний у diagram.json саме з цим піном МК;
  2. кожне з'єднання «пристрій ↔ сигнальний пін МК» з diagram.json є в таблиці
     (щупи логічного аналізатора — не пристрої, пропускаються);
  3. множина пінів у board.h == множина пінів таблиці.
  4. на один пін МК не претендують дві різні ролі в board.h (пін-конфлікт).
"""
import json, os, re, sys
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
POWER = re.compile(r'^(GND|3V3|5V|VIN|IOREF|E5V|AVDD|VDD|NRST)')

def norm(pin): return pin.split('.')[0]

def table(md):
    s = open(md, encoding='utf-8').read()
    m = re.search(r'<!-- PINOUT-TABLE -->(.*?)<!-- /PINOUT-TABLE -->', s, re.S)
    if not m: sys.exit('pinout.md: не знайдено маркерів PINOUT-TABLE')
    rows = [r for r in m.group(1).strip().split('\n') if r.startswith('|')]
    head = [c.strip() for c in rows[0].strip('|').split('|')]
    iw = head.index('Wokwi'); out = {}
    for r in rows[2:]:
        c = [x.strip() for x in r.strip('|').split('|')]
        pin = re.sub(r'[*`]', '', c[0]).strip()
        eps = [e.strip(' `') for e in c[iw].split(',') if ':' in e]
        out[pin] = eps
    return out

def diagram(path):
    d = json.load(open(path))
    con = []
    for a, b, *_ in d['connections']:
        for x, y in ((a, b), (b, a)):
            if x.startswith('nucleo:') and not y.startswith('nucleo:'):
                con.append((y, norm(x.split(':', 1)[1])))
    return con

def board(path):
    s = open(path, encoding='utf-8').read()
    ports = dict(re.findall(r'#define\s+(BOARD_\w+?)_Port\s+GPIO([A-F])', s))
    pins = {}
    for name, num in re.findall(r'#define\s+(BOARD_\w+?)_Pin\s+GPIO_PIN_(\d+)', s):
        pfx = max((p for p in ports if name.startswith(p)), key=len, default=None)
        if pfx is None: continue
        pins.setdefault('P%s%s' % (ports[pfx], num), []).append(name)
    return pins

def main():
    errs = []
    tab = table(os.path.join(ROOT, 'docs', 'pinout.md'))
    con = diagram(os.path.join(ROOT, 'diagram.json'))
    brd = board(os.path.join(ROOT, 'firmware', 'Platform', 'board.h'))
    conset = set(con)
    for pin, eps in tab.items():
        for e in eps:
            if (e, pin) not in conset:
                errs.append('pinout.md: %s → %s, але в diagram.json такого з\'єднання немає' % (pin, e))
    for ep, pin in con:
        if POWER.match(pin) or ep.startswith('logic:'): continue
        if ep not in tab.get(pin, []):
            errs.append('diagram.json: %s ↔ %s відсутнє в таблиці pinout.md' % (ep, pin))
    if set(brd) != set(tab):
        errs.append('board.h vs pinout.md: тільки в board.h %s; тільки в таблиці %s'
                    % (sorted(set(brd) - set(tab)), sorted(set(tab) - set(brd))))
    for pin, names in brd.items():
        if len(names) > 1:
            errs.append('board.h: пін %s має кілька ролей %s' % (pin, names))
    for e in errs: print('PINOUT ERROR:', e)
    print('check_pinout: %d pins in table, %d device wires, %d pins in board.h, %d error(s)'
          % (len(tab), sum(1 for e, p in con if not POWER.match(p) and not e.startswith('logic:')),
             len(brd), len(errs)))
    return 1 if errs else 0

if __name__ == '__main__':
    sys.exit(main())
