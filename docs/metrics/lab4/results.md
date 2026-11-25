# Результати прогонів ЛР3 (згенеровано tools/lr3_metrics.py)

> Усі часові числа отримано **в симуляторі Wokwi** (Форма B) — див. lab3-report.md §4.5
> щодо того, яким із них можна довіряти.

## boot_power.log



## breakpoint.log



## event_longop.log



## fault.log



## fr02_static.log



## fr12_noimu.log



## longop.log



## long_run.log



## s1_idle.log

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `S1` — fw `fd9e8eb3-827d8d5`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.09 мс | 1.33 мс | 1.57 мс |
| imu | 1970 | 2.10 мс | 3.24 мс | 40.2 мс |
| pot | 190 | 22 мкс | 22 мкс | 22 мкс |
| oled | 50 | 140.6 мс | 144.1 мс | 162.1 мс |
| telem | 100 | 12.5 мс | 19.1 мс | 43.0 мс |
| i2c_burst | 1980 | 1.71 мс | 2.81 мс | 35.0 мс |
| ctrl | 1980 | 24 мкс | 25 мкс | 63 мкс |
| oled_txn | 1000 | 1.73 мс | 7.13 мс | 41.5 мс |
| uart_line | 100 | 12.5 мс | 19.0 мс | 43.0 мс |
| bus_wait | 1989 | 37 мкс | 1575.6 мс | 89478.4 мс |
| **LAT cmd** | 2 | 145 мкс | 209 мкс | 273 мкс |

Джитер IMU (період 5.00 мс): n=1993, min -2.67 мс, max 90.0 мс, p99 ≤ 2.00 мс, пропущено (>2 періодів) 2
CPU: останнє вікно 85 %, мін 84 %, макс 85 % (10 вікон по 1 с)
UART RX: 34 байтів, відкинуто 0, ORE 0
I²C: скидів 2, таймаутів 2, помилок IMU 2, відліків 2398
OLED: 50 кадрів, 4.9 кадр/с
Стек: виміряно 576 Б із 768 Б; статична RAM 10748 Б; heap 0; перший відлік IMU через 218.1 мс після скиду


## s2_nominal.log

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `S2_rec` — fw `fd9e8eb3-827d8d5`, mode REC, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 6 | 1.47 мс | 14.0 мс | 24.1 мс |
| btn | 1 | 10.5 мс | 10.5 мс | 10.5 мс |
| imu | 4015 | 1.20 мс | 3.49 мс | 39.4 мс |
| pot | 385 | 22 мкс | 22 мкс | 61 мкс |
| rec | 1002 | 19 мкс | 168 мкс | 29.5 мс |
| oled | 101 | 148.8 мс | 162.1 мс | 190.0 мс |
| telem | 203 | 6.04 мс | 25.2 мс | 51.1 мс |
| i2c_burst | 4027 | 717 мкс | 2.87 мс | 36.0 мс |
| ctrl | 4027 | 24 мкс | 116 мкс | 202 мкс |
| sd_write | 4 | 6.56 мс | 7.25 мс | 9.20 мс |
| oled_txn | 2027 | 1.57 мс | 7.98 мс | 54.0 мс |
| uart_line | 203 | 6.00 мс | 25.1 мс | 51.1 мс |
| bus_wait | 4040 | 37 мкс | 754.2 мс | 89478.4 мс |
| **LAT btn_db** | 1 | 109 мкс | 109 мкс | 109 мкс |
| **LAT btn_edge** | 1 | 51.8 мс | 51.8 мс | 51.8 мс |
| **LAT cmd** | 6 | 620 мкс | 7.57 мс | 20.9 мс |
| **LAT imu2pwm** | 3124 | 1.22 мс | 3.41 мс | 6.45 мс |

Джитер IMU (період 5.00 мс): n=4052, min -2.75 мс, max 89.0 мс, p99 ≤ 2.00 мс, пропущено (>2 періодів) 4
CPU: останнє вікно 91 %, мін 88 %, макс 92 % (20 вікон по 1 с)
UART RX: 66 байтів, відкинуто 0, ORE 0
I²C: скидів 3, таймаутів 3, помилок IMU 3, відліків 4256
Журнал: 1013 записів, 4 блоків, пропусків > 25 мс: 6, макс. інтервал 75 мс
OLED: 101 кадрів, 4.9 кадр/с
Стек: виміряно 576 Б із 768 Б; статична RAM 10748 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

#### `S2_play` — fw `fd9e8eb3-827d8d5`, mode PLAY, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.60 мс | 3.06 мс | 4.52 мс |
| btn | 1 | 56.7 мс | 56.7 мс | 56.7 мс |
| imu | 4087 | 2.13 мс | 3.27 мс | 64.3 мс |
| pot | 395 | 22 мкс | 24 мкс | 61 мкс |
| rec | 2 | 27 мкс | 27 мкс | 27 мкс |
| play | 1018 | 70 мкс | 194 мкс | 28.5 мс |
| oled | 104 | 126.8 мс | 150.9 мс | 561.8 мс |
| telem | 208 | 6.24 мс | 22.4 мс | 65.9 мс |
| i2c_burst | 4101 | 1.71 мс | 2.83 мс | 61.0 мс |
| ctrl | 10 | 25 мкс | 36 мкс | 140 мкс |
| sd_write | 2 | 4.09 мс | 4.09 мс | 4.09 мс |
| sd_read | 5 | 3.88 мс | 6.78 мс | 8.73 мс |
| oled_txn | 2085 | 1.61 мс | 7.33 мс | 67.5 мс |
| uart_line | 208 | 6.21 мс | 22.4 мс | 65.8 мс |
| bus_wait | 4115 | 37 мкс | 88.1 мс | 89478.4 мс |
| **LAT btn_db** | 1 | 109 мкс | 109 мкс | 109 мкс |
| **LAT btn_edge** | 1 | 52.1 мс | 52.1 мс | 52.1 мс |
| **LAT cmd** | 2 | 152 мкс | 7.05 мс | 13.9 мс |
| **LAT imu2pwm** | 1 | 2.24 мс | 2.24 мс | 2.24 мс |

Джитер IMU (період 5.00 мс): n=4126, min -2.71 мс, max 114.0 мс, p99 ≤ 1.00 мс, пропущено (>2 періодів) 1
CPU: останнє вікно 86 %, мін 86 %, макс 100 % (20 вікон по 1 с)
UART RX: 98 байтів, відкинуто 0, ORE 0
I²C: скидів 5, таймаутів 4, помилок IMU 4, відліків 8398
Журнал: 1017 записів, 5 блоків, пропусків > 25 мс: 0, макс. інтервал 22 мс
OLED: 104 кадрів, 5.0 кадр/с
Стек: виміряно 576 Б із 768 Б; статична RAM 10748 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

Кнопка (EVT): debounce→реакція n=3, min 109 мкс, median 109 мкс, p99 109 мкс, max 109 мкс

Кнопка (EVT): перший фронт→реакція n=3, min 51.8 мс, median 52.1 мс, p99 53.1 мс, max 53.1 мс

    REC stop records=1017 blocks=5 duration_ms=20340 gaps=0 max_int_ms=22
    PLAY done records=1017 duration_ms=20394 rec_ms=20340
    STATUS mode=REC pitch_cd=2769 roll_cd=0 pan_us=1500 tilt_us=1808 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-161 roll_cd=0 pan_us=1500 tilt_us=1482 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-2622 roll_cd=0 pan_us=1500 tilt_us=1209 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=2353 roll_cd=0 pan_us=1500 tilt_us=1761 dz_cd=0 imu=TIMEOUT sd=1 dir=0 t=20509

## s3_worst.log

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR busy: long_op queue full
    ERR busy: long_op queue full
    ERR busy: long_op queue full

#### `S3` — fw `fd9e8eb3-827d8d5`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 103 | 100 мкс | 20.2 мс | 35.0 мс |
| btn | 30 | 10.3 мс | 26.1 мс | 57.3 мс |
| imu | 5136 | 1.13 мс | 3.64 мс | 61.7 мс |
| pot | 493 | 22 мкс | 22 мкс | 61 мкс |
| rec | 435 | 19 мкс | 39 мкс | 2.66 мс |
| play | 348 | 70 мкс | 298 мкс | 20.7 мс |
| oled | 128 | 170.9 мс | 211.2 мс | 358.5 мс |
| telem | 848 | 4.86 мс | 26.4 мс | 165.6 мс |
| i2c_burst | 5151 | 716 мкс | 3.03 мс | 59.6 мс |
| ctrl | 3514 | 24 мкс | 147 мкс | 57.7 мс |
| sd_write | 20 | 4.08 мс | 4.14 мс | 5.05 мс |
| sd_read | 10 | 3.84 мс | 3.93 мс | 4.48 мс |
| oled_txn | 2563 | 1.74 мс | 10.4 мс | 160.3 мс |
| uart_line | 848 | 4.82 мс | 26.2 мс | 165.5 мс |
| bus_wait | 5167 | 36 мкс | 53.2 мс | 89478.4 мс |
| **LAT btn_db** | 30 | 107 мкс | 108 мкс | 109 мкс |
| **LAT btn_edge** | 30 | 50.1 мс | 51.8 мс | 54.1 мс |
| **LAT cmd** | 95 | 152 мкс | 27.6 мс | 77.9 мс |
| **LAT imu2pwm** | 3161 | 1.22 мс | 3.48 мс | 17.5 мс |

Джитер IMU (період 5.00 мс): n=5179, min -3.73 мс, max 111.0 мс, p99 ≤ 5.00 мс, пропущено (>2 періодів) 36
CPU: останнє вікно 100 %, мін 100 %, макс 100 % (27 вікон по 1 с)
UART RX: 759 байтів, відкинуто 0, ORE 0
I²C: скидів 26, таймаутів 24, помилок IMU 20, відліків 8132
Журнал: 42 записів, 1 блоків, пропусків > 25 мс: 0, макс. інтервал 23 мс
OLED: 128 кадрів, 4.6 кадр/с
Стек: виміряно 576 Б із 768 Б; статична RAM 10748 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

Кнопка (EVT): debounce→реакція n=30, min 107 мкс, median 109 мкс, p99 109 мкс, max 109 мкс

Кнопка (EVT): перший фронт→реакція n=30, min 50.1 мс, median 51.8 мс, p99 54.1 мс, max 54.1 мс

    REC stop records=737 blocks=3 duration_ms=14740 gaps=5 max_int_ms=112
    REC stop records=49 blocks=1 duration_ms=980 gaps=0 max_int_ms=23
    REC stop records=58 blocks=1 duration_ms=1160 gaps=0 max_int_ms=24
    REC stop records=34 blocks=1 duration_ms=680 gaps=0 max_int_ms=23
    REC stop records=49 blocks=1 duration_ms=980 gaps=0 max_int_ms=23
    REC stop records=27 blocks=1 duration_ms=540 gaps=1 max_int_ms=108
    REC stop records=53 blocks=1 duration_ms=1060 gaps=0 max_int_ms=24
    REC stop records=33 blocks=1 duration_ms=660 gaps=0 max_int_ms=23
    PLAY done records=49 duration_ms=1031 rec_ms=980
    PLAY done records=27 duration_ms=593 rec_ms=540
    PLAY done records=33 duration_ms=751 rec_ms=660
    STATUS mode=LIVE pitch_cd=-2440 roll_cd=0 pan_us=1500 tilt_us=1229 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=-2393 roll_cd=0 pan_us=1500 tilt_us=1234 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=-2350 roll_cd=0 pan_us=1500 tilt_us=1239 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-1847 roll_cd=0 pan_us=1500 tilt_us=1295 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-1391 roll_cd=0 pan_us=1500 tilt_us=1345 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-979 roll_cd=0 pan_us=1500 tilt_us=1391 dz_cd=0 imu=
    STATUS mode=PLAY pitch_cd=2692 roll_cd=0 pan_us=1500 tilt_us=1432 dz_cd=0 imu=
    STATUS mode=PLAY pitch_cd=2249 roll_cd=0 pan_us=1500 tilt_us=1462 dz_cd=0 imu=

## uart_flood.log



## vcd_btn.log



