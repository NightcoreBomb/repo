#!/usr/bin/env python3
"""
decode_capture.py <file.vcd> — текстовий звіт про запис логічного аналізатора:
  • декодування sigrok-cli (основний інструмент звіту) для кожного наявного інтерфейсу;
  • незалежне декодування tools/vcdtool.py + виміряні часові параметри.
Результат придатний для вставки у docs/captures/analysis.md (make capture робить .txt поруч з .vcd).
"""
import os, shutil, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vcdtool as V

def sigrok(vcd, pd, ann):
    if not shutil.which('sigrok-cli'): return '(sigrok-cli не встановлено)'
    cmd = ['sigrok-cli', '-I', 'vcd:downsample=20', '-i', vcd, '-P', pd]
    if ann: cmd += ['-A', ann]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.stdout.strip() or r.stderr.strip()

def us(ns): return '%.3f мкс' % (ns / 1e3)

def main(vcd):
    names, ev = V.load(vcd)
    has = set(names)
    print('# %s' % os.path.basename(vcd))
    print('канали: %s; тривалість %.3f мс; частота дискретизації Wokwi = 1 ГГц (1 нс), sigrok: downsample 20 → 50 МГц'
          % (', '.join(names), ev[-1][0] / 1e6 if ev else 0))
    marks = V.pulses(ev, 'MARK') if 'MARK' in has else []
    if marks:
        print('\n## MARK (вікна сценаріїв cap ...)')
        for t, w in marks[:40]: print('  %10.3f мс  тривалість %s' % (t / 1e6, us(w)))

    if {'SCL', 'SDA'} <= has:
        print('\n## I2C — sigrok-cli (i2c)')
        print(sigrok(vcd, 'i2c:scl=SCL:sda=SDA', 'i2c=start:repeat-start:stop:ack:nack:address-read:address-write:data-read:data-write'))
        tr = V.i2c(ev)
        print('\n## I2C — vcdtool: транзакції, тривалість, пауза до наступної')
        for i, x in enumerate(tr):
            t1 = x.get('t1', x['t0']); gap = (tr[i + 1]['t0'] - t1) if i + 1 < len(tr) else None
            if i < 6 or i >= len(tr) - 30 or 'P' not in V.i2c_text(x)[-1:]:
                print('  %10.3f мс  %-60s  %9s  пауза %s' % (x['t0'] / 1e6, V.i2c_text(x)[:60], us(t1 - x['t0']),
                      us(gap) if gap is not None else '-'))
            elif i == 6: print('  ... (скан адрес пропущено)')
        st = V.scl_stats(ev)
        print('  SCL: tHIGH=%s tLOW=%s період=%s → f=%.2f кГц' % (us(st['t_high_ns'] or 0), us(st['t_low_ns'] or 0),
              us(st['period_ns'] or 0), st['f_khz'] or 0))
        naks = sum(1 for x in tr for k, v, a, _ in x['items'] if k == 'A' and not a)
        print('  транзакцій: %d, NACK на адресу: %d' % (len(tr), naks))

    if {'SCK', 'MOSI', 'CS'} <= has:
        miso = 'MISO' if 'MISO' in has else None
        print('\n## SPI — sigrok-cli (spi, mode 0)')
        pd = 'spi:clk=SCK:mosi=MOSI:cs=CS' + (':miso=MISO' if miso else '')
        out = sigrok(vcd, pd, 'spi=mosi-data' + (':miso-data' if miso else ''))
        lines = out.split('\n'); print('\n'.join(lines[:60]) + ('\n  ... (%d рядків)' % len(lines) if len(lines) > 60 else ''))
        print('\n## SPI — vcdtool: кадри під CS#')
        for f in V.spi(ev, miso=miso or 'MISO'):
            dur = ((f[1] or f[0]) - f[0])
            print('  %10.3f мс  CS# low %s  %3d байт  MOSI %s' % (f[0] / 1e6, us(dur), len(f[2]), ' '.join('%02X' % b for b in f[2][:16])))
            if any(b != 0xFF for b in f[3]): print('  %42s MISO %s' % ('', ' '.join('%02X' % b for b in f[3][:16])))
        s = V.sck_stats(ev); print('  SCK: період %s → f=%.1f кГц' % (us(s['period_ns']), s['f_khz']) if s else '  SCK: немає тактів')

    if 'TX' in has:
        print('\n## UART — vcdtool (115200 8N1)')
        for t, l in V.uart_lines(V.uart(ev)): print('  %10.3f мс  %s' % (t / 1e6, l))
        print('\n## UART — sigrok-cli (перші байти)')
        print('\n'.join(sigrok(vcd, 'uart:rx=TX:baudrate=115200', 'uart=rx-data').split('\n')[:20]))

    for ch in ('PWM', 'PWM_PAN', 'PWM_TILT'):
        if ch in has:
            p = V.pulses(ev, ch)
            print('\n## %s — ширина імпульсу / період' % ch)
            for i, (t, w) in enumerate(p[:40]):
                per = (p[i + 1][0] - t) if i + 1 < len(p) else None
                print('  %10.3f мс  ширина %s  період %s' % (t / 1e6, us(w), us(per) if per else '-'))
            print('\n  sigrok pwm: ' + ' | '.join(sigrok(vcd, 'pwm:data=%s' % ch, None).split('\n')[:6]))

if __name__ == '__main__':
    if len(sys.argv) != 2: print(__doc__); sys.exit(2)
    main(sys.argv[1])
