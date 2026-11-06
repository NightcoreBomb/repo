#!/usr/bin/env python3
"""
vcd2vectors.py — тестові вектори для драйвера MPU-6050 з РЕАЛЬНИХ записів
логічного аналізатора (методичка, п. 4.6: «байти, побачені на шині, мають після
обробки драйвером давати ті самі значення, що й прошивка в UART-лозі»).

З кожного VCD беруться:
  • I²C: запис вказівника 0x75 → читання 1 байта  (WHO_AM_I)
         запис вказівника 0x3B → читання 14 байтів (burst accel/temp/gyro)
  • UART (той самий запис, канал TX): що прошивка НАДРУКУВАЛА про ці байти
         «WHO_AM_I = 0xNN»  (ЛР1) / «who=0xNN» (ЛР2, cap i2c)
         «CAP i2c conv ax=.. ay=.. az=.. t_cC=.. gx=.. gy=.. gz=..» (ЛР2)
і записуються в tests/vectors/capture_vectors.h.

  python3 tools/vcd2vectors.py docs/captures/*.vcd > tests/vectors/capture_vectors.h
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import vcdtool as V

def pairs(ops, reg, n):
    """(t, bytes) для write [reg] до 0x68, за яким іде read n байтів від 0x68."""
    out = []
    for i in range(len(ops) - 1):
        w, r = ops[i], ops[i + 1]
        if (w['addr'] == 0x68 and w['rw'] == 'W' and w['ack'] and w['data'][:1] == [reg]
                and r['addr'] == 0x68 and r['rw'] == 'R' and r['ack'] and len(r['data']) == n):
            out.append((r['t'], r['data']))
    return out

def main(paths):
    who, burst, notes = [], [], []
    for p in paths:
        names, ev = V.load(p)
        if 'SCL' not in names or 'SDA' not in names:
            continue
        ops = V.i2c_ops(V.i2c(ev))
        lines = V.uart_lines(V.uart(ev)) if 'TX' in names else []
        base = os.path.basename(p)
        uart_who = [int(m.group(1), 16) for _, l in lines
                    for m in [re.search(r'(?:WHO_AM_I = |who=)0x([0-9A-Fa-f]{2})', l)] if m]
        uart_conv = [dict(re.findall(r'(\w+)=(-?\d+)', l)) for _, l in lines if l.startswith('CAP i2c conv')]
        for k, (t, d) in enumerate(pairs(ops, 0x75, 1)):
            exp = uart_who[k] if k < len(uart_who) else None
            who.append((base, t, d[0], exp))
        for k, (t, d) in enumerate(pairs(ops, 0x3B, 14)):
            conv = uart_conv[k] if k < len(uart_conv) else None
            burst.append((base, t, d, conv))
        notes.append('%s: %d WHO_AM_I, %d burst, %d UART-рядків' %
                     (base, len(pairs(ops, 0x75, 1)), len(pairs(ops, 0x3B, 14)), len(lines)))

    print('/* АВТОГЕНЕРОВАНО tools/vcd2vectors.py — не редагувати вручну.')
    for n in notes: print(' *   ' + n)
    print(' */')
    print('#ifndef CAPTURE_VECTORS_H\n#define CAPTURE_VECTORS_H\n#include <stdint.h>\n')
    print('typedef struct { const char *src; double t_ms; uint8_t bus; int uart; } cv_who_t;')
    print('static const cv_who_t cv_who[] = {')
    for s, t, b, e in who:
        print('  { "%s", %.3f, 0x%02X, %s },' % (s, t / 1e6, b, ('0x%02X' % e) if e is not None else '-1'))
    if not who: print('  { 0 }')
    print('};\n#define CV_WHO_N %d\n' % len(who))
    keys = ['ax', 'ay', 'az', 't_cC', 'gx', 'gy', 'gz']
    print('typedef struct { const char *src; double t_ms; uint8_t bus[14]; int has_uart; int32_t uart[7]; } cv_burst_t;')
    print('/* uart[] = ax ay az (mg), t_cC (0.01 °C), gx gy gz (m°/с) — як надрукувала прошивка */')
    print('static const cv_burst_t cv_burst[] = {')
    for s, t, d, c in burst:
        u = ', '.join(str(int(c[k])) for k in keys) if c else '0,0,0,0,0,0,0'
        print('  { "%s", %.3f, { %s }, %d, { %s } },' %
              (s, t / 1e6, ', '.join('0x%02X' % b for b in d), 1 if c else 0, u))
    if not burst: print('  { 0 }')
    print('};\n#define CV_BURST_N %d\n' % len(burst))
    print('#endif')

if __name__ == '__main__':
    main(sys.argv[1:])
