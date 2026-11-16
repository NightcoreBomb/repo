#!/usr/bin/env python3
"""
lr3_vcd.py — вимірювання за записом логічного аналізатора (профіль щупів lr3, ЛР3 §5.3).

Канали: BTN (PA8), P_IMU (PA9), P_PWM (PA10), P_LONG (PA15), PWM_PAN (PB4), TX, MARK (PB3), SCL.
  * затримка реакції на кнопку: перший спад BTN після ≥ 60 мс тиші → фронт MARK
    (включно з debounce 50 мс і тим, чого прошивка не бачить: брязкіт, вхід у переривання);
  * чи був у момент натискання активний P_LONG (натискання «під час довгої операції»);
  * P_IMU: ширина (час задачі), період і джитер між фронтами, «дірки» під час P_LONG;
  * P_LONG: тривалості довгих операцій.

  python3 tools/lr3_vcd.py docs/captures/lr3_btn_latency.vcd [--md]
"""
import os, statistics, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vcdtool as V   # noqa: E402


def edges(ev, ch):
    """[(t_ns, level)] без дублікатів рівня (Wokwi пише кілька змін в одну мітку — П-1)"""
    out, last = [], None
    snaps = V.snapshots(ev, [ch])
    for t, s in snaps:
        v = s[ch]
        if v is not None and v != last:
            out.append((t, v))
            last = v
    return out


def level_at(ed, t):
    lv = 0
    for te, v in ed:
        if te > t:
            break
        lv = v
    return lv


def analyze(path):
    names, ev = V.load(path)
    btn, mark, pimu, plong = (edges(ev, c) for c in ('BTN', 'MARK', 'P_IMU', 'P_LONG'))
    res = {'file': os.path.basename(path), 'channels': names}

    presses, last, prev = [], -10**18, None
    for t, v in btn:                      # початковий 0 у #0 (до підтяжки) — не натискання
        if v == 0 and prev == 1 and t - last > 60_000_000:
            presses.append(t)
        last, prev = t, v
    rises = [t for t, v in mark if v == 1]
    lat = []
    for t in presses:
        r = next((m for m in rises if m >= t), None)
        if r is not None:
            lat.append({'t_ms': t / 1e6, 'lat_ms': (r - t) / 1e6, 'during_long': bool(level_at(plong, t))})
    res['btn'] = lat

    imu_r = [t for t, v in pimu if v == 1]
    widths = V.pulses(ev, 'P_IMU')
    per = [(b - a) / 1000 for a, b in zip(imu_r, imu_r[1:])]
    res['imu'] = {
        'n': len(imu_r),
        'width_us': [w / 1000 for _, w in widths],
        'period_us': per,
    }
    res['long_ms'] = [w / 1e6 for _, w in V.pulses(ev, 'P_LONG')]
    return res


def fmt(vals, unit):
    if not vals:
        return '—'
    s = sorted(vals)
    return (f'n={len(s)} min {s[0]:.3f} {unit}, median {statistics.median(s):.3f} {unit}, '
            f'max {s[-1]:.3f} {unit}')


def report(r):
    out = [f"### Запис `{r['file']}` (канали: {', '.join(r['channels'])})", '',
           '| # | момент, мс | фронт BTN → фронт MARK, мс | під час довгої операції |', '|---:|---:|---:|---|']
    for i, b in enumerate(r['btn'], 1):
        out.append(f"| {i} | {b['t_ms']:.1f} | {b['lat_ms']:.3f} | {'так' if b['during_long'] else 'ні'} |")
    idle = [b['lat_ms'] for b in r['btn'] if not b['during_long']]
    busy = [b['lat_ms'] for b in r['btn'] if b['during_long']]
    out += ['', f'Затримка у спокої: {fmt(idle, "мс")}', '', f'Затримка під час довгої операції: {fmt(busy, "мс")}', '',
            f"P_IMU — ширина (час imu_task): {fmt(r['imu']['width_us'], 'мкс')}", '',
            f"P_IMU — інтервал між запусками: {fmt(r['imu']['period_us'], 'мкс')}", '',
            f"P_LONG — довгі операції: {fmt(r['long_ms'], 'мс')}", '']
    return '\n'.join(out)


if __name__ == '__main__':
    for p in [a for a in sys.argv[1:] if not a.startswith('--')]:
        print(report(analyze(p)))
