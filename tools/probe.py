#!/usr/bin/env python3
"""
probe.py — «переставити щупи» логічного аналізатора в diagram.json.

Wokwi for VS Code зберігає запис лише ОДНОГО аналізатора у wokwi.vcd
(журнал проблем, П-5), а 8 каналів не вистачає на всі інтерфейси одночасно.
Як і з реальним аналізатором, щупи переставляються між записами — тут
це робить скрипт, щоб розкладка каналів була відтворюваною і задокументованою.

  python3 tools/probe.py bus            # I²C + SPI + UART + MARK (за замовчуванням)
  python3 tools/probe.py pwm            # PWM pan/tilt + MARK(ADC) + кнопка + UART + LED + I²C
  python3 tools/probe.py bus --gate     # запис лише поки MARK=1 (triggerMode=level на D7)
  python3 tools/probe.py --show
"""
import json, os, sys

DIAGRAM = os.path.join(os.path.dirname(__file__), '..', 'diagram.json')
PROFILES = {
    #        D0            D1              D2            D3             D4            D5            D6            D7
    'bus': [('SCL', 'PB8'), ('SDA', 'PB9'), ('SCK', 'PA1'), ('MOSI', 'PA7'), ('MISO', 'PA6'), ('CS', 'PB0.2'), ('TX', 'PA2'), ('MARK', 'PB3')],
    'pwm': [('PWM_PAN', 'PB4'), ('PWM_TILT', 'PB5'), ('MARK', 'PB3'), ('BTN', 'PA8'), ('TX', 'PA2'), ('LED', 'PA5'), ('SCL', 'PB8'), ('SDA', 'PB9')],
}

def apply(profile, gate):
    d = json.load(open(DIAGRAM))
    la = next(p for p in d['parts'] if p['type'] == 'wokwi-logic-analyzer')
    lid = la['id']
    chans = PROFILES[profile]
    d['connections'] = [c for c in d['connections']
                        if not (c[0].startswith(lid + ':D') or c[1].startswith(lid + ':D'))]
    for i, (_, pin) in enumerate(chans):
        d['connections'].append([lid + ':D%d' % i, 'nucleo:' + pin, 'gray', []])
    la['attrs']['channelNames'] = ','.join(n for n, _ in chans)
    la['attrs']['bufferSize'] = '2000000'
    mark = [i for i, (n, _) in enumerate(chans) if n == 'MARK'][0]
    if gate:
        la['attrs'].update({'triggerMode': 'level', 'triggerPin': 'D%d' % mark, 'triggerLevel': 'high'})
    else:
        for k in ('triggerMode', 'triggerPin', 'triggerLevel'): la['attrs'].pop(k, None)
    with open(DIAGRAM, 'w') as f:
        json.dump(d, f, indent=2, ensure_ascii=False); f.write('\n')
    print('probe profile "%s"%s: %s' % (profile, ' (gated by MARK)' if gate else '',
          ', '.join('D%d=%s(%s)' % (i, n, p) for i, (n, p) in enumerate(chans))))

def show():
    d = json.load(open(DIAGRAM))
    la = next(p for p in d['parts'] if p['type'] == 'wokwi-logic-analyzer')
    print('channelNames:', la['attrs'].get('channelNames'), '| trigger:', la['attrs'].get('triggerMode', 'off'))

if __name__ == '__main__':
    a = sys.argv[1:]
    if not a or a[0] == '--show': show()
    elif a[0] in PROFILES: apply(a[0], '--gate' in a)
    else: print(__doc__); sys.exit(2)
