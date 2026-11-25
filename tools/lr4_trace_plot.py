#!/usr/bin/env python3
"""lr4_trace_plot.py — хронологія задач ЛР4 зі службових пінів (`trace pins`).

Пін = 1, поки виконується задача: P_IMU ← imu (5), P_PWM ← journal (4), P_LONG ← long_op (1),
MARK ← btn (6). BTN — вхід кнопки.

    python3 tools/lr4_trace_plot.py docs/captures/lr4_tasks.vcd out.png [t0_ms t1_ms]
"""
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt  # noqa: E402

ROWS = [('BTN', 'кнопка PA8 (1 — відпущена)'), ('MARK', 'btn, пріоритет 6'), ('P_IMU', 'imu, пріоритет 5'),
        ('P_PWM', 'journal, пріоритет 4'), ('P_LONG', 'long_op: читання блоку SD (dump), пр. 1')]


def load(path):
    ids, ev, t, scale = {}, {}, 0, 1.0
    for line in open(path, encoding='utf-8', errors='replace'):
        line = line.strip()
        if line.startswith('$timescale'):
            scale = {'1ns': 1e-6, '1us': 1e-3, '1ps': 1e-9}.get(line.split()[1] if len(line.split()) > 1 else '1ns', 1e-6)
        elif line.startswith('$var'):
            p = line.split()
            ids[p[3]] = p[4]
        elif line.startswith('#'):
            t = int(line[1:]) * 1e-6           # Wokwi: 1 нс → мс
        elif line[:1] in '01' and line[1:] in ids:
            ev.setdefault(ids[line[1:]], []).append((t, int(line[0])))
    return ev


def wave(ed, t0, t1):
    lv = 0
    for t, v in ed:
        if t <= t0:
            lv = v
    xs, ys = [t0], [lv]
    for t, v in ed:
        if t0 < t < t1:
            xs += [t, t]
            ys += [ys[-1], v]
    xs.append(t1)
    ys.append(ys[-1])
    return xs, ys


def main(a):
    ev = load(a[0])
    t0, t1 = (float(a[2]), float(a[3])) if len(a) > 3 else (3975.0, 4010.0)
    fig, ax = plt.subplots(figsize=(11, 3.6))
    for i, (ch, label) in enumerate(ROWS):
        y0 = (len(ROWS) - 1 - i) * 1.5
        xs, ys = wave(ev.get(ch, []), t0, t1)
        ax.plot(xs, [y0 + v for v in ys], lw=1.1, color='C%d' % i, drawstyle='default')
        ax.fill_between(xs, y0, [y0 + v for v in ys], step=None, alpha=0.18, color='C%d' % i)
        ax.text(t0 - (t1 - t0) * 0.01, y0 + 0.4, label, ha='right', va='center', fontsize=9)
    ax.set_yticks([])
    ax.set_xlim(t0, t1)
    ax.set_xlabel('час симуляції, мс')
    ax.set_title('Хронологія задач: dump (long_op, пр. 1) читає SD, imu (пр. 5) витісняє його кожні 5 мс', fontsize=10)
    for s in ('left', 'right', 'top'):
        ax.spines[s].set_visible(False)
    fig.tight_layout()
    fig.subplots_adjust(left=0.2)
    fig.savefig(a[1], dpi=150)


if __name__ == '__main__':
    main(sys.argv[1:])
