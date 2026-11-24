#!/usr/bin/env python3
"""
lr4_compare.py — таблиця порівняння super loop (ЛР3) ↔ FreeRTOS (ЛР4) і метрики ОС.

Вхід: docs/metrics/lab3/*.log, docs/metrics/lab4/*.log (ті самі сценарії tests/sim/lr3_*.yaml),
      docs/metrics/os/*.log (сценарії ОС), docs/metrics/fw/sizes.txt (Flash/RAM/LOC, tools/fw_snapshot.sh).
Вихід (stdout): markdown — абсолютна/відносна різниця і вердикт для кожної метрики (методичка §7.3).

  python3 tools/lr4_compare.py [--json] [--root docs/metrics]
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lr3_metrics as m3  # noqa: E402

LOWER, HIGHER, INFO = -1, +1, 0        # що краще: менше / більше / лише довідково
TOL = 0.05                             # |відн. різниця| ≤ 5 % → «без змін»


def flat(run):
    """Плоский словник метрик одного прогону: ключ → (значення, напрям, одиниця)."""
    out = {}
    for b in run['blocks']:
        t = b['tag']
        j = b['jit'].get('imu')
        if j:
            out[f'{t}: джитер IMU max'] = (j['max_us'], LOWER, 'мкс')
            if int(j['p99_us']) < 4294967295:
                out[f'{t}: джитер IMU p99'] = (j['p99_us'], LOWER, 'мкс')
            out[f'{t}: пропущені періоди IMU'] = (j['missed'], LOWER, '')
        for name, s in b['lat'].items():
            out[f'{t}: LAT {name} avg'] = (s['avg_us'], LOWER, 'мкс')
            out[f'{t}: LAT {name} max'] = (s['max_us'], LOWER, 'мкс')
        for name, s in b['stat'].items():
            out[f'{t}: {name} max'] = (s['max_us'], INFO, 'мкс')
        mm = b['misc']
        if 'LOAD' in mm:
            out[f'{t}: CPU max'] = (mm['LOAD'].get('max_pct'), LOWER, '%')
        if 'RX' in mm:
            out[f'{t}: UART RX відкинуто'] = (mm['RX'].get('dropped'), LOWER, 'Б')
        if 'REC' in mm and mm['REC'].get('records'):
            out[f'{t}: журнал пропусків >25 мс'] = (mm['REC'].get('gaps'), LOWER, '')
            out[f'{t}: журнал макс. інтервал'] = (mm['REC'].get('max_int_ms'), LOWER, 'мс')
        if 'OLED' in mm and mm['OLED'].get('frames'):
            out[f'{t}: OLED кадр/с'] = (mm['OLED'].get('fps_x10', 0) / 10, HIGHER, '')
        if 'MEM' in mm:
            out[f'{t}: перший відлік IMU після скиду'] = (mm['MEM'].get('first_imu_us'), LOWER, 'мкс')
            out[f'{t}: стек MSP виміряно'] = (mm['MEM'].get('stack_used'), INFO, 'Б')
    db, ed = m3.btn_latencies(run['events'])
    if db:
        s = sorted(db)
        out['кнопка debounce→реакція median'] = (s[len(s) // 2], LOWER, 'мкс')
        out['кнопка debounce→реакція max'] = (s[-1], LOWER, 'мкс')
    for name, v in m3.event_durations(run['events']).items():
        out[f'довга операція {name} max'] = (max(v), LOWER, 'мкс')
    return out


def fmt(v, unit):
    if v is None:
        return '—'
    if unit == 'мкс':
        return m3.us(v)
    if isinstance(v, float):
        return f'{v:.1f}{(" " + unit) if unit else ""}'
    return f'{v}{(" " + unit) if unit else ""}'


def verdict(a, b, d):
    if a is None or b is None or d == INFO:
        return 'довідково'
    a, b = float(a), float(b)
    base = max(abs(a), 1e-9)
    if abs(b - a) / base <= TOL or abs(b - a) < 1e-9:
        return 'без змін'
    better = (b < a) if d == LOWER else (b > a)
    return '**краще**' if better else '**гірше**'


def compare(root):
    rows = []
    l3 = os.path.join(root, 'lab3')
    l4 = os.path.join(root, 'lab4')
    if not (os.path.isdir(l3) and os.path.isdir(l4)):
        return rows
    for f in sorted(os.listdir(l3)):
        if not f.endswith('.log') or not os.path.exists(os.path.join(l4, f)):
            continue
        a = flat(m3.parse(os.path.join(l3, f)))
        b = flat(m3.parse(os.path.join(l4, f)))
        for k in a:
            if k not in b:
                continue
            va, d, unit = a[k]
            vb = b[k][0]
            diff = None if va is None or vb is None else float(vb) - float(va)
            rel = None if diff is None or not va else diff / abs(float(va)) * 100
            rows.append({'scenario': f[:-4], 'metric': k, 'lab3': va, 'lab4': vb, 'unit': unit,
                         'diff': diff, 'rel': rel, 'verdict': verdict(va, vb, d)})
    return rows


OS_KEYS = ('TASK ', 'HEAP ', 'OSTICK', 'CTXSW', 'MSP ', 'INV ', 'QFLOOD', 'HEAPX', 'STARVE', 'OVF',
           'FATAL', 'HOOK', 'TR ', 'TRACE', 'TASKS BEGIN', '====', 'STAT name=', 'JIT name=', 'LAT name=', 'LOAD ')


def os_lines(path):
    out = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for raw in f:
            for line in m3.GLUE.split(raw):
                line = line.strip().replace('\x00', '')
                for k in OS_KEYS:
                    i = line.find(k)
                    if i >= 0:
                        out.append(line[i:])
                        break
    return out


def tasks_table(lines):
    """Останній блок TASKS із логу → markdown."""
    blk, cur = [], None
    for l in lines:
        if l.startswith('TASKS BEGIN'):
            cur = [l]
        elif cur is not None:
            cur.append(l)
            if l.startswith('MSP'):
                blk, cur = cur, None
    if not blk:
        return []
    out = ['| Задача | prio (base) | стан | CPU, % | стек, Б | мін. вільно, Б | використано, Б |',
           '|---|---|---|---:|---:|---:|---:|']
    for l in blk:
        if l.startswith('TASK '):
            d = m3.kv(l)
            st, fr = int(d['stack']), int(d['stack_free_min'])
            out.append(f"| {d['name']} | {d['prio']} ({d['base']}) | {d['state']} | {d['cpu_x10'] / 10:.1f} | "
                       f"{st} | {fr} | {st - fr} |")
    for l in blk:
        if not l.startswith('TASK ') and not l.startswith('TASKS'):
            out.append(f'\n    {l}')
    return out


def sizes(root):
    p = os.path.join(root, 'fw', 'sizes.txt')
    return open(p, encoding='utf-8').read().strip().splitlines() if os.path.exists(p) else []


def main(argv):
    root = 'docs/metrics'
    if '--root' in argv:
        root = argv[argv.index('--root') + 1]
    rows = compare(root)
    if '--json' in argv:
        json.dump(rows, sys.stdout, ensure_ascii=False, indent=1)
        return
    print('# Дані порівняння super loop ↔ FreeRTOS (згенеровано tools/lr4_compare.py)\n')
    print('> Обидві прошивки прогнано тими самими сценаріями `tests/sim/lr3_*.yaml` у Wokwi в один день'
          ' (tools/run_lab4.sh). Вердикт: зміна ≤ 5 % — «без змін»; «довідково» — метрика без «кращого» напряму.\n')
    sz = sizes(root)
    if sz:
        print('## Розмір і складність коду\n')
        print('\n'.join(sz) + '\n')
    print('## Метрики продукту\n')
    print('| Сценарій | Метрика | ЛР3 super loop | ЛР4 FreeRTOS | Δ абс. | Δ відн. | Вердикт |')
    print('|---|---|---:|---:|---:|---:|---|')
    for r in rows:
        rel = '—' if r['rel'] is None else f"{r['rel']:+.0f} %"
        dif = '—' if r['diff'] is None else fmt(int(r['diff']) if r['unit'] in ('мкс', 'Б', '', 'мс', '%') and float(r['diff']).is_integer() else r['diff'], r['unit'])
        print(f"| {r['scenario']} | {r['metric']} | {fmt(r['lab3'], r['unit'])} | {fmt(r['lab4'], r['unit'])} | "
              f"{dif} | {rel} | {r['verdict']} |")
    v = [r['verdict'] for r in rows]
    print(f"\nПідсумок: краще {v.count('**краще**')}, гірше {v.count('**гірше**')}, без змін {v.count('без змін')}, "
          f"довідково {v.count('довідково')}.\n")
    osd = os.path.join(root, 'os')
    if os.path.isdir(osd):
        print('## Метрики ОС\n')
        for f in sorted(os.listdir(osd)):
            if not f.endswith('.log'):
                continue
            lines = os_lines(os.path.join(osd, f))
            print(f'### {f}\n')
            t = tasks_table(lines)
            if t:
                print('\n'.join(t) + '\n')
            for l in lines:
                if l.startswith(('INV', 'QFLOOD', 'HEAPX', 'STARVE', 'OVF', 'FATAL', 'HOOK', 'CTXSW_COST', '====')):
                    print(f'    {l}')
            trs = [m3.kv(l) for l in lines if l.startswith('TR ')]
            if trs:
                print('\nХронологія перемикань (trace, від `trace start`):\n')
                print('| t, мкс від першого | Δ, мкс | задача |\n|---:|---:|---|')
                t0 = trs[0]['t_us']
                prev = t0
                for d in trs:
                    print(f"| {d['t_us'] - t0} | {d['t_us'] - prev} | {d['task']} |")
                    prev = d['t_us']
            print()


if __name__ == '__main__':
    main(sys.argv[1:])
