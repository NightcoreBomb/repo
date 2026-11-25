# Результати прогонів ЛР3 (згенеровано tools/lr3_metrics.py)

> Усі часові числа отримано **в симуляторі Wokwi** (Форма B) — див. lab3-report.md §4.5
> щодо того, яким із них можна довіряти.

## boot_power.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `BOOT` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 896 мкс | 1.09 мс | 1.28 мс |
| imu | 26 | 2.05 мс | 2.12 мс | 2.14 мс |
| pot | 3 | 22 мкс | 22 мкс | 22 мкс |
| i2c_burst | 30 | 1.70 мс | 1.70 мс | 1.71 мс |
| ctrl | 33 | 25 мкс | 25 мкс | 25 мкс |
| bus_wait | 34 | 35 мкс | 35 мкс | 41 мкс |
| **LAT cmd** | 3 | 2.22 мс | 71.1 мс | 105.5 мс |

Джитер IMU (період 5.00 мс): n=38, min -52 мкс, max 32 мкс, p99 ≤ 100 мкс, пропущено (>2 періодів) 0
CPU: останнє вікно 0 %, мін 0 %, макс 0 % (0 вікон по 1 с)
UART RX: 24 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 44
OLED: 0 кадрів, 0.0 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

    REC stop records=600 blocks=3 duration_ms=12000 gaps=0 max_int_ms=24

## breakpoint.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `RATE_200` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.66 мс | 4.00 мс | 6.34 мс |
| imu | 606 | 2.09 мс | 3.16 мс | 5.77 мс |
| pot | 58 | 22 мкс | 22 мкс | 22 мкс |
| oled | 15 | 139.4 мс | 140.2 мс | 141.2 мс |
| telem | 30 | 9.73 мс | 20.1 мс | 34.5 мс |
| i2c_burst | 615 | 1.70 мс | 2.74 мс | 5.38 мс |
| ctrl | 617 | 24 мкс | 25 мкс | 54 мкс |
| oled_txn | 300 | 1.69 мс | 6.91 мс | 9.95 мс |
| uart_line | 30 | 9.69 мс | 19.6 мс | 34.4 мс |
| bus_wait | 624 | 35 мкс | 1.05 мс | 3.70 мс |
| **LAT cmd** | 2 | 219 мкс | 1.89 мс | 3.56 мс |

Джитер IMU (період 5.00 мс): n=628, min -957 мкс, max 956 мкс, p99 ≤ 1.00 мс, пропущено (>2 періодів) 0
CPU: останнє вікно 84 %, мін 84 %, макс 84 % (3 вікон по 1 с)
UART RX: 37 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 834
OLED: 15 кадрів, 4.9 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

#### `RATE_300` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 333 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 17.8 мс | 18.0 мс | 18.1 мс |
| imu | 1031 | 2.08 мс | 2.60 мс | 11.1 мс |
| pot | 61 | 22 мкс | 22 мкс | 22 мкс |
| oled | 8 | 309.5 мс | 332.1 мс | 372.1 мс |
| telem | 26 | 4.68 мс | 103.5 мс | 216.0 мс |
| i2c_burst | 1066 | 1.70 мс | 2.19 мс | 10.7 мс |
| ctrl | 1076 | 24 мкс | 25 мкс | 54 мкс |
| oled_txn | 179 | 6.16 мс | 16.4 мс | 41.5 мс |
| uart_line | 26 | 4.64 мс | 98.6 мс | 192.0 мс |
| bus_wait | 1103 | 33 мкс | 501 мкс | 9.03 мс |
| **LAT cmd** | 2 | 2.09 мс | 4.34 мс | 6.59 мс |

Джитер IMU (період 3.00 мс): n=1119, min -774 мкс, max 8.28 мс, p99 ≤ 5.00 мс, пропущено (>2 періодів) 7
CPU: останнє вікно 100 %, мін 100 %, макс 100 % (3 вікон по 1 с)
UART RX: 67 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 1986
OLED: 8 кадрів, 2.6 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду


## event_longop.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0


## fault.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0


## fr02_static.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

    STATUS mode=LIVE pitch_cd=0 roll_cd=0 pan_us=1500 tilt_us=1500 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2974 roll_cd=0 pan_us=1500 tilt_us=1830 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=4495 roll_cd=0 pan_us=1500 tilt_us=1999 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=-2974 roll_cd=0 pan_us=1500 tilt_us=1170 dz_cd=0 imu=

## fr12_noimu.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=NACK who=0x00 oled=
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR IMU not found: servo centered, LIVE disabled

#### `NOIMU` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.39 мс | 5.98 мс | 10.6 мс |
| imu | 405 | 33 мкс | 66 мкс | 69 мкс |
| pot | 41 | 22 мкс | 22 мкс | 22 мкс |
| oled | 9 | 71.1 мс | 71.2 мс | 71.4 мс |
| telem | 19 | 2.55 мс | 4.06 мс | 6.06 мс |
| oled_txn | 180 | 1.69 мс | 3.51 мс | 4.84 мс |
| uart_line | 19 | 2.52 мс | 4.03 мс | 6.02 мс |
| **LAT cmd** | 3 | 130 мкс | 2.77 мс | 8.01 мс |

Джитер IMU (період 5.00 мс): n=412, min -63 мкс, max 62 мкс, p99 ≤ 100 мкс, пропущено (>2 періодів) 0
CPU: останнє вікно 41 %, мін 33 %, макс 41 % (2 вікон по 1 с)
UART RX: 25 байтів, відкинуто 0, ORE 0
I²C: скидів 1, таймаутів 0, помилок IMU 0, відліків 0
OLED: 9 кадрів, 4.4 кадр/с
Стек: виміряно 568 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 0 мкс після скиду

    STATUS mode=LIVE pitch_cd=0 roll_cd=0 pan_us=1500 tilt_us=1500 dz_cd=0 imu=NACK sd=1 dir=0 t=2212

## longop.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0


## long_run.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0


## s1_idle.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `S1` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.07 мс | 1.32 мс | 1.57 мс |
| imu | 2006 | 2.09 мс | 3.19 мс | 5.77 мс |
| pot | 192 | 22 мкс | 22 мкс | 52 мкс |
| oled | 50 | 139.4 мс | 140.4 мс | 141.6 мс |
| telem | 100 | 15.2 мс | 17.9 мс | 34.6 мс |
| i2c_burst | 2015 | 1.70 мс | 2.77 мс | 5.38 мс |
| ctrl | 2018 | 24 мкс | 25 мкс | 54 мкс |
| oled_txn | 1000 | 1.69 мс | 6.94 мс | 10.0 мс |
| uart_line | 100 | 15.2 мс | 17.4 мс | 34.5 мс |
| bus_wait | 2024 | 34 мкс | 1.09 мс | 3.70 мс |
| **LAT cmd** | 2 | 177 мкс | 666 мкс | 1.16 мс |

Джитер IMU (період 5.00 мс): n=2028, min -955 мкс, max 954 мкс, p99 ≤ 1.00 мс, пропущено (>2 періодів) 0
CPU: останнє вікно 85 %, мін 84 %, макс 85 % (10 вікон по 1 с)
UART RX: 34 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 2434
OLED: 50 кадрів, 4.9 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду


## s2_nominal.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0

#### `S2_rec` — fw `18a8168e-nogit`, mode REC, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 6 | 1.68 мс | 14.8 мс | 20.8 мс |
| btn | 1 | 10.2 мс | 10.2 мс | 10.2 мс |
| imu | 4066 | 2.10 мс | 3.39 мс | 14.8 мс |
| pot | 389 | 22 мкс | 22 мкс | 71 мкс |
| rec | 1015 | 19 мкс | 163 мкс | 27.2 мс |
| oled | 101 | 145.7 мс | 158.0 мс | 181.3 мс |
| telem | 203 | 10.5 мс | 23.6 мс | 55.1 мс |
| i2c_burst | 4078 | 1.70 мс | 2.78 мс | 14.4 мс |
| ctrl | 4080 | 24 мкс | 116 мкс | 206 мкс |
| sd_write | 4 | 6.53 мс | 6.55 мс | 6.61 мс |
| oled_txn | 2030 | 1.56 мс | 7.78 мс | 39.4 мс |
| uart_line | 203 | 9.91 мс | 21.7 мс | 34.6 мс |
| bus_wait | 4090 | 34 мкс | 1.10 мс | 3.77 мс |
| **LAT btn_db** | 1 | 99 мкс | 99 мкс | 99 мкс |
| **LAT btn_edge** | 1 | 52.8 мс | 52.8 мс | 52.8 мс |
| **LAT cmd** | 6 | 138 мкс | 1.51 мс | 2.34 мс |
| **LAT imu2pwm** | 3136 | 2.21 мс | 3.34 мс | 6.00 мс |

Джитер IMU (період 5.00 мс): n=4101, min -2.75 мс, max 9.97 мс, p99 ≤ 2.00 мс, пропущено (>2 періодів) 1
CPU: останнє вікно 92 %, мін 87 %, макс 92 % (20 вікон по 1 с)
UART RX: 66 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 4308
Журнал: 1025 записів, 4 блоків, пропусків > 25 мс: 0, макс. інтервал 24 мс
OLED: 101 кадрів, 4.9 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

#### `S2_play` — fw `18a8168e-nogit`, mode PLAY, OLED-режим 0, телеметрія 100 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 2 | 1.56 мс | 2.80 мс | 4.04 мс |
| btn | 1 | 54.9 мс | 54.9 мс | 54.9 мс |
| imu | 4163 | 2.12 мс | 3.15 мс | 5.86 мс |
| pot | 414 | 22 мкс | 22 мкс | 22 мкс |
| rec | 2 | 27 мкс | 44 мкс | 62 мкс |
| play | 1030 | 70 мкс | 362 мкс | 31.4 мс |
| oled | 106 | 139.5 мс | 150.9 мс | 581.9 мс |
| telem | 211 | 10.6 мс | 19.8 мс | 120.8 мс |
| i2c_burst | 4177 | 1.70 мс | 2.71 мс | 5.43 мс |
| ctrl | 9 | 25 мкс | 25 мкс | 25 мкс |
| sd_write | 2 | 4.06 мс | 4.06 мс | 4.06 мс |
| sd_read | 5 | 4.21 мс | 7.90 мс | 11.8 мс |
| oled_txn | 2110 | 1.64 мс | 7.45 мс | 320.3 мс |
| uart_line | 211 | 10.6 мс | 19.6 мс | 120.8 мс |
| bus_wait | 4191 | 34 мкс | 1.03 мс | 3.75 мс |
| **LAT btn_db** | 1 | 101 мкс | 101 мкс | 101 мкс |
| **LAT btn_edge** | 1 | 50.1 мс | 50.1 мс | 50.1 мс |
| **LAT cmd** | 2 | 128 мкс | 9.63 мс | 19.1 мс |

Джитер IMU (період 5.00 мс): n=4199, min -2.74 мс, max 55.1 мс, p99 ≤ 1.00 мс, пропущено (>2 періодів) 1
CPU: останнє вікно 86 %, мін 86 %, макс 100 % (20 вікон по 1 с)
UART RX: 98 байтів, відкинуто 0, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 8524
Журнал: 1029 записів, 5 блоків, пропусків > 25 мс: 0, макс. інтервал 21 мс
OLED: 106 кадрів, 5.1 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

Кнопка (EVT): debounce→реакція n=3, min 99 мкс, median 100 мкс, p99 101 мкс, max 101 мкс

Кнопка (EVT): перший фронт→реакція n=3, min 50.1 мс, median 50.1 мс, p99 52.8 мс, max 52.8 мс

    REC stop records=1029 blocks=5 duration_ms=20580 gaps=0 max_int_ms=21
    PLAY done records=1029 duration_ms=20673 rec_ms=20580
    STATUS mode=REC pitch_cd=2769 roll_cd=0 pan_us=1500 tilt_us=1808 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-185 roll_cd=0 pan_us=1500 tilt_us=1479 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=-2622 roll_cd=0 pan_us=1500 tilt_us=1209 dz_cd=0 imu=
    STATUS mode=REC pitch_cd=2364 roll_cd=0 pan_us=1500 tilt_us=1763 dz_cd=0 imu=

## s3_worst.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0


## uart_flood.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CHK imu=
    DIR NONE blocks=0 records=0 duration_ms=0
    ERR unknown 's'
    ERR unknown 's'
    ERR unknown 's'

#### `FLOOD_idle` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 0 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 22 | 4.22 мс | 18.0 мс | 20.1 мс |
| imu | 109 | 2.18 мс | 2.52 мс | 5.88 мс |
| pot | 11 | 22 мкс | 22 мкс | 22 мкс |
| oled | 1 | 606.6 мс | 606.6 мс | 606.6 мс |
| telem | 1 | 469.4 мс | 469.4 мс | 469.4 мс |
| i2c_burst | 119 | 1.70 мс | 2.00 мс | 5.40 мс |
| ctrl | 121 | 24 мкс | 24 мкс | 25 мкс |
| oled_txn | 18 | 4.12 мс | 32.7 мс | 466.9 мс |
| uart_line | 1 | 469.4 мс | 469.4 мс | 469.4 мс |
| bus_wait | 128 | 35 мкс | 309 мкс | 3.73 мс |
| **LAT cmd** | 7 | 109 мкс | 20.7 мс | 59.8 мс |

Джитер IMU (період 5.00 мс): n=131, min -1.12 мс, max 1.10 мс, p99 ≤ 2.00 мс, пропущено (>2 періодів) 0
CPU: останнє вікно 0 %, мін 0 %, макс 0 % (0 вікон по 1 с)
UART RX: 210 байтів, відкинуто 6, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 1376
Журнал: 300 записів, 2 блоків, пропусків > 25 мс: 0, макс. інтервал 0 мс
OLED: 1 кадрів, 1.9 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

#### `FLOOD_longop` — fw `18a8168e-nogit`, mode LIVE, OLED-режим 0, телеметрія 0 мс, IMU 200 Гц, load 0 мкс

| Задача / виклик | n | min | avg | max |
|---|---:|---:|---:|---:|
| cmd | 67 | 160 мкс | 18.2 мс | 23.1 мс |
| imu | 2620 | 2.17 мс | 3.22 мс | 5.94 мс |
| pot | 253 | 22 мкс | 22 мкс | 30 мкс |
| oled | 69 | 131.3 мс | 143.8 мс | 754.3 мс |
| i2c_burst | 2627 | 1.70 мс | 2.73 мс | 5.43 мс |
| ctrl | 2629 | 24 мкс | 25 мкс | 44 мкс |
| sd_read | 6 | 6.28 мс | 233.5 мс | 1225.9 мс |
| oled_txn | 1367 | 1.53 мс | 6.93 мс | 385.9 мс |
| long_op | 3 | 1955.9 мс | 2320.2 мс | 3044.8 мс |
| bus_wait | 2639 | 34 мкс | 1.05 мс | 3.76 мс |
| **LAT cmd** | 22 | 72 мкс | 26.6 мс | 62.6 мс |

Джитер IMU (період 5.00 мс): n=2643, min -1.13 мс, max 1.13 мс, p99 ≤ 2.00 мс, пропущено (>2 періодів) 0
CPU: останнє вікно 88 %, мін 88 %, макс 100 % (13 вікон по 1 с)
UART RX: 1105 байтів, відкинуто 424, ORE 0
I²C: скидів 0, таймаутів 0, помилок IMU 0, відліків 4035
Журнал: 300 записів, 2 блоків, пропусків > 25 мс: 0, макс. інтервал 0 мс
OLED: 69 кадрів, 5.2 кадр/с
Стек: виміряно 592 Б із 1024 Б; статична RAM 10632 Б; heap 0; перший відлік IMU через 218.1 мс після скиду

Довга операція `dump` (EVT dur_us): n=3, min 1956.0 мс, median 1960.0 мс, p99 3044.8 мс, max 3044.8 мс

    REC stop records=300 blocks=2 duration_ms=6000 gaps=0 max_int_ms=24
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=
    STATUS mode=LIVE pitch_cd=2472 roll_cd=0 pan_us=1500 tilt_us=1775 dz_cd=0 imu=

## vcd_btn.log



