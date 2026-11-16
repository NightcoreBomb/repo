#!/usr/bin/env python3
"""lr3_vcd_plot.py — анотований малюнок запису аналізатора ЛР3 (кнопка у спокої і під час calib).

    python3 tools/lr3_vcd_plot.py docs/captures/lr3_btn_latency.vcd docs/lab_3/img/lr3_btn_latency.png
"""
import os
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lr3_vcd  # noqa: E402
import vcdtool as V  # noqa: E402

CH = ['BTN', 'MARK', 'P_LONG', 'P_IMU', 'P_PWM']


def wave(ed, t0, t1):
    xs, ys, lv = [t0], [], 0
    for t, v in ed:
        if t <= t0:
            lv = v
    ys.append(lv)
    for t, v in ed:
        if t0 < t < t1:
            xs += [t, t]
            ys += [ys[-1], v]
    xs.append(t1)
    ys.append(ys[-1])
    return xs, ys


def main(path, out):
    _, ev = V.load(path)
    eds = {c: lr3_vcd.edges(ev, c) for c in CH}
    res = lr3_vcd.analyze(path)
    idle = next(b for b in res['btn'] if not b['during_long'])
    busy = [b for b in res['btn'] if b['during_long']][-1]
    panes = [(idle, 30, 90, 'у спокої'), (busy, 300, busy['lat_ms'] + 150, 'під час calib (P_LONG = 1)')]
    fig, axes = plt.subplots(1, 2, figsize=(13, 4.6))
    for ax, (b, pre, post, title) in zip(axes, panes):
        t0 = b['t_ms'] * 1e6 - pre * 1e6
        t1 = b['t_ms'] * 1e6 + post * 1e6
        for i, c in enumerate(CH):
            xs, ys = wave(eds[c], t0, t1)
            base = (len(CH) - 1 - i) * 1.5
            ax.plot([(x - b['t_ms'] * 1e6) / 1e6 for x in xs], [base + y for y in ys], lw=1.1,
                    drawstyle='default', color='C%d' % i)
            ax.text(-pre * 1.0, base + 0.35, c, ha='right', va='center', fontsize=9)
        lat = b['lat_ms']
        y = (len(CH) - 1) * 1.5 + 1.25
        ax.annotate('', xy=(lat, y), xytext=(0, y), arrowprops=dict(arrowstyle='<->', color='k'))
        ax.text(lat / 2, y + 0.15, f'BTN↓ → MARK↑ = {lat:.3f} мс', ha='center', fontsize=10)
        ax.axvline(0, color='grey', ls=':', lw=0.8)
        ax.axvline(lat, color='grey', ls=':', lw=0.8)
        ax.set_title(f'Натискання {title}', fontsize=11)
        ax.set_xlabel('мс від першого спаду BTN')
        ax.set_yticks([])
        ax.set_xlim(-pre, post)
        ax.set_ylim(-0.3, y + 0.8)
        for s in ('top', 'right', 'left'):
            ax.spines[s].set_visible(False)
    fig.suptitle('ЛР3 · lr3_btn_latency.vcd (Wokwi, fw=126ed3cd): затримка реакції на кнопку', fontsize=12)
    fig.tight_layout()
    fig.savefig(out, dpi=130)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
