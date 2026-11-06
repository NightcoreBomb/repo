#!/usr/bin/env python3
"""
vcdtool.py — розбір VCD від wokwi-logic-analyzer без зовнішніх залежностей.

Незалежний від sigrok декодер (sigrok-cli лишається основним інструментом у
звіті; цей потрібен, щоб: (1) працювати в CI/Docker без GUI, (2) генерувати
тестові вектори, (3) мати другу, перевірну реалізацію декодування).

Важливо (журнал проблем, П-1): Wokwi пише кілька змін одного сигналу з
однаковою міткою часу (напр. SDA 1→0 у ту саму наносекунду, коли падає SCL).
Наївний декодер бачить у цьому STOP+START. Тому події згортаються «по
мітці часу»: береться остаточний стан усіх ліній на кожен момент.

  vcdtool.py info  file.vcd
  vcdtool.py i2c   file.vcd [--scl SCL --sda SDA]
  vcdtool.py uart  file.vcd [--tx TX --baud 115200]
  vcdtool.py spi   file.vcd [--sck SCK --mosi MOSI --miso MISO --cs CS]
  vcdtool.py pwm   file.vcd [--ch PWM]
  vcdtool.py mark  file.vcd [--ch MARK]
"""
import argparse, re, sys
from collections import defaultdict

def load(path):
    ids, names, ev, t = {}, [], [], 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            m = re.match(r'\$var\s+wire\s+1\s+(\S+)\s+(\S+)', line)
            if m:
                ids[m.group(1)] = m.group(2); names.append(m.group(2)); continue
            if line.startswith('#'):
                t = int(line[1:]); continue
            if line and line[0] in '01xz' and line[1:] in ids:
                ev.append((t, ids[line[1:]], 1 if line[0] == '1' else 0))
    return names, ev

def snapshots(ev, chans):
    """[(t, {ch: level})] — один знімок на унікальну мітку часу."""
    st = {c: None for c in chans}
    out, cur = [], None
    for t, n, v in ev:
        if n not in st: continue
        if cur is not None and t != cur:
            out.append((cur, dict(st)))
        st[n] = v; cur = t
    if cur is not None: out.append((cur, dict(st)))
    return out

# ---------------------------------------------------------------- I2C
def i2c(ev, scl='SCL', sda='SDA'):
    """Повертає список транзакцій: dict(t0,t1,items=[(kind,val,ack,t)]).
    kind: 'S','Sr','P','A' (address byte), 'D' (data byte)."""
    snaps = snapshots(ev, [scl, sda])
    trans, cur, bits, pc, pd, first = [], None, [], 1, 1, False
    for t, s in snaps:
        c = 1 if s[scl] is None else s[scl]
        d = 1 if s[sda] is None else s[sda]
        if c == 1 and pc == 1 and d != pd:
            if d == 0:   # START / repeated START
                if cur is not None and cur['items'] and cur['items'][-1][0] != 'P':
                    cur['items'].append(('Sr', None, None, t))
                else:
                    cur = {'t0': t, 'items': [('S', None, None, t)]}
                    trans.append(cur)
                bits, first = [], True
            else:        # STOP
                if cur is not None:
                    cur['items'].append(('P', None, None, t)); cur['t1'] = t
                    cur = None
                bits = []
        elif c == 1 and pc == 0 and cur is not None:   # фронт SCL — захоплення біта
            bits.append((d, t))
            if len(bits) == 9:
                b = 0
                for bit, _ in bits[:8]: b = (b << 1) | bit
                ack = bits[8][0] == 0
                cur['items'].append(('A' if first else 'D', b, ack, bits[0][1]))
                first, bits = False, []
        pc, pd = c, d
    return trans

def i2c_text(tr):
    out = []
    for it in tr['items']:
        k, v, a, _ = it
        if k in ('S', 'Sr', 'P'): out.append(k)
        elif k == 'A': out.append('%02X%s(%s)%s' % (v >> 1, '', 'R' if v & 1 else 'W', '+' if a else '-'))
        else: out.append('%02X%s' % (v, '+' if a else '-'))
    return ' '.join(out)

def i2c_ops(trans):
    """Звести транзакції у логічні операції: (addr7, rw, [bytes], acked, t0, t1)."""
    ops = []
    for tr in trans:
        seg = None
        for k, v, a, t in tr['items']:
            if k == 'A':
                seg = {'addr': v >> 1, 'rw': 'R' if v & 1 else 'W', 'data': [], 'ack': a, 't': t}
                ops.append(seg)
            elif k == 'D' and seg is not None:
                seg['data'].append(v)
            elif k == 'P' and seg is not None:
                seg['t1'] = t
    return ops

def scl_stats(ev, scl='SCL'):
    """Період і заповнення SCL усередині пакетів (ігноруючи паузи > 50 мкс)."""
    e = [(t, v) for t, n, v in ev if n == scl]
    highs, lows, periods = [], [], []
    for i in range(1, len(e)):
        dt = e[i][0] - e[i - 1][0]
        if dt > 50000 or dt == 0: continue
        (highs if e[i - 1][1] == 1 else lows).append(dt)
    rises = [t for t, v in e if v == 1]
    for i in range(1, len(rises)):
        dt = rises[i] - rises[i - 1]
        if 0 < dt < 50000: periods.append(dt)
    def med(x): return sorted(x)[len(x) // 2] if x else None
    return {'t_high_ns': med(highs), 't_low_ns': med(lows), 'period_ns': med(periods),
            'f_khz': (1e6 / med(periods)) if periods else None}

# ---------------------------------------------------------------- UART
def uart(ev, tx='TX', baud=115200):
    """8N1, idle high. Повертає [(t_start, byte)]."""
    e = [(t, v) for t, n, v in ev if n == tx]
    if not e: return []
    bit = 1e9 / baud
    # функція рівня в момент t
    times = [t for t, _ in e]; vals = [v for _, v in e]
    import bisect
    def level(t):
        i = bisect.bisect_right(times, t) - 1
        return vals[i] if i >= 0 else 1
    out, i = [], 0
    while i < len(e):
        t, v = e[i]
        if v == 0 and (i == 0 or e[i - 1][1] == 1):
            b = 0
            for k in range(8):
                b |= level(t + bit * (1.5 + k)) << k
            stop = level(t + bit * 9.5)
            out.append((t, b if stop == 1 else None))
            tend = t + bit * 9.6
            while i < len(e) and e[i][0] < tend: i += 1
            continue
        i += 1
    return out

def uart_lines(chars):
    s, lines, t0 = '', [], None
    for t, b in chars:
        if b is None: s += '\ufffd'; continue
        if t0 is None: t0 = t
        c = chr(b)
        if c == '\n':
            lines.append((t0, s.rstrip('\r'))); s, t0 = '', None
        else: s += c
    if s: lines.append((t0, s))
    return lines

# ---------------------------------------------------------------- SPI
def spi(ev, sck='SCK', mosi='MOSI', miso='MISO', cs='CS', cpol=0, cpha=0):
    """Кадри під CS#=0: [(t_cs_low, t_cs_high, [mosi bytes], [miso bytes])]."""
    chans = [c for c in (sck, mosi, miso, cs)]
    snaps = snapshots(ev, chans)
    frames, cur, pk, bm, bs, nb = [], None, None, 0, 0, 0
    pcs = 1
    sample_rise = (cpol == 0 and cpha == 0) or (cpol == 1 and cpha == 1)
    for t, s in snaps:
        c = s[sck] if s[sck] is not None else cpol
        csv = s[cs] if s[cs] is not None else 1
        if csv == 0 and pcs == 1:
            cur = [t, None, [], []]; frames.append(cur); bm = bs = nb = 0
        if csv == 1 and pcs == 0 and cur is not None:
            cur[1] = t; cur = None
        if cur is not None and pk is not None and c != pk:
            edge_rise = c == 1
            if edge_rise == sample_rise:
                bm = (bm << 1) | (s[mosi] or 0)
                bs = (bs << 1) | (s[miso] if (miso in s and s[miso] is not None) else 1)
                nb += 1
                if nb == 8:
                    cur[2].append(bm); cur[3].append(bs); bm = bs = nb = 0
        pk, pcs = c, csv
    return frames

def sck_stats(ev, sck='SCK'):
    rises = [t for t, n, v in ev if n == sck and v == 1]
    per = [rises[i] - rises[i - 1] for i in range(1, len(rises)) if 0 < rises[i] - rises[i - 1] < 20000]
    if not per: return None
    m = sorted(per)[len(per) // 2]
    return {'period_ns': m, 'f_khz': 1e6 / m}

# ---------------------------------------------------------------- PWM / MARK
def pulses(ev, ch):
    e = [(t, v) for t, n, v in ev if n == ch]
    out = []
    for i in range(len(e) - 1):
        if e[i][1] == 1 and e[i + 1][1] == 0:
            out.append((e[i][0], e[i + 1][0] - e[i][0]))
    return out   # [(t_rise, width_ns)]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cmd'); ap.add_argument('vcd')
    ap.add_argument('--scl', default='SCL'); ap.add_argument('--sda', default='SDA')
    ap.add_argument('--tx', default='TX'); ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--sck', default='SCK'); ap.add_argument('--mosi', default='MOSI')
    ap.add_argument('--miso', default='MISO'); ap.add_argument('--cs', default='CS')
    ap.add_argument('--ch', default='PWM')
    a = ap.parse_args()
    names, ev = load(a.vcd)
    if a.cmd == 'info':
        cnt = defaultdict(int)
        for _, n, _ in ev: cnt[n] += 1
        print('channels:', ', '.join(names))
        print('duration_ms: %.3f' % (ev[-1][0] / 1e6 if ev else 0))
        for n in names: print('  %-6s edges=%d' % (n, cnt[n]))
    elif a.cmd == 'i2c':
        for tr in i2c(ev, a.scl, a.sda):
            print('%12.3f us  %s' % (tr['t0'] / 1e3, i2c_text(tr)))
        print('SCL:', scl_stats(ev, a.scl))
    elif a.cmd == 'uart':
        for t, l in uart_lines(uart(ev, a.tx, a.baud)):
            print('%12.3f ms  %s' % (t / 1e6, l))
    elif a.cmd == 'spi':
        for f in spi(ev, a.sck, a.mosi, a.miso, a.cs):
            dur = ((f[1] or f[0]) - f[0]) / 1e3
            print('%12.3f us  CS# low %8.1f us  MOSI: %s' % (f[0] / 1e3, dur, ' '.join('%02X' % b for b in f[2][:24])))
            if any(b != 0xFF for b in f[3]):
                print('%31s MISO: %s' % ('', ' '.join('%02X' % b for b in f[3][:24])))
        print('SCK:', sck_stats(ev, a.sck))
    elif a.cmd in ('pwm', 'mark'):
        p = pulses(ev, a.ch)
        for i, (t, w) in enumerate(p):
            per = (p[i + 1][0] - t) / 1e3 if i + 1 < len(p) else float('nan')
            print('%12.3f ms  width %9.3f us  period %9.3f us' % (t / 1e6, w / 1e3, per))
    else:
        ap.error('unknown cmd')

if __name__ == '__main__':
    main()
