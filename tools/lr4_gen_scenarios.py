#!/usr/bin/env python3
"""
lr4_gen_scenarios.py — сценарії wokwi-cli, специфічні для ЛР4 (tests/sim/lr4_*.yaml).

Сценарії базової лінії (S1–S3, 6.2.1–6.2.8, FR-02, FR-12, аналізатор) — ТІ САМІ файли
tests/sim/lr3_*.yaml (tools/lr3_gen_scenarios.py, SEED=3): обидві прошивки проганяються
ними без змін (методичка ЛР4 §7.1). Тут — лише те, чого в super loop не було:

  lr4_s3_os      S3-подібне навантаження + run-time stats за задачами, запас стеків, heap,
                 вартість тіку, хронологія перемикань (trace) під час dump   — збірка build
  lr4_vcd_trace  службові піни задач на аналізаторі (`trace pins`)            — збірка build
  lr4_os         сценарії §7.2.1–7.2.5 + вартість перемикання контексту       — збірка build-ostest
  lr4_nocheck    переповнення стека з вимкненим контролем (7.2.2)             — build-nocheck
  lr4_heapsmall  heap ОС замалий для задач продукту (7.2.5)                   — build-heapsmall
  (7.2.6: lr3_s3_worst.yaml на збірці build-tick100)

Запуск — tools/run_lab4.sh; розбір — tools/lr4_compare.py.
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lr3_gen_scenarios as g3  # noqa: E402

SEED = 4


class Sc(g3.Sc):
    def write(self):
        self.delay(300)
        with open(os.path.join(g3.OUT, f'lr4_{self.name}.yaml'), 'w', encoding='utf-8') as f:
            f.write(f'# {self.title}\n# Згенеровано tools/lr4_gen_scenarios.py (SEED={SEED}) — не редагувати вручну.\n')
            f.write(f"name: 'LR4 {self.name}'\nversion: 1\nauthor: '{g3.AUTHOR}'\n\nsteps:\n")
            f.write('\n'.join(self.steps) + '\n')
        return self

    def tasks(self):
        self.cmd('tasks')
        self.wait('TASKS END')


def build():
    rnd = random.Random(SEED)
    out = []

    s = Sc('s3_os', 'S3 + метрики ОС: run-time stats, high water mark стеків, heap, тік, хронологія під час dump')
    s.boot(); s.cmd('telem 20'); s.record(8)
    s.cmd('stats reset')
    s.cmd('trace start'); s.wait('OK trace')                 # 48 перемикань від початку dump
    s.cmd('dump'); s.wait('DUMP end')
    s.cmd('trace dump'); s.wait('TRACE END')
    for i in range(12):                                     # як S3: dump, кнопка, команди, рух
        s.tilt(25 * math.sin(i / 2))
        if i % 3 == 0:
            s.cmd('dump')
        s.delay(rnd.randint(40, 400))
        s.press(80)
        s.delay(rnd.randint(200, 600))
        s.cmd('status')
    s.delay(1500)
    s.stats('S3OS')
    s.tasks()
    out.append(s.write())

    s = Sc('vcd_trace', 'Хронологія задач на аналізаторі: пін = задача виконується (`trace pins`)')
    s.boot(); s.cmd('telem 0'); s.record(3)
    s.cmd('trace pins'); s.wait('OK trace')
    s.cmd('mode rec'); s.wait('MODE REC')                   # journal + imu + oled
    s.delay(500)
    s.cmd('dump'); s.delay(rnd.randint(100, 300)); s.press(80); s.delay(1500)
    s.cmd('calib'); s.delay(300); s.press(80); s.delay(1500)
    s.press(80); s.delay(500)                               # PLAY → LIVE (кратно 3)
    s.cmd('trace stop'); s.wait('OK trace')
    out.append(s.write())

    s = Sc('os', 'Сценарії ОС §7.2.1–7.2.5 і вартість перемикання контексту (збірка OSTEST=1)')
    s.boot(); s.cmd('telem 0'); s.delay(500)
    s.cmd('os ctxsw'); s.wait('CTXSW_COST')
    s.cmd('os inv mutex'); s.wait('INV kind=mutex')
    s.cmd('os inv sem'); s.wait('INV kind=binsem')
    s.cmd('stats reset'); s.delay(1000)
    s.cmd('os starve 300'); s.wait('STARVE end')
    s.stats('STARVE'); s.tasks()
    s.cmd('os qflood 50'); s.wait('QFLOOD')
    s.cmd('os heap'); s.wait('HEAPX')
    s.tasks()
    s.cmd('os overflow'); s.wait('FATAL stack overflow')    # хук зупиняє систему — останній крок
    out.append(s.write())

    s = Sc('nocheck', 'Переповнення стека з configCHECK_FOR_STACK_OVERFLOW = 0 (збірка STACK_CHECK=0)')
    s.boot(); s.cmd('telem 0'); s.delay(300)
    s.cmd('os overflow'); s.wait('OVF start'); s.delay(1500)
    s.cmd('status'); s.delay(500)                           # чи живий продукт після переповнення?
    out.append(s.write())

    s = Sc('heapsmall', 'configTOTAL_HEAP_SIZE замалий для задач продукту (збірка HEAP_BYTES=3000)')
    s.wait('HOOK malloc_failed'); s.delay(1000)
    out.append(s.write())
    return out


if __name__ == '__main__':
    for sc in build():
        print(f'tests/sim/lr4_{sc.name}.yaml  steps={len(sc.steps)}  sim≈{sc.t_ms / 1000:.1f} с')
