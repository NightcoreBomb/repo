#!/usr/bin/env python3
"""
Тести інструментів (python3 -m unittest tools/test_tools.py):
  • check_deps ловить порушення шарів (без цього «зелена» перевірка нічого не варта);
  • vcdtool правильно декодує синтетичний VCD з відомими I²C/UART/SPI-транзакціями,
    включно з «нульовими» глітчами SDA, як у записах Wokwi;
  • vcd2vectors витягує burst + рядок UART;
  • parse_selftest розпізнає PASS/FAIL/відсутні пристрої.
"""
import os, shutil, subprocess, sys, tempfile, unittest
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import check_deps, vcdtool, parse_selftest

class VcdWriter:
    """Мінімальний генератор VCD у форматі wokwi-logic-analyzer (timescale 1 ns)."""
    def __init__(self, chans):
        self.chans = chans; self.sym = {c: chr(33 + i) for i, c in enumerate(chans)}
        self.t = 0; self.ev = [(0, c, 1 if c in ('SCL', 'SDA', 'TX', 'CS') else 0) for c in chans]
        self.lvl = {c: v for _, c, v in self.ev}
    def set(self, c, v, dt=0):
        self.t += dt; self.ev.append((self.t, c, v)); self.lvl[c] = v
    def text(self):
        out = ['$timescale 1ns $end', '$scope module logic $end']
        out += ['$var wire 1 %s %s $end' % (self.sym[c], c) for c in self.chans]
        out += ['$upscope $end', '$enddefinitions $end']
        for t, c, v in self.ev: out += ['#%d' % t, '%d%s' % (v, self.sym[c])]
        return '\n'.join(out) + '\n'
    # ---- I2C master, 100 кГц, з Wokwi-подібним глітчем на ACK ----
    def i2c_byte(self, b, ack=True, glitch=True):
        for k in range(7, -1, -1):                       # tHIGH = tLOW = 5 мкс
            self.set('SCL', 0, 5000); self.set('SDA', (b >> k) & 1, 1000); self.set('SCL', 1, 4000)
        self.set('SCL', 0, 5000)
        if glitch: self.set('SDA', 1, 0)                 # той самий ns — артефакт Wokwi
        self.set('SDA', 0 if ack else 1, 0)
        self.set('SCL', 1, 5000)
    def i2c_start(self):
        self.set('SDA', 0, 5000)
    def i2c_stop(self):
        self.set('SCL', 0, 5000); self.set('SDA', 0, 1000); self.set('SCL', 1, 4000); self.set('SDA', 1, 5000)
    def uart(self, s, baud=115200):
        bit = int(1e9 / baud)
        for ch in s.encode():
            self.set('TX', 0, bit)
            for k in range(8): self.set('TX', (ch >> k) & 1, bit)
            self.set('TX', 1, bit)
        self.t += bit
    def spi(self, mosi, miso):
        self.set('CS', 0, 1000)
        for a, b in zip(mosi, miso):
            for k in range(7, -1, -1):
                self.set('MOSI', (a >> k) & 1, 500); self.set('MISO', (b >> k) & 1, 0)
                self.set('SCK', 1, 500); self.set('SCK', 0, 500)
        self.set('CS', 1, 1000)

class TestCheckDeps(unittest.TestCase):
    def setUp(self):
        self.d = tempfile.mkdtemp()
        for l in ('Interfaces', 'Devices', 'App', 'Platform', 'Core/Inc', 'Drivers/X/Inc'):
            os.makedirs(os.path.join(self.d, l))
        w = lambda p, s: open(os.path.join(self.d, p), 'w').write(s)
        w('Interfaces/bus.h', '#include <stdint.h>\n')
        w('Drivers/X/Inc/stm32c0xx_hal.h', '')
        w('Platform/board.h', '#include "stm32c0xx_hal.h"\n#include "bus.h"\n')
        w('Devices/dev.c', '#include "bus.h"\n/* HAL_I2C_Mem_Read у коментарі — дозволено */\n')
        w('App/app.c', '#include "dev.h"\n')
        w('Devices/dev.h', '#include "bus.h"\n')
        self.w = w
    def tearDown(self): shutil.rmtree(self.d)
    def test_clean_tree_passes(self):
        errs, _, n = check_deps.check(self.d)
        self.assertEqual(errs, []); self.assertGreaterEqual(n, 5)
    def test_app_including_board_h_is_caught(self):
        self.w('App/bad.c', '#include "board.h"\n')
        errs, _, _ = check_deps.check(self.d)
        self.assertTrue(any('App/bad.c' in e and 'Platform' in e for e in errs), errs)
    def test_device_including_hal_is_caught(self):
        self.w('Devices/bad.c', '#include "stm32c0xx_hal.h"\n')
        errs, _, _ = check_deps.check(self.d)
        self.assertTrue(any('Vendor' in e for e in errs), errs)
    def test_device_including_app_is_caught(self):
        self.w('App/app.h', ''); self.w('Devices/up.c', '#include "app.h"\n')
        errs, _, _ = check_deps.check(self.d)
        self.assertTrue(any('Devices → App' in e for e in errs), errs)
    def test_hal_identifier_without_include_is_caught(self):
        self.w('App/sneaky.c', 'extern int HAL_GPIO_WritePin(void);\n')
        errs, _, _ = check_deps.check(self.d)
        self.assertTrue(any('HAL_GPIO_WritePin' in e for e in errs), errs)
    def test_identifier_in_string_is_ignored(self):
        self.w('App/log.c', 'const char *s = "HAL_OK";\n')
        errs, _, _ = check_deps.check(self.d)
        self.assertEqual(errs, [])

class TestVcd(unittest.TestCase):
    def make(self):
        v = VcdWriter(['SCL', 'SDA', 'SCK', 'MOSI', 'MISO', 'CS', 'TX', 'MARK'])
        v.t = 10000
        # WHO_AM_I: S 68W 75 P S 68R [68] NACK P   — як у ЛР1
        v.i2c_start(); v.i2c_byte(0xD0); v.i2c_byte(0x75); v.i2c_stop(); v.t += 20000
        v.i2c_start(); v.i2c_byte(0xD1); v.i2c_byte(0x68, ack=False); v.i2c_stop(); v.t += 20000
        # burst 0x3B (14 байтів)
        data = [0, 0, 0, 0, 0x40, 0, 0xEF, 0x5C, 0, 0, 0, 0, 0, 0]
        v.i2c_start(); v.i2c_byte(0xD0); v.i2c_byte(0x3B); v.i2c_stop(); v.t += 20000
        v.i2c_start(); v.i2c_byte(0xD1)
        for i, b in enumerate(data): v.i2c_byte(b, ack=i < 13)
        v.i2c_stop(); v.t += 20000
        v.uart('CAP i2c who=0x68(OK)\r\nCAP i2c conv ax=0 ay=0 az=1000 t_cC=2400 gx=0 gy=0 gz=0\r\n')
        v.spi([0x40, 0, 0, 0, 0, 0x95, 0xFF, 0xFF], [0xFF] * 7 + [0x01])
        p = os.path.join(tempfile.mkdtemp(), 'synthetic.vcd'); open(p, 'w').write(v.text())
        return p
    def test_i2c_decode_with_glitches(self):
        _, ev = vcdtool.load(self.make())
        tr = vcdtool.i2c(ev)
        self.assertEqual(vcdtool.i2c_text(tr[0]), 'S 68(W)+ 75+ P')
        self.assertEqual(vcdtool.i2c_text(tr[1]), 'S 68(R)+ 68- P')
        self.assertEqual(len(tr), 4)
        st = vcdtool.scl_stats(ev); self.assertAlmostEqual(st['f_khz'], 100.0, delta=1)
    def test_uart_decode(self):
        _, ev = vcdtool.load(self.make())
        lines = [l for _, l in vcdtool.uart_lines(vcdtool.uart(ev))]
        self.assertEqual(lines[0], 'CAP i2c who=0x68(OK)')
    def test_spi_decode_mode0(self):
        _, ev = vcdtool.load(self.make())
        f = vcdtool.spi(ev)
        self.assertEqual(f[0][2][:6], [0x40, 0, 0, 0, 0, 0x95])
        self.assertEqual(f[0][3][-1], 0x01)
    def test_vectors_generated(self):
        p = self.make()
        out = subprocess.run([sys.executable, os.path.join(HERE, 'vcd2vectors.py'), p],
                             capture_output=True, text=True, check=True).stdout
        self.assertIn('0x68, 0x68', out)
        self.assertIn('0x40, 0x00, 0xEF, 0x5C', out)
        self.assertIn('{ 0, 0, 1000, 2400, 0, 0, 0 }', out)
        self.assertIn('#define CV_BURST_N 1', out)

class TestParseSelftest(unittest.TestCase):
    LOG = ['noise', 'ST BEGIN fw=LR2 board=NUCLEO-C031C6 sysclk=48000000',
           'ST imu PASS id=0x68 az=1000', 'ST oled PASS addr=0x3C', 'ST sd PASS type=SDHC',
           'ST servo PASS pan_us=1500', 'ST pot PASS raw=1', 'ST btn PASS level=1', 'ST END pass=6 fail=0']
    def test_all_pass(self):
        h, r, s = parse_selftest.parse(self.LOG)
        self.assertEqual(h['board'], 'NUCLEO-C031C6'); self.assertEqual(r['imu']['id'], '0x68')
        self.assertEqual(s, {'pass': 6, 'fail': 0})
    def test_fail_and_missing(self):
        log = [l.replace('ST imu PASS', 'ST imu FAIL') for l in self.LOG if 'btn' not in l]
        _, r, _ = parse_selftest.parse(log)
        self.assertEqual(r['imu']['result'], 'FAIL'); self.assertNotIn('btn', r)

if __name__ == '__main__':
    unittest.main()
