#!/usr/bin/env python3
"""stack_tree.py — статична оцінка найгіршої глибини стека кожної задачі (ЛР4, методичка §6.3).

Читає граф викликів GCC (-fcallgraph-info=su → build-su/*.ci): вузли з розміром кадру
(«N bytes (static|dynamic)») і ребра «хто кого викликає». Для кожної точки входу задачі
шукає найглибший ланцюжок (DFS, рекурсія — помилка). Непрямі виклики (покажчики на функції
в bus_i2c_t/bus_spi_t, «__indirect_call») замінюються найдовшим ланцюжком серед
INDIRECT_TARGETS — це консервативно. До результату додається кадр перемикання контексту
ядра (16 слів = 64 Б на ARMv6-M: r4-r11 + апаратний кадр r0-r3,r12,lr,pc,xpsr).

  make -C firmware LTO= BUILD_DIR=build-su     # без LTO: .ci пишуться для кожного .c
  python3 tools/stack_tree.py firmware/build-su  [--path]

Вивід: TASK name=<..> entry=<..> static_bytes=<..> path=<f1>f2>...>
"""
import glob
import re
import sys

TASKS = {                       # задача → функція-точка входу (App/app_rtos.c)
    'btn': 'task_btn', 'imu': 'task_imu', 'journal': 'task_journal', 'cmd': 'task_cmd',
    'telem': 'task_telem', 'oled': 'task_oled', 'long_op': 'task_long',
}
# реалізації інтерфейсів шин (bus_i2c_t / bus_spi_t) — куди ведуть непрямі виклики
I2C_LOCKED = ['App/app.c:lk_write', 'App/app.c:lk_read', 'App/app.c:lk_wr', 'App/app.c:lk_probe']
I2C_RAW = ['Platform/bus_i2c_stm32.c:w', 'Platform/bus_i2c_stm32.c:r', 'Platform/bus_i2c_stm32.c:wr',
           'Platform/bus_i2c_stm32.c:probe']
SPI = ['Platform/bus_spi_stm32.c:xfer', 'Platform/bus_spi_stm32.c:sel', 'Platform/bus_spi_stm32.c:set_fast']


def indirect_targets(caller):
    """Куди веде виклик через покажчик з цієї функції (за шаром/файлом викликача)."""
    if ':lk_' in caller:
        return I2C_RAW                       # м'ютекс → справжній драйвер
    short = caller.split(':')[-1]
    if 'sdcard.c' in caller or short.startswith('sd_') or short.startswith('xchg') or short.startswith('send_cmd'):
        return SPI
    return I2C_LOCKED + SPI                  # драйвери пристроїв, self-test
CTX_FRAME = 64
# Функції ядра/бібліотеки без .ci (зібрані без графа або з libgcc): оцінка кадру, байт
EXTERNAL = {'__aeabi_uidiv': 8, '__aeabi_idiv': 8, '__aeabi_uidivmod': 8, '__aeabi_idivmod': 8,
            'memset': 8, 'memcpy': 8, 'strcmp': 8, 'strchr': 8, 'strlen': 8}

# Ребра, яких у контексті задачі не буває (обґрунтування — docs/rtos-design.md §3):
#  - xTaskIncrementTick → vApplicationTickHook: з xTaskResumeAll тік обробляється з xPendedTicks > 0,
#    а хук викликається лише при xPendedTicks == 0 (tasks.c V11.1.0) — тобто лише в ISR (MSP);
#  - os_assert_failed: фатальна зупинка, далі програма не виконується.
SKIP_EDGES = {('xTaskIncrementTick', 'vApplicationTickHook')}
TERMINAL = {'os_assert_failed'}

NODE = re.compile(r'node: \{ title: "([^"]+)" label: "([^"]*)"')
EDGE = re.compile(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"')


def load(d):
    frame, edges, alias = {}, {}, {}
    for f in glob.glob(d + '/*.ci'):
        for line in open(f, encoding='utf-8', errors='replace'):
            m = NODE.search(line)
            if m:
                title, label = m.groups()
                short = title.split(':')[-1]
                sz = re.search(r'\\n(\d+) bytes \((static|dynamic|dynamic,bounded)\)', label)
                if sz:
                    frame[title] = int(sz.group(1))
                    alias.setdefault(short, title)
                continue
            m = EDGE.search(line)
            if m:
                edges.setdefault(m.group(1), []).append(m.group(2))
    return frame, edges, alias


def solve(frame, edges, alias):
    memo = {}

    def resolve(name):
        if name in frame:
            return name
        return alias.get(name.split(':')[-1], name)

    def depth(n, stack):
        n = resolve(n)
        if n in memo:
            return memo[n]
        if n in stack:
            raise SystemExit('рекурсія: ' + ' > '.join(stack + [n]))
        own = frame.get(n, EXTERNAL.get(n.split(':')[-1], 0))
        best, path = 0, []
        targets = list(edges.get(n, []))
        if '__indirect_call' in targets:
            targets = [t for t in targets if t != '__indirect_call'] + indirect_targets(n)
        for t in targets:
            if resolve(t) not in frame and t.split(':')[-1] not in EXTERNAL:
                continue
            if (n.split(':')[-1], t.split(':')[-1]) in SKIP_EDGES or t.split(':')[-1] in TERMINAL:
                continue
            d, p = depth(t, stack + [n])
            if d > best:
                best, path = d, p
        memo[n] = (own + best, [n.split(':')[-1]] + path)
        return memo[n]

    out = {}
    for task, entry in TASKS.items():
        d, p = depth(entry, [])
        out[task] = (d + CTX_FRAME, p)
    return out


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else 'firmware/build-su'
    frame, edges, alias = load(d)
    for task, (b, p) in solve(frame, edges, alias).items():
        line = f'TASK name={task} entry={TASKS[task]} static_bytes={b}'
        if '--path' in sys.argv:
            line += ' path=' + '>'.join(p)
        print(line)


if __name__ == '__main__':
    main()
