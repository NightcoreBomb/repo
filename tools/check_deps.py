#!/usr/bin/env python3
"""
check_deps.py — формальна перевірка правил залежностей між шарами прошивки.

Правило «хто кого може #include-ити» (docs/architecture.md §2):

  Interfaces → (лише стандартна бібліотека C)
  Devices    → Interfaces, Devices
  App        → Interfaces, Devices, App
  Platform   → Interfaces, Platform, Vendor (HAL/CMSIS)
  Core       → будь-що (точка складання)
  Vendor     — не перевіряється (код ST)

Додатково для App/ і Devices/: у коді (без коментарів і рядків) не повинно
бути ідентифікаторів HAL_*, __HAL_*, *_HandleTypeDef, GPIOx, а також
#include <stm32...>. Це ловить «обхідні» залежності, коли заголовок HAL
потрапив транзитивно або тип оголошено вручну.

Використання:  python3 tools/check_deps.py [firmware_dir]   (exit 1 при порушенні)
"""
import os, re, sys

LAYERS = ['Interfaces', 'Devices', 'App', 'Platform', 'Core']
ALLOWED = {
    'Interfaces': {'Interfaces'},
    'Devices':    {'Interfaces', 'Devices'},
    'App':        {'Interfaces', 'Devices', 'App'},
    'Platform':   {'Interfaces', 'Platform', 'Vendor'},
    'Core':       {'Interfaces', 'Devices', 'App', 'Platform', 'Core', 'Vendor'},
}
STD = {'stdint.h', 'stddef.h', 'stdbool.h', 'string.h', 'stdlib.h', 'stdio.h', 'stdarg.h',
       'limits.h', 'errno.h', 'assert.h', 'math.h', 'sys/stat.h', 'sys/times.h', 'time.h',
       'signal.h', 'sys/time.h'}
HAL_TOKENS = re.compile(r'\b(HAL_\w+|__HAL_\w+|\w+_HandleTypeDef|GPIO[A-F]\b|stm32\w*\.h)')
INC = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)

def strip_comments(src):
    src = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), src, flags=re.S)
    return re.sub(r'//[^\n]*', '', src)

def strip_strings(code):
    # ВАЖЛИВО: лише ПІСЛЯ вилучення #include — інакше "app.h" стає "" і
    # include-и непомітно зникають з перевірки (журнал проблем, П-7)
    return re.sub(r'"(\\.|[^"\\])*"', '""', code)

def index_headers(fw):
    where = {}
    for layer in LAYERS:
        base = os.path.join(fw, layer)
        for root, _, files in os.walk(base):
            for f in files:
                if f.endswith('.h'): where.setdefault(f, layer)
    # ЛР4: ядро FreeRTOS (Middlewares/) — вендорський код, як HAL: доступне лише Platform і Core;
    # App отримує сервіси ОС через Interfaces/os.h
    for vend in ('Drivers', 'Middlewares'):
        for root, _, files in os.walk(os.path.join(fw, vend)):
            for f in files:
                if f.endswith('.h'): where.setdefault(f, 'Vendor')
    return where

def layer_of(fw, path):
    rel = os.path.relpath(path, fw).split(os.sep)[0]
    return rel if rel in LAYERS else None

def check(fw):
    where, errors, nfiles, nincl = index_headers(fw), [], 0, 0
    for layer in LAYERS:
        for root, _, files in os.walk(os.path.join(fw, layer)):
            for f in sorted(files):
                if not f.endswith(('.c', '.h')): continue
                p = os.path.join(root, f); rel = os.path.relpath(p, fw); nfiles += 1
                src = open(p, encoding='utf-8').read()
                code = strip_comments(src)
                for m in INC.finditer(code):
                    nincl += 1
                    kind, name = m.group(1), m.group(2)
                    if name in STD: continue
                    tgt = where.get(os.path.basename(name))
                    if tgt is None:
                        if kind == '<':   # невідомий системний — дозволено лише Core/Platform
                            if layer not in ('Core', 'Platform'):
                                errors.append('%s: <%s> — не зі стандартної бібліотеки' % (rel, name))
                            continue
                        errors.append('%s: "%s" — заголовок не знайдено в жодному шарі' % (rel, name)); continue
                    if tgt not in ALLOWED[layer]:
                        errors.append('%s: #include "%s" (%s → %s) порушує правило шарів' % (rel, name, layer, tgt))
                if layer in ('App', 'Devices', 'Interfaces'):
                    for ln, line in enumerate(strip_strings(code).split('\n'), 1):
                        for t in HAL_TOKENS.findall(line):
                            errors.append('%s:%d: ідентифікатор апаратного шару «%s»' % (rel, ln, t))
    return errors, nfiles, nincl

if __name__ == '__main__':
    fw = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'firmware')
    errs, n, k = check(os.path.abspath(fw))
    for e in errs: print('DEPS ERROR:', e)
    print('check_deps: %d files, %d includes, %d violation(s)' % (n, k, len(errs)))
    sys.exit(1 if errs else 0)
