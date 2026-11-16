#!/usr/bin/env python3
"""
lr3_gen_scenarios.py — генерує сценарії wokwi-cli для ЛР3 (методичка §6) у tests/sim/lr3_*.yaml.

Сценарії генеруються скриптом, а не пишуться руками, бо в деяких потрібні десятки подій у
«випадкові» моменти (30–50 натискань під час довгої операції, §6.2.2). random.Random(SEED)
робить їх відтворюваними: у ЛР4 той самий генератор дасть ті самі послідовності (базова лінія).

Запуск усіх сценаріїв — tools/run_lab3.sh; розбір логів — tools/lr3_metrics.py.
"""
import math, os, random

SEED = 3
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
OUT = os.path.join(ROOT, 'tests', 'sim')
AUTHOR = 'Paniushkin Vladyslav'


class Sc:
    def __init__(self, name, title):
        self.name, self.title, self.steps, self.t_ms = name, title, [], 0

    def raw(self, s): self.steps.append(s)
    # Увага: wait-serial чекає НОВИЙ вивід — рядок, надрукований під час попереднього
    # delay/press, він не побачить. Тому після press() не чекаємо 'MODE …' (див. лог).
    def wait(self, text): self.raw(f"  - wait-serial: '{text}'")
    def cmd(self, c): self.raw(f'  - write-serial: "{c}\\n"')
    def send(self, s): self.raw(f'  - write-serial: "{s}"')

    def delay(self, ms):
        self.t_ms += ms
        self.raw(f'  - delay: {int(ms)}ms')

    def ctl(self, part, control, value):
        self.raw(f'  - set-control:\n      part-id: {part}\n      control: {control}\n      value: {value}')

    def press(self, hold_ms=80):
        self.ctl('btn', 'pressed', 1)
        self.delay(hold_ms)
        self.ctl('btn', 'pressed', 0)

    def tilt(self, deg):                       # нахил по pitch: ax = −sin, az = cos (у g)
        r = math.radians(deg)
        self.ctl('imu', 'accelX', round(-math.sin(r), 3))
        self.ctl('imu', 'accelZ', round(math.cos(r), 3))

    def stats(self, tag):
        self.cmd(f'note {tag};')                 # ';' відділяє тег від рядка телеметрії,
        self.wait(f'NOTE {tag};')                # який wokwi-cli може приклеїти до нього
        self.cmd('stats')
        self.wait('STAT END')

    def boot(self):
        self.wait('READY')
        self.cmd('evt on')

    def record(self, seconds):                 # підготувати журнал (для dump/play)
        self.cmd('mode rec')
        self.wait('MODE REC')
        for i in range(seconds):
            self.tilt(25 * math.sin(i / 3))
            self.delay(1000)
        self.cmd('mode live')
        self.wait('MODE LIVE')

    def write(self):
        # wokwi-cli зупиняє симуляцію одразу після останнього кроку, і хвіст рядка, на якому
        # спрацював wait-serial, у лог не потрапляє («NOTE FR02_-30STATUS» без значень).
        self.delay(300)
        with open(os.path.join(OUT, f'lr3_{self.name}.yaml'), 'w', encoding='utf-8') as f:
            f.write(f'# {self.title}\n# Згенеровано tools/lr3_gen_scenarios.py (SEED={SEED}) — не редагувати вручну.\n')
            f.write(f"name: 'LR3 {self.name}'\nversion: 1\nauthor: '{AUTHOR}'\n\nsteps:\n")
            f.write('\n'.join(self.steps) + '\n')
        return self


def build():
    rnd = random.Random(SEED)
    out = []

    # S1 — простій: лише періодичні задачі (IMU 200 Гц, pot, OLED 5 Гц, телеметрія 10 Гц)
    s = Sc('s1_idle', 'S1 — простій: подій немає, працюють лише періодичні задачі')
    s.boot(); s.delay(2000); s.cmd('stats reset'); s.delay(10000); s.stats('S1'); s.cmd('hist'); s.wait('HIST')
    out.append(s.write())

    # S2 — номінальний: запис траєкторії 20 с (рух руки), рідкісні команди, потім відтворення
    s = Sc('s2_nominal', 'S2 — номінальний режим PRD: Record 20 с з рухом, рідкісні команди, Playback')
    s.boot(); s.delay(1000); s.cmd('stats reset')
    s.press(); s.delay(200)                                   # FR-06: кнопкою (MODE REC — у лозі)
    for i in range(20):
        s.tilt(30 * math.sin(i / 2.5))
        if i % 5 == 4: s.cmd('status')
        s.delay(1000)
    s.stats('S2_rec')
    s.cmd('stats reset')
    s.press()
    s.wait('PLAY done')
    s.stats('S2_play')
    s.press(); s.delay(300)
    out.append(s.write())

    # S3 — найгірший: запис + OLED блокувально + телеметрія 50 Гц + потік команд + кнопка + dump
    s = Sc('s3_worst', 'S3 — найгірший: довгі операції + періодичні задачі + потік команд + події')
    s.boot(); s.record(15)                                    # журнал для dump
    s.cmd('oled 0'); s.cmd('telem 20'); s.cmd('stats reset')
    for i in range(30):
        s.send('status\\n' * 3)
        s.tilt(rnd.uniform(-35, 35))
        if i % 3 == 0: s.cmd('dump')                         # «передача на хост» (довга)
        s.delay(rnd.randint(40, 600))
        s.press(rnd.randint(60, 120))                        # LIVE→REC→PLAY→LIVE…
        s.delay(rnd.randint(200, 700))
    s.cmd('telem 100')
    s.stats('S3'); s.cmd('hist'); s.wait('HIST')
    out.append(s.write())

    # 6.2.1 — довга операція і періодична задача
    s = Sc('longop', '6.2.1 — довга операція (dump, calib) і періодична задача IMU')
    s.boot(); s.record(30)
    s.cmd('stats reset')
    for _ in range(3):
        s.cmd('dump'); s.wait('DUMP end'); s.delay(500)
    s.cmd('calib'); s.wait('CALIB n=')
    s.stats('LONGOP'); s.cmd('hist'); s.wait('HIST')
    out.append(s.write())

    # 6.2.2 — подія під час довгої операції: 40 натискань у випадковий момент dump
    s = Sc('event_longop', '6.2.2 — 40 натискань кнопки у випадкові моменти довгої операції (dump)')
    s.boot(); s.record(30); s.cmd('telem 0'); s.cmd('stats reset')
    for _ in range(40):
        s.cmd('dump')
        s.delay(rnd.randint(5, 900))
        s.press(80)
        s.delay(1400)
    s.stats('EVENT_LONGOP')
    out.append(s.write())

    # 6.2.3 — потік команд по UART під час довгої операції
    s = Sc('uart_flood', '6.2.3 — команди без пауз під час довгої операції: втрати байтів, час відповіді')
    s.boot(); s.record(6); s.cmd('telem 0'); s.cmd('stats reset')
    s.send('status\\n' * 20); s.delay(500)
    s.send('\\n')                                            # дописати рядок, що міг обірватись втратою
    s.stats('FLOOD_idle')
    s.cmd('stats reset')
    for _ in range(3):
        s.cmd('dump'); s.delay(20)
        s.send('status\\n' * 40)                              # 280 байтів поки триває dump
        s.wait('DUMP end'); s.delay(800); s.send('\\n')
    s.stats('FLOOD_longop')
    out.append(s.write())

    # 6.2.4 — відмова пристрою: «SDA затиснута» (емуляція в драйвері) на 2 с
    s = Sc('fault', '6.2.4 — відмова I²C під час роботи і відновлення')
    s.boot(); s.delay(1000); s.cmd('stats reset'); s.delay(2000)
    s.cmd('fault i2c 1'); s.wait('FAULT i2c=1'); s.delay(2000)
    s.cmd('fault i2c 0'); s.wait('FAULT i2c=0'); s.delay(2000)
    s.stats('FAULT')
    out.append(s.write())

    # 6.2.5 — пошук точки зламу: частота IMU, штучне навантаження, спосіб оновлення OLED
    s = Sc('breakpoint', '6.2.5 — пошук точки зламу: частота опитування, навантаження, OLED')
    s.boot(); s.delay(1000)
    for hz in (200, 300, 400, 600, 800, 1000, 1500, 2000):
        s.cmd(f'rate {hz}'); s.delay(3000); s.stats(f'RATE_{hz}')
    s.cmd('rate 200')
    for m in (0, 1, 2):
        s.cmd(f'oled {m}'); s.cmd('stats reset'); s.delay(4000); s.stats(f'OLED_{m}')
    s.cmd('oled 0')
    for us in (500, 1000, 2000, 3000, 4000):
        s.cmd(f'load {us}'); s.cmd('stats reset'); s.delay(3000); s.stats(f'LOAD_{us}')
    s.cmd('load 0')
    out.append(s.write())

    # 6.2.6 — тривала робота в S2. Безкоштовний план Wokwi CI обриває з'єднання через 5 хв
    # реального часу (≈ 120 с симуляції цієї схеми), тому 105 с: одне переповнення
    # 32-бітного лічильника тактів (89.5 с) і три контрольні точки.
    s = Sc('long_run', '6.2.6 — тривала робота в S2 (105 с симуляції; переповнення лічильника тактів на 89.5 с)')
    s.boot(); s.cmd('stats reset'); s.cmd('mode rec'); s.wait('MODE REC')
    for k in range(3):
        for i in range(35):
            s.tilt(20 * math.sin((k * 35 + i) / 4))
            s.delay(1000)
        s.stats(f'LONG_{(k + 1) * 35}s')
    s.cmd('mode live'); s.wait('REC stop')
    out.append(s.write())

    # 6.2.7 / 6.2.8 / FR-09 — старт, перезапуск, «зникнення живлення» під час запису
    s = Sc('boot_power', '6.2.7–6.2.8, FR-09 — старт, журнал після перезапуску, скид під час запису')
    s.boot(); s.stats('BOOT')
    s.record(12)                                              # 600 записів = 3 блоки
    s.cmd('reset'); s.boot()
    s.cmd('mode play'); s.wait('PLAY done'); s.cmd('mode live')
    s.cmd('mode rec'); s.wait('MODE REC'); s.delay(7000)     # блок 0 записано, блок 1 — у RAM
    s.cmd('reset'); s.boot()                                  # «вимкнули живлення» без rec_stop
    s.cmd('mode play'); s.wait('PLAY done'); s.cmd('mode live')
    out.append(s.write())

    # FR-02 — статична точність у точках плану тестування PRD
    s = Sc('fr02_static', 'FR-02 — pitch/roll у статиці: 0°, 30°, 45°, −30° (≤ 3°)')
    s.boot()
    for deg in (0, 30, 45, -30):
        s.tilt(deg); s.delay(2000)
        s.cmd(f'note FR02_{deg}'); s.wait(f'NOTE FR02_{deg}'); s.cmd('status'); s.wait('STATUS')
    out.append(s.write())

    # FR-12 — IMU відсутній (окрема схема без MPU-6050)
    s = Sc('fr12_noimu', 'FR-12 — старт без MPU-6050: повідомлення, серво в центрі, без зависання')
    s.wait('ERR IMU not found'); s.wait('READY'); s.delay(2000)
    s.cmd('status'); s.wait('STATUS'); s.stats('NOIMU')
    out.append(s.write())

    # Запис логічного аналізатора: затримка реакції на кнопку у спокої і під час dump
    s = Sc('vcd_btn', 'Запис аналізатора (профіль lr3): кнопка → MARK, IMU/PWM/LONG-пробники')
    # Кнопка перемикає LIVE→REC→PLAY→LIVE, тож натискань кратно 3 (повертаємось у LIVE).
    # Реакція (≈52 мс) настає ще під час утримання (80 мс), тож wait-serial її не побачить —
    # лише фіксовані паузи. Довга операція — calib (≈1 с, у будь-якому режимі).
    s.boot(); s.cmd('telem 0'); s.record(3)
    for _ in range(3):                                        # 3 натискання у спокої
        s.delay(rnd.randint(400, 800)); s.press(80)
    s.delay(1000)
    for _ in range(3):                                        # 3 — під час calib
        s.cmd('calib'); s.delay(rnd.randint(50, 600)); s.press(80); s.delay(1500)
    s.delay(500); s.stats('VCD')
    out.append(s.write())
    return out


if __name__ == '__main__':
    for sc in build():
        print(f'tests/sim/lr3_{sc.name}.yaml  steps={len(sc.steps):4d}  delays≈{sc.t_ms / 1000:.0f} s')
