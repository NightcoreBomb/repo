#!/usr/bin/env python3
"""
lr3_metrics.py — розбір логів прогонів ЛР3 (docs/metrics/*.log) у таблиці.

Лог — вивід UART прошивки (wokwi-cli --serial-log-file). Блок вимірювань:
  NOTE <tag>
  STAT BEGIN fw=… t_ms=… mode=… oled_mode=… telem_ms=… imu_hz=… load_us=…
  STAT|LAT name=<n> n=.. min_us=.. avg_us=.. max_us=..
  JIT name=imu period_us=.. n=.. min_us=.. max_us=.. p99_us=.. missed=..
  LOAD … / RX … / I2C … / REC … / OLED … / MEM …
  STAT END
Плюс події: EVT t_us=… name=…, PLAY done …, REC stop …, CHK …, STATUS …, DIR …

  python3 tools/lr3_metrics.py docs/metrics/*.log            → markdown у stdout
  python3 tools/lr3_metrics.py --json docs/metrics/*.log     → JSON
Час — з МК (мітки t_us / лічильник тактів), а не час прибуття рядка на хості (§5.2).
"""
import json, os, re, statistics, sys

KV = re.compile(r'(\w+)=(\S+)')
# wokwi-cli (0.29) у --serial-log-file «з'їдає» кінець рядка, на якому спрацював wait-serial:
# «NOTE S1STAT BEGIN …», «MODE RECOK mode». Розрізаємо такі склейки перед розбором.
CSV_TAIL = re.compile(r'\d+,-?\d+,-?\d+,\d+,\d+,[A-Z]+$')   # рядок телеметрії, приклеєний до тегу
GLUE = re.compile(r'(?=STAT BEGIN|EVT t_us=|STATUS mode=|HIST name|PLAY done|REC stop|CALIB n=|DUMP end|OK [a-z])')


def kv(line):
    out = {}
    for k, v in KV.findall(line):
        try:
            out[k] = int(v)
        except ValueError:
            out[k] = v
    return out


def parse(path):
    run = {'file': os.path.basename(path), 'blocks': [], 'events': [], 'lines': {}}
    tag, cur = None, None
    with open(path, encoding='utf-8', errors='replace') as f:
        for raw0 in f:
          for raw in GLUE.split(raw0):
              line = raw.strip().replace('\x00', '')
              if not line:
                  continue
              if line.startswith('NOTE '):
                  t = line[5:].strip()
                  tag = t.split(';')[0] if ';' in t else CSV_TAIL.sub('', t)
              elif line.startswith('STAT BEGIN'):
                  cur = {'tag': tag or '?', 'head': kv(line), 'stat': {}, 'lat': {}, 'jit': {}, 'misc': {}}
              elif cur is not None and line.startswith('STAT END'):
                  run['blocks'].append(cur)
                  cur, tag = None, None
              elif cur is not None and line.startswith('STAT name='):
                  d = kv(line); cur['stat'][d['name']] = d
              elif cur is not None and line.startswith('LAT name='):
                  d = kv(line); cur['lat'][d['name']] = d
              elif cur is not None and line.startswith('JIT name='):
                  d = kv(line); cur['jit'][d['name']] = d
              elif cur is not None and line.split(' ')[0] in ('LOAD', 'RX', 'I2C', 'REC', 'OLED', 'MEM'):
                  cur['misc'][line.split(' ')[0]] = kv(line)
              elif line.startswith('EVT '):
                  run['events'].append(kv(line))
              else:
                  for key in ('PLAY done', 'REC stop', 'CHK', 'DIR', 'STATUS', 'HIST', 'CALIB', 'ERR', '===='):
                      if line.startswith(key):
                          run['lines'].setdefault(key, []).append(line)
                          break
    return run


def us(v):
    """мкс → читабельно"""
    if v is None:
        return '—'
    v = int(v)
    if abs(v) >= 10000:
        return f'{v / 1000:.1f} мс'
    if abs(v) >= 1000:
        return f'{v / 1000:.2f} мс'
    return f'{v} мкс'


def block_table(b):
    out = [f"#### `{b['tag']}` — fw `{b['head'].get('fw')}`, mode {b['head'].get('mode')}, "
           f"OLED-режим {b['head'].get('oled_mode')}, телеметрія {b['head'].get('telem_ms')} мс, "
           f"IMU {b['head'].get('imu_hz')} Гц, load {b['head'].get('load_us')} мкс", '',
           '| Задача / виклик | n | min | avg | max |', '|---|---:|---:|---:|---:|']
    for name, s in b['stat'].items():
        out.append(f"| {name} | {s['n']} | {us(s['min_us'])} | {us(s['avg_us'])} | {us(s['max_us'])} |")
    for name, s in b['lat'].items():
        out.append(f"| **LAT {name}** | {s['n']} | {us(s['min_us'])} | {us(s['avg_us'])} | {us(s['max_us'])} |")
    j = b['jit'].get('imu')
    if j:
        out += ['', f"Джитер IMU (період {us(j['period_us'])}): n={j['n']}, min {us(j['min_us'])}, "
                    f"max {us(j['max_us'])}, p99 {('> 100 мс (останній кошик гістограми)' if int(j['p99_us']) >= 4294967295 else '≤ ' + us(j['p99_us']))}, пропущено (>2 періодів) {j['missed']}"]
    m = b['misc']
    if 'LOAD' in m:
        out.append(f"CPU: останнє вікно {m['LOAD'].get('last_pct')} %, мін {m['LOAD'].get('min_pct')} %, "
                   f"макс {m['LOAD'].get('max_pct')} % ({m['LOAD'].get('windows')} вікон по 1 с)")
    if 'RX' in m:
        out.append(f"UART RX: {m['RX'].get('bytes')} байтів, відкинуто {m['RX'].get('dropped')}, ORE {m['RX'].get('ore')}")
    if 'I2C' in m:
        out.append(f"I²C: скидів {m['I2C'].get('recov')}, таймаутів {m['I2C'].get('timeouts')}, "
                   f"помилок IMU {m['I2C'].get('imu_err')}, відліків {m['I2C'].get('imu_samples')}")
    if 'REC' in m and m['REC'].get('records'):
        out.append(f"Журнал: {m['REC'].get('records')} записів, {m['REC'].get('blocks')} блоків, "
                   f"пропусків > 25 мс: {m['REC'].get('gaps')}, макс. інтервал {m['REC'].get('max_int_ms')} мс")
    if 'OLED' in m:
        out.append(f"OLED: {m['OLED'].get('frames')} кадрів, {m['OLED'].get('fps_x10', 0) / 10:.1f} кадр/с")
    if 'MEM' in m:
        out.append(f"Стек: виміряно {m['MEM'].get('stack_used')} Б із {m['MEM'].get('stack_size')} Б; "
                   f"статична RAM {m['MEM'].get('ram_static')} Б; heap 0; перший відлік IMU "
                   f"через {us(m['MEM'].get('first_imu_us'))} після скиду")
    return '\n'.join(out) + '\n'


def event_durations(events):
    """тривалості довгих операцій з пар *_start / *_done (dur_us з МК)"""
    d = {}
    for e in events:
        n = str(e.get('name', ''))
        if n.endswith('_done') and 'dur_us' in e:
            d.setdefault(n[:-5], []).append(e['dur_us'])
    return d


def btn_latencies(events):
    db = [e['lat_db_us'] for e in events if e.get('name') == 'btn' and 'lat_db_us' in e]
    ed = [e['lat_edge_us'] for e in events if e.get('name') == 'btn' and 'lat_edge_us' in e]
    return db, ed


def summary(vals):
    if not vals:
        return '—'
    s = sorted(vals)
    p99 = s[min(len(s) - 1, int(round(0.99 * (len(s) - 1))))]
    return f'n={len(s)}, min {us(s[0])}, median {us(statistics.median(s))}, p99 {us(p99)}, max {us(s[-1])}'


def to_md(runs):
    out = ['# Результати прогонів ЛР3 (згенеровано tools/lr3_metrics.py)', '',
           '> Усі часові числа отримано **в симуляторі Wokwi** (Форма B) — див. lab3-report.md §4.5',
           '> щодо того, яким із них можна довіряти.', '']
    for r in runs:
        out += [f"## {r['file']}", '']
        for key in ('====', 'CHK', 'DIR', 'ERR'):
            for l in r['lines'].get(key, [])[:3]:
                out.append(f'    {l}')
        out.append('')
        for b in r['blocks']:
            out.append(block_table(b))
        db, ed = btn_latencies(r['events'])
        if db:
            out += [f'Кнопка (EVT): debounce→реакція {summary(db)}', '',
                    f'Кнопка (EVT): перший фронт→реакція {summary(ed)}', '']
        for name, v in event_durations(r['events']).items():
            out += [f'Довга операція `{name}` (EVT dur_us): {summary(v)}', '']
        for key in ('REC stop', 'PLAY done', 'CALIB', 'STATUS'):
            for l in r['lines'].get(key, [])[:8]:
                out.append(f'    {l}')
        out.append('')
    return '\n'.join(out)


def main(argv):
    as_json = '--json' in argv
    files = [a for a in argv if not a.startswith('--')]
    runs = [parse(f) for f in files]
    if as_json:
        json.dump(runs, sys.stdout, ensure_ascii=False, indent=1)
    else:
        print(to_md(runs))


if __name__ == '__main__':
    main(sys.argv[1:])
