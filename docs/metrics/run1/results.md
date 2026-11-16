# Результати прогонів ЛР3 (згенеровано tools/lr3_metrics.py)

> Усі часові числа отримано **в симуляторі Wokwi** (Форма B) — див. lab3-report.md §4.5
> щодо того, яким із них можна довіряти.

## boot_power.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=FAIL
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=FAIL
    DIR NONE blocks=0 records=0 duration_ms=0
    DIR OK blocks=3 records=601 duration_ms=12020
    DIR OK blocks=3 records=601 duration_ms=12020

    REC stop records=601 blocks=3 duration_ms=12020 gaps=60 max_int_ms=83
    PLAY done records=601 duration_ms=12020 rec_ms=12020
    PLAY done

## breakpoint.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0

#### `RATE_200026184,0,0,1500,1500,LIVE` — fw `51601fab-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 2000 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| loop | 925 | 2.11 мс | 3.24 мс | 65.1 мс |
| cmd | 2 | 1.15 мс | 1.40 мс | 1.65 мс |
| imu | 926 | 2.00 мс | 2.02 мс | 2.12 мс |
| pot | 60 | 23 мкс | 23 мкс | 36 мкс |
| oled | 15 | 62.8 мс | 62.9 мс | 62.9 мс |
| telem | 30 | 2.71 мс | 2.72 мс | 2.73 мс |
| i2c_burst | 926 | 1.61 мс | 1.62 мс | 1.73 мс |
| ctrl | 926 | 23 мкс | 24 мкс | 37 мкс |
| oled_txn | 300 | 1.50 мс | 3.09 мс | 3.53 мс |
| uart_line | 30 | 2.67 мс | 2.68 мс | 2.69 мс |
| **LAT cmd** | 2 | 432 мкс | 2.54 мс | 4.66 мс |

Джитер IMU (період 500 мкс): n=925, min 1.58 мс, max 64.6 мс, p99 ≤ 100.0 мс, пропущено (>2 періодів) 925
CPU: останнє вікно 96 %, мін 96 %, макс 96 % (3 вікон по 1 с)
UART RX: 245 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 6538
OLED: 15 кадрів, 4.9 кадр/с
Стек: виміряно 724 Б із 1024 Б; статична RAM 2756 Б; heap 0; перший відлік IMU через 208.8 мс після скиду


## event_longop.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR dump busy: recording
    ERR dump busy: recording
    ERR dump busy: recording

Кнопка (EVT): debounce→реакція n=40, min 16 мкс, median 1.63 мс, p99 880.3 мс, max 880.3 мс

Кнопка (EVT): перший фронт→реакція n=40, min 5.81 мс, median 53.5 мс, p99 89469.6 мс, max 89469.6 мс

Довга операція `dump` (EVT dur_us): n=27, min 152.1 мс, median 152.2 мс, p99 1029.2 мс, max 1029.2 мс

    REC stop records=1501 blocks=7 duration_ms=30020 gaps=150 max_int_ms=82
    REC stop records=61 blocks=1 duration_ms=1220 gaps=6 max_int_ms=80
    REC stop records=79 blocks=1 duration_ms=1580 gaps=8 max_int_ms=81
    REC stop records=104 blocks=1 duration_ms=2080 gaps=11 max_int_ms=79
    REC stop records=106 blocks=1 duration_ms=2120 gaps=10 max_int_ms=82
    REC stop records=92 blocks=1 duration_ms=1840 gaps=9 max_int_ms=79
    REC stop records=77 blocks=1 duration_ms=1540 gaps=7 max_int_ms=83
    REC stop records=94 blocks=1 duration_ms=1880 gaps=9 max_int_ms=83
    PLAY done records=61 duration_ms=1220 rec_ms=1220
    PLAY done records=69 duration_ms=1537 rec_ms=1580
    PLAY done records=67 duration_ms=1510 rec_ms=2080
    PLAY done records=70 duration_ms=1557 rec_ms=1840
    PLAY done records=70 duration_ms=1540 rec_ms=1540
    PLAY done records=71 duration_ms=1559 rec_ms=1880
    PLAY done records=71 duration_ms=1561 rec_ms=2020
    PLAY done records=71 duration_ms=1560 rec_ms=2380

## fault.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0


## fr02_static.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0


## fr12_noimu.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=NACK who=0x00 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR IMU not found: servo centered, LIVE disabled

    STATUS mode=LIVE pitch_cd=0 roll_cd=0 pan_us=1500 tilt_us=1500 dz_cd=0 imu=NACK sd=1 dir=0 t=2273

## longop.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0

Довга операція `calib` (EVT dur_us): n=1, min 1404.4 мс, median 1404.4 мс, p99 1404.4 мс, max 1404.4 мс

    REC stop records=1501 blocks=7 duration_ms=30020 gaps=150 max_int_ms=82
    CALIB n=200 bias_mdps=0,0,0

## long_run.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0


## s1_idle.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0


## s2_nominal.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0

Кнопка (EVT): debounce→реакція n=3, min 13.8 мс, median 53.3 мс, p99 53.8 мс, max 53.8 мс

Кнопка (EVT): перший фронт→реакція n=3, min 65.8 мс, median 105.1 мс, p99 106.3 мс, max 106.3 мс

    REC stop records=1012 blocks=5 duration_ms=20240 gaps=0 max_int_ms=0
    PLAY done records=1012 duration_ms=20304 rec_ms=20240
    STATUS mode=REC pitch_cd=2731 roll_cd=0 pan_us=1500 tilt_us=1803 dz_cd=0 imu=OK sd=1 dir=0 t=5486
    STATUS mode=REC pitch_cd=-101 roll_cd=0 pan_us=1500 tilt_us=1489 dz_cd=0 imu=OK sd=1 dir=0 t=10485
    STATUS mode=REC pitch_cd=-2649 roll_cd=0 pan_us=1500 tilt_us=1206 dz_cd=0 imu=OK sd=1 dir=0 t=15486
    STATUS mode=REC pitch_cd=2307 roll_cd=0 pan_us=1500 tilt_us=1756 dz_cd=0 imu=OK sd=1 dir=0 t=20486

## s3_worst.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0

Кнопка (EVT): debounce→реакція n=29, min 23 мкс, median 10.4 мс, p99 80.3 мс, max 80.3 мс

Кнопка (EVT): перший фронт→реакція n=29, min 472 мкс, median 56.6 мс, p99 133.0 мс, max 133.0 мс

Довга операція `dump` (EVT dur_us): n=10, min 152.1 мс, median 152.2 мс, p99 590.9 мс, max 590.9 мс

    REC stop records=751 blocks=4 duration_ms=15020 gaps=75 max_int_ms=82
    REC stop records=46 blocks=1 duration_ms=920 gaps=5 max_int_ms=90
    REC stop records=57 blocks=1 duration_ms=1140 gaps=5 max_int_ms=90
    REC stop records=37 blocks=1 duration_ms=740 gaps=4 max_int_ms=80
    REC stop records=52 blocks=1 duration_ms=1040 gaps=5 max_int_ms=83
    REC stop records=33 blocks=1 duration_ms=660 gaps=5 max_int_ms=82
    REC stop records=55 blocks=1 duration_ms=1100 gaps=6 max_int_ms=79
    REC stop records=30 blocks=1 duration_ms=600 gaps=4 max_int_ms=83
    PLAY done records=52 duration_ms=1046 rec_ms=1040
    PLAY done records=33 duration_ms=719 rec_ms=660
    PLAY done records=30 duration_ms=603 rec_ms=600
    STATUS mode=LIVE pitch_cd=-2429 roll_cd=0 pan_us=1500 tilt_us=1230 dz_cd=0 imu=OK sd=1 dir=4 t=15329
    STATUS mode=LIVE pitch_cd=-2417 roll_cd=0 pan_us=1500 tilt_us=1231 dz_cd=0 imu=OK sd=1 dir=4 t=15341
    STATUS mode=LIVE pitch_cd=-2405 roll_cd=0 pan_us=1500 tilt_us=1233 dz_cd=0 imu=OK sd=1 dir=4 t=15354
    STATUS mode=REC pitch_cd=-2028 roll_cd=0 pan_us=1500 tilt_us=1275 dz_cd=0 imu=OK sd=1 dir=4 t=16431
    STATUS mode=REC pitch_cd=-1930 roll_cd=0 pan_us=1500 tilt_us=1286 dz_cd=0 imu=OK sd=1 dir=4 t=16444
    STATUS mode=REC pitch_cd=-1833 roll_cd=0 pan_us=1500 tilt_us=1296 dz_cd=0 imu=OK sd=1 dir=4 t=16459
    STATUS mode=PLAY pitch_cd=2497 roll_cd=0 pan_us=1500 tilt_us=1417 dz_cd=0 imu=OK sd=1 dir=1 t=17492
    STATUS mode=PLAY pitch_cd=2387 roll_cd=0 pan_us=1500 tilt_us=1449 dz_cd=0 imu=OK sd=1 dir=1 t=17505

## uart_flood.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR unknown 'snote'

    REC stop records=1501 blocks=7 duration_ms=30020 gaps=150 max_int_ms=82
    STATUS mode=LIVE pitch_cd=-556 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30328
    STATUS mode=LIVE pitch_cd=-557 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30340
    STATUS mode=LIVE pitch_cd=-558 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30353
    STATUS mode=LIVE pitch_cd=-559 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30365
    STATUS mode=LIVE pitch_cd=-560 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30378
    STATUS mode=LIVE pitch_cd=-561 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30390
    STATUS mode=LIVE pitch_cd=-562 roll_cd=0 pan_us=1500 tilt_us=1438 dz_cd=0 imu=OK sd=1 dir=7 t=30403
    STATUS mode=LIVE pitch_cd=-563 roll_cd=0 pan_us=1500 tilt_us=1437 dz_cd=0 imu=OK sd=1 dir=7 t=30478

## vcd_btn.log

    ==== LR3 super-loop | fw=51601fab-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | reset=0x00 ====
    CHK imu=OK who=0x68 oled=OK sd=OK type=SDHC adc=OK
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR dump busy: recording

Кнопка (EVT): debounce→реакція n=5, min 48 мкс, median 2.07 мс, p99 61.1 мс, max 61.1 мс

Кнопка (EVT): перший фронт→реакція n=5, min 53.0 мс, median 53.1 мс, p99 113.2 мс, max 113.2 мс

    REC stop records=1001 blocks=5 duration_ms=20020 gaps=100 max_int_ms=79
    REC stop records=46 blocks=1 duration_ms=920 gaps=5 max_int_ms=74
    REC stop records=30 blocks=1 duration_ms=600 gaps=3 max_int_ms=83
    PLAY done records=30 duration_ms=600 rec_ms=600

