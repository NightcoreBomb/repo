# Дані порівняння super loop ↔ FreeRTOS (згенеровано tools/lr4_compare.py)

> Обидві прошивки прогнано тими самими сценаріями `tests/sim/lr3_*.yaml` у Wokwi в один день (tools/run_lab4.sh). Вердикт: зміна ≤ 5 % — «без змін»; «довідково» — метрика без «кращого» напряму.

## Розмір і складність коду

| Показник | ЛР3 super loop | ЛР4 FreeRTOS | Δ |
|---|---:|---:|---:|
| Flash (text+data), Б | 23 992 | 31 544 | +7 552 (+31 %) |
| RAM статична (data+bss), Б | 4 304 | 11 664 | +7 360 (+171 %) |
| з них heap FreeRTOS (стеки задач, TCB, черги, м'ютекси), Б | 0 | 5 888 | +5 888 |
| Оптимізація | -Os | -Os -flto (без LTO не влазить у 32 КБ) | |
| Власний код, NLOC (App+Platform+Core/Src+Interfaces, lizard) | 1 998 | 2 852 | +854 (+43 %) |
| — з них App | 1 103 | 1 638 | +535 |
| — з них Platform | 437 | 682 | +245 (os_freertos.c 225) |
| — з них тестова оснастка ОС (`#ifdef LR4_OSTEST`) | — | 194 | |
| Функцій | 184 | 283 | +99 |
| Середня цикломатична складність (CCN) | 3.3 | 3.1 | −0.2 |
| Найскладніша функція | app_handle_line, CCN 35 | app_handle_line, CCN 41 | +6 (нові команди tasks/trace/os) |
| `volatile` у власному коді | 14 | 29 | +15 |
| Критичні секції / призупинення планувальника | 1 | 6 | +5 |
| Сторонній код ядра (tasks/queue/list/port/heap_4), рядків | 0 | 12 789 | |
| FreeRTOSConfig.h, непорожніх рядків | — | 109 | |

## Метрики продукту

| Сценарій | Метрика | ЛР3 super loop | ЛР4 FreeRTOS | Δ абс. | Δ відн. | Вердикт |
|---|---|---:|---:|---:|---:|---|
| boot_power | BOOT: джитер IMU max | 63.8 мс | 1.03 мс | -62.8 мс | -98 % | **краще** |
| boot_power | BOOT: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| boot_power | BOOT: пропущені періоди IMU | 1 | 0 | -1 | -100 % | **краще** |
| boot_power | BOOT: LAT cmd avg | 23.7 мс | 2.77 мс | -20.9 мс | -88 % | **краще** |
| boot_power | BOOT: LAT cmd max | 68.3 мс | 4.09 мс | -64.2 мс | -94 % | **краще** |
| boot_power | BOOT: cmd max | 1.29 мс | 1.29 мс | 1 мкс | +0 % | довідково |
| boot_power | BOOT: imu max | 2.06 мс | 2.14 мс | 72 мкс | +3 % | довідково |
| boot_power | BOOT: pot max | 23 мкс | 22 мкс | -1 мкс | -4 % | довідково |
| boot_power | BOOT: i2c_burst max | 1.63 мс | 1.71 мс | 81 мкс | +5 % | довідково |
| boot_power | BOOT: ctrl max | 24 мкс | 25 мкс | 1 мкс | +4 % | довідково |
| boot_power | BOOT: CPU max | 0 % | 0 % | 0 % | — | без змін |
| boot_power | BOOT: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| boot_power | BOOT: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.24 мс | +4 % | без змін |
| boot_power | BOOT: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| breakpoint | RATE_200: джитер IMU max | 66.8 мс | 1.03 мс | -65.8 мс | -98 % | **краще** |
| breakpoint | RATE_200: джитер IMU p99 | 100.0 мс | 1.00 мс | -99.0 мс | -99 % | **краще** |
| breakpoint | RATE_200: пропущені періоди IMU | 16 | 0 | -16 | -100 % | **краще** |
| breakpoint | RATE_200: LAT cmd avg | 32.8 мс | 1.92 мс | -30.9 мс | -94 % | **краще** |
| breakpoint | RATE_200: LAT cmd max | 63.7 мс | 3.60 мс | -60.1 мс | -94 % | **краще** |
| breakpoint | RATE_200: cmd max | 1.67 мс | 6.43 мс | 4.76 мс | +285 % | довідково |
| breakpoint | RATE_200: imu max | 2.13 мс | 5.68 мс | 3.55 мс | +167 % | довідково |
| breakpoint | RATE_200: pot max | 23 мкс | 61 мкс | 38 мкс | +165 % | довідково |
| breakpoint | RATE_200: oled max | 62.9 мс | 145.3 мс | 82.4 мс | +131 % | довідково |
| breakpoint | RATE_200: telem max | 2.62 мс | 35.0 мс | 32.4 мс | +1237 % | довідково |
| breakpoint | RATE_200: i2c_burst max | 1.73 мс | 5.29 мс | 3.56 мс | +206 % | довідково |
| breakpoint | RATE_200: ctrl max | 24 мкс | 82 мкс | 58 мкс | +242 % | довідково |
| breakpoint | RATE_200: oled_txn max | 3.53 мс | 10.5 мс | 6.99 мс | +198 % | довідково |
| breakpoint | RATE_200: uart_line max | 2.58 мс | 34.9 мс | 32.4 мс | +1253 % | довідково |
| breakpoint | RATE_200: CPU max | 63 % | 85 % | 22 % | +35 % | **гірше** |
| breakpoint | RATE_200: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| breakpoint | RATE_200: OLED кадр/с | 5.2 | 4.9 | -0.3 | -6 % | **гірше** |
| breakpoint | RATE_200: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| breakpoint | RATE_200: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| breakpoint | RATE_300: джитер IMU max | 65.6 мс | 3.08 мс | -62.5 мс | -95 % | **краще** |
| breakpoint | RATE_300: джитер IMU p99 | 100.0 мс | 5.00 мс | -95.0 мс | -95 % | **краще** |
| breakpoint | RATE_300: пропущені періоди IMU | 15 | 2 | -13 | -87 % | **краще** |
| breakpoint | RATE_300: LAT cmd avg | 952 мкс | 10.0 мс | 9.06 мс | +951 % | **гірше** |
| breakpoint | RATE_300: LAT cmd max | 1.85 мс | 17.6 мс | 15.8 мс | +852 % | **гірше** |
| breakpoint | RATE_300: cmd max | 1.66 мс | 33.0 мс | 31.3 мс | +1887 % | довідково |
| breakpoint | RATE_300: imu max | 2.07 мс | 5.93 мс | 3.86 мс | +187 % | довідково |
| breakpoint | RATE_300: pot max | 23 мкс | 61 мкс | 38 мкс | +165 % | довідково |
| breakpoint | RATE_300: oled max | 62.9 мс | 399.6 мс | 336.7 мс | +536 % | довідково |
| breakpoint | RATE_300: telem max | 2.62 мс | 165.1 мс | 162.5 мс | +6203 % | довідково |
| breakpoint | RATE_300: i2c_burst max | 1.63 мс | 5.50 мс | 3.87 мс | +237 % | довідково |
| breakpoint | RATE_300: ctrl max | 38 мкс | 81 мкс | 43 мкс | +113 % | довідково |
| breakpoint | RATE_300: oled_txn max | 3.53 мс | 45.8 мс | 42.3 мс | +1197 % | довідково |
| breakpoint | RATE_300: uart_line max | 2.58 мс | 147.4 мс | 144.8 мс | +5603 % | довідково |
| breakpoint | RATE_300: CPU max | 75 % | 100 % | 25 % | +33 % | **гірше** |
| breakpoint | RATE_300: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| breakpoint | RATE_300: OLED кадр/с | 5.0 | 2.6 | -2.4 | -48 % | **гірше** |
| breakpoint | RATE_300: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| breakpoint | RATE_300: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| event_longop | EVENT_LONGOP: джитер IMU max | 1042.8 мс | 115.0 мс | -927.8 мс | -89 % | **краще** |
| event_longop | EVENT_LONGOP: джитер IMU p99 | 100.0 мс | 5.00 мс | -95.0 мс | -95 % | **краще** |
| event_longop | EVENT_LONGOP: пропущені періоди IMU | 437 | 72 | -365 | -84 % | **краще** |
| event_longop | EVENT_LONGOP: LAT btn_db avg | 39.3 мс | 110 мкс | -39.2 мс | -100 % | **краще** |
| event_longop | EVENT_LONGOP: LAT btn_db max | 882.4 мс | 192 мкс | -882.2 мс | -100 % | **краще** |
| event_longop | EVENT_LONGOP: LAT btn_edge avg | 91.2 мс | 51.8 мс | -39.4 мс | -43 % | **краще** |
| event_longop | EVENT_LONGOP: LAT btn_edge max | 933.4 мс | 54.1 мс | -879.3 мс | -94 % | **краще** |
| event_longop | EVENT_LONGOP: LAT cmd avg | 8.27 мс | 3.62 мс | -4.65 мс | -56 % | **краще** |
| event_longop | EVENT_LONGOP: LAT cmd max | 62.9 мс | 37.8 мс | -25.1 мс | -40 % | **краще** |
| event_longop | EVENT_LONGOP: LAT imu2pwm avg | 2.22 мс | 6.95 мс | 4.73 мс | +213 % | **гірше** |
| event_longop | EVENT_LONGOP: LAT imu2pwm max | 5.40 мс | 23.1 мс | 17.7 мс | +327 % | **гірше** |
| event_longop | EVENT_LONGOP: cmd max | 1034.5 мс | 89477.6 мс | 88443.2 мс | +8550 % | довідково |
| event_longop | EVENT_LONGOP: btn max | 54.0 мс | 60.4 мс | 6.41 мс | +12 % | довідково |
| event_longop | EVENT_LONGOP: imu max | 38.5 мс | 65.4 мс | 26.9 мс | +70 % | довідково |
| event_longop | EVENT_LONGOP: pot max | 38 мкс | 80 мкс | 42 мкс | +111 % | довідково |
| event_longop | EVENT_LONGOP: rec max | 97 мкс | 2.53 мс | 2.43 мс | +2505 % | довідково |
| event_longop | EVENT_LONGOP: play max | 8.29 мс | 26.4 мс | 18.1 мс | +218 % | довідково |
| event_longop | EVENT_LONGOP: oled max | 64.1 мс | 243.7 мс | 179.6 мс | +280 % | довідково |
| event_longop | EVENT_LONGOP: i2c_burst max | 35.0 мс | 62.0 мс | 27.0 мс | +77 % | довідково |
| event_longop | EVENT_LONGOP: ctrl max | 169 мкс | 182 мкс | 13 мкс | +8 % | довідково |
| event_longop | EVENT_LONGOP: sd_write max | 4.64 мс | 4.12 мс | -520 мкс | -11 % | довідково |
| event_longop | EVENT_LONGOP: sd_read max | 5.17 мс | 28.3 мс | 23.1 мс | +448 % | довідково |
| event_longop | EVENT_LONGOP: oled_txn max | 4.44 мс | 110.4 мс | 106.0 мс | +2389 % | довідково |
| event_longop | EVENT_LONGOP: long_op max | 1029.2 мс | 6596.0 мс | 5566.8 мс | +541 % | довідково |
| event_longop | EVENT_LONGOP: CPU max | 99 % | 100 % | 1 % | +1 % | без змін |
| event_longop | EVENT_LONGOP: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| event_longop | EVENT_LONGOP: журнал пропусків >25 мс | 7 | 0 | -7 | -100 % | **краще** |
| event_longop | EVENT_LONGOP: журнал макс. інтервал | 81 мс | 24 мс | -57 мс | -70 % | **краще** |
| event_longop | EVENT_LONGOP: OLED кадр/с | 4.9 | 5.0 | 0.1 | +2 % | без змін |
| event_longop | EVENT_LONGOP: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| event_longop | EVENT_LONGOP: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| event_longop | кнопка debounce→реакція median | 1.87 мс | 109 мкс | -1.76 мс | -94 % | **краще** |
| event_longop | кнопка debounce→реакція max | 882.4 мс | 192 мкс | -882.2 мс | -100 % | **краще** |
| event_longop | довга операція dump max | 1029.3 мс | 6596.0 мс | 5566.8 мс | +541 % | **гірше** |
| fault | FAULT: джитер IMU max | 990.4 мс | 125.0 мс | -865.4 мс | -87 % | **краще** |
| fault | FAULT: пропущені періоди IMU | 28 | 23 | -5 | -18 % | **краще** |
| fault | FAULT: LAT cmd avg | 41.4 мс | 5.32 мс | -36.0 мс | -87 % | **краще** |
| fault | FAULT: LAT cmd max | 100.7 мс | 20.6 мс | -80.1 мс | -80 % | **краще** |
| fault | FAULT: cmd max | 2.31 мс | 28.3 мс | 25.9 мс | +1120 % | довідково |
| fault | FAULT: imu max | 25.0 мс | 75.1 мс | 50.1 мс | +201 % | довідково |
| fault | FAULT: pot max | 23 мкс | 61 мкс | 38 мкс | +165 % | довідково |
| fault | FAULT: oled max | 967.9 мс | 1215.1 мс | 247.2 мс | +26 % | довідково |
| fault | FAULT: telem max | 2.62 мс | 54.8 мс | 52.1 мс | +1989 % | довідково |
| fault | FAULT: i2c_burst max | 22.1 мс | 74.9 мс | 52.9 мс | +240 % | довідково |
| fault | FAULT: ctrl max | 24 мкс | 82 мкс | 58 мкс | +242 % | довідково |
| fault | FAULT: oled_txn max | 54.0 мс | 79.0 мс | 25.1 мс | +46 % | довідково |
| fault | FAULT: uart_line max | 2.59 мс | 54.7 мс | 52.1 мс | +2015 % | довідково |
| fault | FAULT: CPU max | 99 % | 100 % | 1 % | +1 % | без змін |
| fault | FAULT: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| fault | FAULT: OLED кадр/с | 3.7 | 4.3 | 0.6 | +16 % | **краще** |
| fault | FAULT: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| fault | FAULT: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| fr12_noimu | NOIMU: джитер IMU max | 74.8 мс | 62 мкс | -74.8 мс | -100 % | **краще** |
| fr12_noimu | NOIMU: джитер IMU p99 | 100.0 мс | 100 мкс | -99.9 мс | -100 % | **краще** |
| fr12_noimu | NOIMU: пропущені періоди IMU | 11 | 0 | -11 | -100 % | **краще** |
| fr12_noimu | NOIMU: LAT cmd avg | 23.9 мс | 2.99 мс | -20.9 мс | -87 % | **краще** |
| fr12_noimu | NOIMU: LAT cmd max | 64.0 мс | 8.29 мс | -55.7 мс | -87 % | **краще** |
| fr12_noimu | NOIMU: cmd max | 9.80 мс | 10.8 мс | 1.01 мс | +10 % | довідково |
| fr12_noimu | NOIMU: imu max | 103 мкс | 69 мкс | -34 мкс | -33 % | довідково |
| fr12_noimu | NOIMU: pot max | 36 мкс | 22 мкс | -14 мкс | -39 % | довідково |
| fr12_noimu | NOIMU: oled max | 62.9 мс | 72.3 мс | 9.39 мс | +15 % | довідково |
| fr12_noimu | NOIMU: telem max | 2.61 мс | 6.07 мс | 3.46 мс | +133 % | довідково |
| fr12_noimu | NOIMU: oled_txn max | 3.52 мс | 4.91 мс | 1.38 мс | +39 % | довідково |
| fr12_noimu | NOIMU: uart_line max | 2.57 мс | 6.03 мс | 3.46 мс | +134 % | довідково |
| fr12_noimu | NOIMU: CPU max | 39 % | 41 % | 2 % | +5 % | **гірше** |
| fr12_noimu | NOIMU: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| fr12_noimu | NOIMU: OLED кадр/с | 5.2 | 4.4 | -0.8 | -15 % | **гірше** |
| fr12_noimu | NOIMU: перший відлік IMU після скиду | 0 мкс | 0 мкс | 0 мкс | — | без змін |
| fr12_noimu | NOIMU: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| long_run | LONG_35s: джитер IMU max | 78.4 мс | 89.0 мс | 10.6 мс | +13 % | **гірше** |
| long_run | LONG_35s: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| long_run | LONG_35s: пропущені періоди IMU | 181 | 1 | -180 | -99 % | **краще** |
| long_run | LONG_35s: LAT cmd avg | 24.4 мс | 2.70 мс | -21.7 мс | -89 % | **краще** |
| long_run | LONG_35s: LAT cmd max | 71.2 мс | 4.92 мс | -66.3 мс | -93 % | **краще** |
| long_run | LONG_35s: LAT imu2pwm avg | 2.17 мс | 3.42 мс | 1.25 мс | +57 % | **гірше** |
| long_run | LONG_35s: LAT imu2pwm max | 2.23 мс | 6.09 мс | 3.86 мс | +173 % | **гірше** |
| long_run | LONG_35s: cmd max | 5.07 мс | 10.1 мс | 4.99 мс | +98 % | довідково |
| long_run | LONG_35s: imu max | 2.30 мс | 39.4 мс | 37.1 мс | +1616 % | довідково |
| long_run | LONG_35s: pot max | 36 мкс | 61 мкс | 25 мкс | +69 % | довідково |
| long_run | LONG_35s: rec max | 14.0 мс | 30.1 мс | 16.1 мс | +114 % | довідково |
| long_run | LONG_35s: oled max | 63.0 мс | 195.1 мс | 132.2 мс | +210 % | довідково |
| long_run | LONG_35s: telem max | 3.05 мс | 67.1 мс | 64.0 мс | +2102 % | довідково |
| long_run | LONG_35s: i2c_burst max | 1.63 мс | 36.0 мс | 34.3 мс | +2106 % | довідково |
| long_run | LONG_35s: ctrl max | 171 мкс | 202 мкс | 31 мкс | +18 % | довідково |
| long_run | LONG_35s: sd_write max | 3.99 мс | 9.36 мс | 5.37 мс | +134 % | довідково |
| long_run | LONG_35s: oled_txn max | 3.54 мс | 41.2 мс | 37.7 мс | +1065 % | довідково |
| long_run | LONG_35s: uart_line max | 3.00 мс | 67.0 мс | 64.0 мс | +2136 % | довідково |
| long_run | LONG_35s: CPU max | 65 % | 92 % | 27 % | +42 % | **гірше** |
| long_run | LONG_35s: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| long_run | LONG_35s: журнал пропусків >25 мс | 175 | 2 | -173 | -99 % | **краще** |
| long_run | LONG_35s: журнал макс. інтервал | 82 мс | 72 мс | -10 мс | -12 % | **краще** |
| long_run | LONG_35s: OLED кадр/с | 4.9 | 4.9 | 0 | +0 % | без змін |
| long_run | LONG_35s: перший відлік IMU після скиду | 208.9 мс | 218.1 мс | 9.21 мс | +4 % | без змін |
| long_run | LONG_35s: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| long_run | LONG_70s: джитер IMU max | 124.7 мс | 89.0 мс | -35.7 мс | -29 % | **краще** |
| long_run | LONG_70s: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| long_run | LONG_70s: пропущені періоди IMU | 364 | 5 | -359 | -99 % | **краще** |
| long_run | LONG_70s: LAT cmd avg | 25.2 мс | 2.44 мс | -22.7 мс | -90 % | **краще** |
| long_run | LONG_70s: LAT cmd max | 71.2 мс | 4.92 мс | -66.3 мс | -93 % | **краще** |
| long_run | LONG_70s: LAT imu2pwm avg | 2.17 мс | 3.40 мс | 1.23 мс | +57 % | **гірше** |
| long_run | LONG_70s: LAT imu2pwm max | 2.29 мс | 6.09 мс | 3.80 мс | +166 % | **гірше** |
| long_run | LONG_70s: cmd max | 127.3 мс | 279.5 мс | 152.2 мс | +120 % | довідково |
| long_run | LONG_70s: imu max | 2.36 мс | 39.4 мс | 37.1 мс | +1574 % | довідково |
| long_run | LONG_70s: pot max | 36 мкс | 62 мкс | 26 мкс | +72 % | довідково |
| long_run | LONG_70s: rec max | 14.0 мс | 30.4 мс | 16.4 мс | +117 % | довідково |
| long_run | LONG_70s: oled max | 63.0 мс | 466.8 мс | 403.8 мс | +641 % | довідково |
| long_run | LONG_70s: telem max | 3.05 мс | 67.1 мс | 64.0 мс | +2102 % | довідково |
| long_run | LONG_70s: i2c_burst max | 1.73 мс | 36.0 мс | 34.2 мс | +1980 % | довідково |
| long_run | LONG_70s: ctrl max | 171 мкс | 202 мкс | 31 мкс | +18 % | довідково |
| long_run | LONG_70s: sd_write max | 3.99 мс | 9.36 мс | 5.37 мс | +134 % | довідково |
| long_run | LONG_70s: oled_txn max | 3.54 мс | 295.8 мс | 292.2 мс | +8267 % | довідково |
| long_run | LONG_70s: uart_line max | 3.01 мс | 67.0 мс | 64.0 мс | +2128 % | довідково |
| long_run | LONG_70s: CPU max | 71 % | 100 % | 29 % | +41 % | **гірше** |
| long_run | LONG_70s: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| long_run | LONG_70s: журнал пропусків >25 мс | 352 | 9 | -343 | -97 % | **краще** |
| long_run | LONG_70s: журнал макс. інтервал | 140 мс | 108 мс | -32 мс | -23 % | **краще** |
| long_run | LONG_70s: OLED кадр/с | 5.0 | 4.9 | -0.1 | -2 % | без змін |
| long_run | LONG_70s: перший відлік IMU після скиду | 208.9 мс | 218.1 мс | 9.21 мс | +4 % | без змін |
| long_run | LONG_70s: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| long_run | LONG_105s: джитер IMU max | 125.9 мс | 89.0 мс | -36.9 мс | -29 % | **краще** |
| long_run | LONG_105s: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| long_run | LONG_105s: пропущені періоди IMU | 547 | 13 | -534 | -98 % | **краще** |
| long_run | LONG_105s: LAT cmd avg | 27.7 мс | 1.80 мс | -25.9 мс | -93 % | **краще** |
| long_run | LONG_105s: LAT cmd max | 71.2 мс | 4.92 мс | -66.3 мс | -93 % | **краще** |
| long_run | LONG_105s: LAT imu2pwm avg | 2.17 мс | 3.41 мс | 1.24 мс | +57 % | **гірше** |
| long_run | LONG_105s: LAT imu2pwm max | 2.29 мс | 8.19 мс | 5.90 мс | +258 % | **гірше** |
| long_run | LONG_105s: cmd max | 128.3 мс | 289.6 мс | 161.2 мс | +126 % | довідково |
| long_run | LONG_105s: imu max | 2.36 мс | 39.5 мс | 37.2 мс | +1578 % | довідково |
| long_run | LONG_105s: pot max | 38 мкс | 64 мкс | 26 мкс | +68 % | довідково |
| long_run | LONG_105s: rec max | 14.0 мс | 30.4 мс | 16.4 мс | +117 % | довідково |
| long_run | LONG_105s: oled max | 63.0 мс | 466.8 мс | 403.8 мс | +641 % | довідково |
| long_run | LONG_105s: telem max | 3.06 мс | 67.1 мс | 64.0 мс | +2095 % | довідково |
| long_run | LONG_105s: i2c_burst max | 1.73 мс | 36.0 мс | 34.2 мс | +1980 % | довідково |
| long_run | LONG_105s: ctrl max | 171 мкс | 218 мкс | 47 мкс | +27 % | довідково |
| long_run | LONG_105s: sd_write max | 3.99 мс | 9.36 мс | 5.37 мс | +134 % | довідково |
| long_run | LONG_105s: oled_txn max | 3.54 мс | 295.8 мс | 292.2 мс | +8267 % | довідково |
| long_run | LONG_105s: uart_line max | 3.02 мс | 67.0 мс | 64.0 мс | +2119 % | довідково |
| long_run | LONG_105s: CPU max | 72 % | 100 % | 28 % | +39 % | **гірше** |
| long_run | LONG_105s: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| long_run | LONG_105s: журнал пропусків >25 мс | 528 | 22 | -506 | -96 % | **краще** |
| long_run | LONG_105s: журнал макс. інтервал | 140 мс | 114 мс | -26 мс | -19 % | **краще** |
| long_run | LONG_105s: OLED кадр/с | 5.0 | 4.9 | -0.1 | -2 % | без змін |
| long_run | LONG_105s: перший відлік IMU після скиду | 208.9 мс | 218.1 мс | 9.21 мс | +4 % | без змін |
| long_run | LONG_105s: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| longop | LONGOP: джитер IMU max | 1409.7 мс | 89.0 мс | -1320.7 мс | -94 % | **краще** |
| longop | LONGOP: пропущені періоди IMU | 14 | 125 | 111 | +793 % | **гірше** |
| longop | LONGOP: LAT cmd avg | 13.6 мс | 3.14 мс | -10.5 мс | -77 % | **краще** |
| longop | LONGOP: LAT cmd max | 70.1 мс | 6.78 мс | -63.3 мс | -90 % | **краще** |
| longop | LONGOP: LAT imu2pwm avg | 2.16 мс | 2.15 мс | -12 мкс | -1 % | без змін |
| longop | LONGOP: LAT imu2pwm max | 2.22 мс | 2.32 мс | 100 мкс | +4 % | без змін |
| longop | LONGOP: cmd max | 1412.3 мс | 89477.6 мс | 88065.3 мс | +6235 % | довідково |
| longop | LONGOP: imu max | 2.29 мс | 39.5 мс | 37.2 мс | +1624 % | довідково |
| longop | LONGOP: pot max | 23 мкс | 61 мкс | 38 мкс | +165 % | довідково |
| longop | LONGOP: oled max | 62.9 мс | 232.6 мс | 169.7 мс | +270 % | довідково |
| longop | LONGOP: telem max | 3.04 мс | 101.2 мс | 98.1 мс | +3224 % | довідково |
| longop | LONGOP: i2c_burst max | 1.63 мс | 36.0 мс | 34.3 мс | +2110 % | довідково |
| longop | LONGOP: ctrl max | 169 мкс | 140 мкс | -29 мкс | -17 % | довідково |
| longop | LONGOP: sd_read max | 3.78 мс | 159.3 мс | 155.5 мс | +4117 % | довідково |
| longop | LONGOP: oled_txn max | 3.53 мс | 89.8 мс | 86.3 мс | +2446 % | довідково |
| longop | LONGOP: uart_line max | 3.01 мс | 101.1 мс | 98.1 мс | +3264 % | довідково |
| longop | LONGOP: long_op max | 1403.4 мс | 33809.0 мс | 32405.6 мс | +2309 % | довідково |
| longop | LONGOP: CPU max | 99 % | 100 % | 1 % | +1 % | без змін |
| longop | LONGOP: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| longop | LONGOP: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| longop | LONGOP: журнал макс. інтервал | 0 мс | 0 мс | 0 мс | — | без змін |
| longop | LONGOP: OLED кадр/с | 1.6 | 5.0 | 3.4 | +212 % | **краще** |
| longop | LONGOP: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| longop | LONGOP: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| longop | довга операція dump max | 1028.3 мс | 7612.4 мс | 6584.1 мс | +640 % | **гірше** |
| longop | довга операція calib max | 1403.5 мс | 33809.1 мс | 32405.6 мс | +2309 % | **гірше** |
| s1_idle | S1: джитер IMU max | 66.2 мс | 90.0 мс | 23.8 мс | +36 % | **гірше** |
| s1_idle | S1: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| s1_idle | S1: пропущені періоди IMU | 50 | 2 | -48 | -96 % | **краще** |
| s1_idle | S1: LAT cmd avg | 32.6 мс | 209 мкс | -32.4 мс | -99 % | **краще** |
| s1_idle | S1: LAT cmd max | 63.2 мс | 273 мкс | -62.9 мс | -100 % | **краще** |
| s1_idle | S1: cmd max | 1.48 мс | 1.57 мс | 91 мкс | +6 % | довідково |
| s1_idle | S1: imu max | 2.17 мс | 40.2 мс | 38.1 мс | +1757 % | довідково |
| s1_idle | S1: pot max | 23 мкс | 22 мкс | -1 мкс | -4 % | довідково |
| s1_idle | S1: oled max | 62.9 мс | 162.1 мс | 99.2 мс | +158 % | довідково |
| s1_idle | S1: telem max | 2.71 мс | 43.0 мс | 40.3 мс | +1488 % | довідково |
| s1_idle | S1: i2c_burst max | 1.73 мс | 35.0 мс | 33.2 мс | +1922 % | довідково |
| s1_idle | S1: ctrl max | 37 мкс | 63 мкс | 26 мкс | +70 % | довідково |
| s1_idle | S1: oled_txn max | 3.53 мс | 41.5 мс | 38.0 мс | +1075 % | довідково |
| s1_idle | S1: uart_line max | 2.68 мс | 43.0 мс | 40.3 мс | +1507 % | довідково |
| s1_idle | S1: CPU max | 61 % | 85 % | 24 % | +39 % | **гірше** |
| s1_idle | S1: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s1_idle | S1: OLED кадр/с | 4.9 | 4.9 | 0 | +0 % | без змін |
| s1_idle | S1: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| s1_idle | S1: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| s2_nominal | S2_rec: джитер IMU max | 79.0 мс | 89.0 мс | 10.00 мс | +13 % | **гірше** |
| s2_nominal | S2_rec: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| s2_nominal | S2_rec: пропущені періоди IMU | 110 | 4 | -106 | -96 % | **краще** |
| s2_nominal | S2_rec: LAT btn_db avg | 17.5 мс | 109 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_rec: LAT btn_db max | 17.5 мс | 109 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_rec: LAT btn_edge avg | 68.5 мс | 51.8 мс | -16.7 мс | -24 % | **краще** |
| s2_nominal | S2_rec: LAT btn_edge max | 68.5 мс | 51.8 мс | -16.7 мс | -24 % | **краще** |
| s2_nominal | S2_rec: LAT cmd avg | 1.00 мс | 7.57 мс | 6.57 мс | +656 % | **гірше** |
| s2_nominal | S2_rec: LAT cmd max | 2.02 мс | 20.9 мс | 18.8 мс | +931 % | **гірше** |
| s2_nominal | S2_rec: LAT imu2pwm avg | 2.18 мс | 3.41 мс | 1.23 мс | +56 % | **гірше** |
| s2_nominal | S2_rec: LAT imu2pwm max | 2.30 мс | 6.45 мс | 4.16 мс | +181 % | **гірше** |
| s2_nominal | S2_rec: cmd max | 10.0 мс | 24.1 мс | 14.1 мс | +140 % | довідково |
| s2_nominal | S2_rec: btn max | 10.3 мс | 10.5 мс | 207 мкс | +2 % | довідково |
| s2_nominal | S2_rec: imu max | 38.3 мс | 39.4 мс | 1.14 мс | +3 % | довідково |
| s2_nominal | S2_rec: pot max | 36 мкс | 61 мкс | 25 мкс | +69 % | довідково |
| s2_nominal | S2_rec: rec max | 14.0 мс | 29.5 мс | 15.5 мс | +110 % | довідково |
| s2_nominal | S2_rec: oled max | 63.0 мс | 190.0 мс | 127.0 мс | +202 % | довідково |
| s2_nominal | S2_rec: telem max | 3.05 мс | 51.1 мс | 48.1 мс | +1578 % | довідково |
| s2_nominal | S2_rec: i2c_burst max | 35.1 мс | 36.0 мс | 823 мкс | +2 % | довідково |
| s2_nominal | S2_rec: ctrl max | 171 мкс | 202 мкс | 31 мкс | +18 % | довідково |
| s2_nominal | S2_rec: sd_write max | 3.99 мс | 9.20 мс | 5.21 мс | +131 % | довідково |
| s2_nominal | S2_rec: oled_txn max | 3.54 мс | 54.0 мс | 50.5 мс | +1427 % | довідково |
| s2_nominal | S2_rec: uart_line max | 3.00 мс | 51.1 мс | 48.1 мс | +1604 % | довідково |
| s2_nominal | S2_rec: CPU max | 67 % | 92 % | 25 % | +37 % | **гірше** |
| s2_nominal | S2_rec: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s2_nominal | S2_rec: журнал пропусків >25 мс | 101 | 6 | -95 | -94 % | **краще** |
| s2_nominal | S2_rec: журнал макс. інтервал | 82 мс | 75 мс | -7 мс | -9 % | **краще** |
| s2_nominal | S2_rec: OLED кадр/с | 5.0 | 4.9 | -0.1 | -2 % | без змін |
| s2_nominal | S2_rec: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| s2_nominal | S2_rec: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| s2_nominal | S2_play: джитер IMU max | 60.2 мс | 114.0 мс | 53.8 мс | +89 % | **гірше** |
| s2_nominal | S2_play: джитер IMU p99 | 100.0 мс | 1.00 мс | -99.0 мс | -99 % | **краще** |
| s2_nominal | S2_play: пропущені періоди IMU | 106 | 1 | -105 | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_db avg | 17.5 мс | 109 мкс | -17.3 мс | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_db max | 17.5 мс | 109 мкс | -17.3 мс | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_edge avg | 70.2 мс | 52.1 мс | -18.1 мс | -26 % | **краще** |
| s2_nominal | S2_play: LAT btn_edge max | 70.2 мс | 52.1 мс | -18.1 мс | -26 % | **краще** |
| s2_nominal | S2_play: LAT cmd avg | 4.08 мс | 7.05 мс | 2.96 мс | +73 % | **гірше** |
| s2_nominal | S2_play: LAT cmd max | 6.33 мс | 13.9 мс | 7.61 мс | +120 % | **гірше** |
| s2_nominal | S2_play: cmd max | 1.56 мс | 4.52 мс | 2.96 мс | +189 % | довідково |
| s2_nominal | S2_play: btn max | 53.9 мс | 56.7 мс | 2.84 мс | +5 % | довідково |
| s2_nominal | S2_play: imu max | 2.10 мс | 64.3 мс | 62.2 мс | +2961 % | довідково |
| s2_nominal | S2_play: pot max | 36 мкс | 61 мкс | 25 мкс | +69 % | довідково |
| s2_nominal | S2_play: play max | 13.8 мс | 28.5 мс | 14.8 мс | +107 % | довідково |
| s2_nominal | S2_play: oled max | 63.0 мс | 561.8 мс | 498.8 мс | +792 % | довідково |
| s2_nominal | S2_play: telem max | 3.07 мс | 65.9 мс | 62.8 мс | +2045 % | довідково |
| s2_nominal | S2_play: i2c_burst max | 1.63 мс | 61.0 мс | 59.3 мс | +3640 % | довідково |
| s2_nominal | S2_play: sd_write max | 5.07 мс | 4.09 мс | -977 мкс | -19 % | довідково |
| s2_nominal | S2_play: sd_read max | 3.78 мс | 8.73 мс | 4.95 мс | +131 % | довідково |
| s2_nominal | S2_play: oled_txn max | 3.54 мс | 67.5 мс | 64.0 мс | +1808 % | довідково |
| s2_nominal | S2_play: uart_line max | 3.04 мс | 65.8 мс | 62.8 мс | +2069 % | довідково |
| s2_nominal | S2_play: CPU max | 66 % | 100 % | 34 % | +52 % | **гірше** |
| s2_nominal | S2_play: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s2_nominal | S2_play: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| s2_nominal | S2_play: журнал макс. інтервал | 0 мс | 22 мс | 22 мс | — | **гірше** |
| s2_nominal | S2_play: OLED кадр/с | 4.9 | 5.0 | 0.1 | +2 % | без змін |
| s2_nominal | S2_play: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.29 мс | +4 % | без змін |
| s2_nominal | S2_play: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| s2_nominal | кнопка debounce→реакція median | 17.5 мс | 109 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | кнопка debounce→реакція max | 53.4 мс | 109 мкс | -53.3 мс | -100 % | **краще** |
| s3_worst | S3: джитер IMU max | 593.6 мс | 111.0 мс | -482.6 мс | -81 % | **краще** |
| s3_worst | S3: джитер IMU p99 | 100.0 мс | 5.00 мс | -95.0 мс | -95 % | **краще** |
| s3_worst | S3: пропущені періоди IMU | 233 | 36 | -197 | -85 % | **краще** |
| s3_worst | S3: LAT btn_db avg | 20.2 мс | 108 мкс | -20.1 мс | -99 % | **краще** |
| s3_worst | S3: LAT btn_db max | 80.2 мс | 109 мкс | -80.1 мс | -100 % | **краще** |
| s3_worst | S3: LAT btn_edge avg | 72.2 мс | 51.8 мс | -20.4 мс | -28 % | **краще** |
| s3_worst | S3: LAT btn_edge max | 132.9 мс | 54.1 мс | -78.8 мс | -59 % | **краще** |
| s3_worst | S3: LAT cmd avg | 37.4 мс | 27.6 мс | -9.82 мс | -26 % | **краще** |
| s3_worst | S3: LAT cmd max | 106.2 мс | 77.9 мс | -28.4 мс | -27 % | **краще** |
| s3_worst | S3: LAT imu2pwm avg | 2.18 мс | 3.48 мс | 1.30 мс | +60 % | **гірше** |
| s3_worst | S3: LAT imu2pwm max | 5.24 мс | 17.5 мс | 12.2 мс | +233 % | **гірше** |
| s3_worst | S3: cmd max | 596.1 мс | 35.0 мс | -561.1 мс | -94 % | довідково |
| s3_worst | S3: btn max | 54.0 мс | 57.3 мс | 3.32 мс | +6 % | довідково |
| s3_worst | S3: imu max | 37.8 мс | 61.7 мс | 23.9 мс | +63 % | довідково |
| s3_worst | S3: pot max | 124 мкс | 61 мкс | -63 мкс | -51 % | довідково |
| s3_worst | S3: rec max | 243 мкс | 2.66 мс | 2.42 мс | +995 % | довідково |
| s3_worst | S3: play max | 8.26 мс | 20.7 мс | 12.4 мс | +150 % | довідково |
| s3_worst | S3: oled max | 64.5 мс | 358.5 мс | 294.0 мс | +456 % | довідково |
| s3_worst | S3: telem max | 3.42 мс | 165.6 мс | 162.2 мс | +4736 % | довідково |
| s3_worst | S3: i2c_burst max | 34.4 мс | 59.6 мс | 25.2 мс | +73 % | довідково |
| s3_worst | S3: ctrl max | 514 мкс | 57.7 мс | 57.2 мс | +11127 % | довідково |
| s3_worst | S3: sd_write max | 4.00 мс | 5.05 мс | 1.06 мс | +26 % | довідково |
| s3_worst | S3: sd_read max | 3.85 мс | 4.48 мс | 629 мкс | +16 % | довідково |
| s3_worst | S3: oled_txn max | 4.08 мс | 160.3 мс | 156.2 мс | +3824 % | довідково |
| s3_worst | S3: uart_line max | 3.39 мс | 165.5 мс | 162.2 мс | +4786 % | довідково |
| s3_worst | S3: CPU max | 91 % | 100 % | 9 % | +10 % | **гірше** |
| s3_worst | S3: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s3_worst | S3: журнал пропусків >25 мс | 4 | 0 | -4 | -100 % | **краще** |
| s3_worst | S3: журнал макс. інтервал | 90 мс | 23 мс | -67 мс | -74 % | **краще** |
| s3_worst | S3: OLED кадр/с | 4.9 | 4.6 | -0.3 | -6 % | **гірше** |
| s3_worst | S3: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| s3_worst | S3: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| s3_worst | кнопка debounce→реакція median | 10.8 мс | 109 мкс | -10.7 мс | -99 % | **краще** |
| s3_worst | кнопка debounce→реакція max | 80.2 мс | 109 мкс | -80.1 мс | -100 % | **краще** |
| uart_flood | FLOOD_idle: джитер IMU max | 70.2 мс | 1.03 мс | -69.1 мс | -99 % | **краще** |
| uart_flood | FLOOD_idle: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| uart_flood | FLOOD_idle: пропущені періоди IMU | 20 | 0 | -20 | -100 % | **краще** |
| uart_flood | FLOOD_idle: LAT cmd avg | 24.8 мс | 21.7 мс | -3.14 мс | -13 % | **краще** |
| uart_flood | FLOOD_idle: LAT cmd max | 51.3 мс | 60.8 мс | 9.45 мс | +18 % | **гірше** |
| uart_flood | FLOOD_idle: cmd max | 9.97 мс | 22.8 мс | 12.8 мс | +129 % | довідково |
| uart_flood | FLOOD_idle: imu max | 2.26 мс | 5.63 мс | 3.37 мс | +149 % | довідково |
| uart_flood | FLOOD_idle: pot max | 36 мкс | 22 мкс | -14 мкс | -39 % | довідково |
| uart_flood | FLOOD_idle: i2c_burst max | 1.63 мс | 5.15 мс | 3.53 мс | +217 % | довідково |
| uart_flood | FLOOD_idle: ctrl max | 157 мкс | 46 мкс | -111 мкс | -71 % | довідково |
| uart_flood | FLOOD_idle: oled_txn max | 3.53 мс | 480.0 мс | 476.5 мс | +13487 % | довідково |
| uart_flood | FLOOD_idle: CPU max | 0 % | 0 % | 0 % | — | без змін |
| uart_flood | FLOOD_idle: UART RX відкинуто | 10 Б | 6 Б | -4 Б | -40 % | **краще** |
| uart_flood | FLOOD_idle: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| uart_flood | FLOOD_idle: журнал макс. інтервал | 0 мс | 0 мс | 0 мс | — | без змін |
| uart_flood | FLOOD_idle: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| uart_flood | FLOOD_idle: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| uart_flood | FLOOD_longop: джитер IMU max | 300.2 мс | 95.0 мс | -205.2 мс | -68 % | **краще** |
| uart_flood | FLOOD_longop: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| uart_flood | FLOOD_longop: пропущені періоди IMU | 62 | 5 | -57 | -92 % | **краще** |
| uart_flood | FLOOD_longop: LAT cmd avg | 197.1 мс | 25.2 мс | -172.0 мс | -87 % | **краще** |
| uart_flood | FLOOD_longop: LAT cmd max | 375.3 мс | 68.5 мс | -306.8 мс | -82 % | **краще** |
| uart_flood | FLOOD_longop: cmd max | 302.9 мс | 29.2 мс | -273.7 мс | -90 % | довідково |
| uart_flood | FLOOD_longop: imu max | 2.21 мс | 45.5 мс | 43.3 мс | +1960 % | довідково |
| uart_flood | FLOOD_longop: pot max | 23 мкс | 61 мкс | 38 мкс | +165 % | довідково |
| uart_flood | FLOOD_longop: oled max | 62.9 мс | 1235.6 мс | 1172.7 мс | +1863 % | довідково |
| uart_flood | FLOOD_longop: i2c_burst max | 1.73 мс | 36.0 мс | 34.2 мс | +1981 % | довідково |
| uart_flood | FLOOD_longop: ctrl max | 37 мкс | 46 мкс | 9 мкс | +24 % | довідково |
| uart_flood | FLOOD_longop: sd_read max | 3.78 мс | 1395.7 мс | 1391.9 мс | +36861 % | довідково |
| uart_flood | FLOOD_longop: oled_txn max | 3.53 мс | 409.5 мс | 406.0 мс | +11487 % | довідково |
| uart_flood | FLOOD_longop: long_op max | 297.9 мс | 3373.6 мс | 3075.7 мс | +1032 % | довідково |
| uart_flood | FLOOD_longop: CPU max | 87 % | 100 % | 13 % | +15 % | **гірше** |
| uart_flood | FLOOD_longop: UART RX відкинуто | 469 Б | 438 Б | -31 Б | -7 % | **краще** |
| uart_flood | FLOOD_longop: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| uart_flood | FLOOD_longop: журнал макс. інтервал | 0 мс | 0 мс | 0 мс | — | без змін |
| uart_flood | FLOOD_longop: OLED кадр/с | 3.8 | 5.3 | 1.5 | +39 % | **краще** |
| uart_flood | FLOOD_longop: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| uart_flood | FLOOD_longop: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| uart_flood | довга операція dump max | 298.0 мс | 3373.7 мс | 3075.7 мс | +1032 % | **гірше** |
| vcd_btn | VCD: джитер IMU max | 1496.6 мс | 91.0 мс | -1405.6 мс | -94 % | **краще** |
| vcd_btn | VCD: джитер IMU p99 | 100.0 мс | 50.0 мс | -50.0 мс | -50 % | **краще** |
| vcd_btn | VCD: пропущені періоди IMU | 49 | 33 | -16 | -33 % | **краще** |
| vcd_btn | VCD: LAT btn_db avg | 550.1 мс | 108 мкс | -550.0 мс | -100 % | **краще** |
| vcd_btn | VCD: LAT btn_db max | 1299.9 мс | 109 мкс | -1299.7 мс | -100 % | **краще** |
| vcd_btn | VCD: LAT btn_edge avg | 602.5 мс | 52.1 мс | -550.4 мс | -91 % | **краще** |
| vcd_btn | VCD: LAT btn_edge max | 1352.7 мс | 54.1 мс | -1298.6 мс | -96 % | **краще** |
| vcd_btn | VCD: LAT cmd avg | 16.0 мс | 6.92 мс | -9.07 мс | -57 % | **краще** |
| vcd_btn | VCD: LAT cmd max | 71.2 мс | 38.6 мс | -32.6 мс | -46 % | **краще** |
| vcd_btn | VCD: LAT imu2pwm avg | 2.17 мс | 3.31 мс | 1.14 мс | +53 % | **гірше** |
| vcd_btn | VCD: LAT imu2pwm max | 2.29 мс | 7.28 мс | 4.99 мс | +218 % | **гірше** |
| vcd_btn | VCD: cmd max | 1446.9 мс | 69.8 мс | -1377.1 мс | -95 % | довідково |
| vcd_btn | VCD: btn max | 53.0 мс | 53.6 мс | 671 мкс | +1 % | довідково |
| vcd_btn | VCD: imu max | 2.78 мс | 57.9 мс | 55.1 мс | +1979 % | довідково |
| vcd_btn | VCD: pot max | 36 мкс | 61 мкс | 25 мкс | +69 % | довідково |
| vcd_btn | VCD: rec max | 112 мкс | 2.58 мс | 2.47 мс | +2206 % | довідково |
| vcd_btn | VCD: play max | 7.92 мс | 15.5 мс | 7.53 мс | +95 % | довідково |
| vcd_btn | VCD: oled max | 63.3 мс | 241.4 мс | 178.2 мс | +282 % | довідково |
| vcd_btn | VCD: i2c_burst max | 1.78 мс | 38.0 мс | 36.2 мс | +2034 % | довідково |
| vcd_btn | VCD: ctrl max | 170 мкс | 182 мкс | 12 мкс | +7 % | довідково |
| vcd_btn | VCD: sd_write max | 3.99 мс | 9.19 мс | 5.20 мс | +130 % | довідково |
| vcd_btn | VCD: sd_read max | 5.13 мс | 3.93 мс | -1.20 мс | -23 % | довідково |
| vcd_btn | VCD: oled_txn max | 3.54 мс | 106.5 мс | 102.9 мс | +2910 % | довідково |
| vcd_btn | VCD: CPU max | 96 % | 99 % | 3 % | +3 % | без змін |
| vcd_btn | VCD: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| vcd_btn | VCD: журнал пропусків >25 мс | 3 | 9 | 6 | +200 % | **гірше** |
| vcd_btn | VCD: журнал макс. інтервал | 79 мс | 108 мс | 29 мс | +37 % | **гірше** |
| vcd_btn | VCD: OLED кадр/с | 3.3 | 4.8 | 1.5 | +45 % | **краще** |
| vcd_btn | VCD: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.25 мс | +4 % | без змін |
| vcd_btn | VCD: стек MSP виміряно | 724 Б | 576 Б | -148 Б | -20 % | довідково |
| vcd_btn | кнопка debounce→реакція median | 999.8 мс | 109 мкс | -999.6 мс | -100 % | **краще** |
| vcd_btn | кнопка debounce→реакція max | 1299.9 мс | 109 мкс | -1299.7 мс | -100 % | **краще** |

Підсумок: краще 117, гірше 48, без змін 54, довідково 187.

## Метрики ОС

### heapsmall.log

    ==== LR4 FreeRTOS | fw=3474c6a6-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    FATAL os_task_new cmd
    FATAL os_task_new oled
    FATAL os_task_new long_op
    FATAL assert Middlewares/FreeRTOS/Source/tasks.c:3753 task=-

### nocheck.log

    ==== LR4 FreeRTOS | fw=3c16e2aa-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    OVF start stack_words=64 need_bytes=~400 pad=OK
    OVF survived (check disabled?)
    OVF end (no hook)

### os_tests.log

| Задача | prio (base) | стан | CPU, % | стек, Б | мін. вільно, Б | використано, Б |
|---|---|---|---:|---:|---:|---:|
| btn | 6 (6) | B | 0.0 | 416 | 292 | 124 |
| imu | 5 (5) | B | 47.0 | 560 | 216 | 344 |
| journal | 4 (4) | B | 0.5 | 448 | 292 | 156 |
| cmd | 3 (3) | R | 8.9 | 880 | 460 | 420 |
| telem | 2 (2) | B | 0.1 | 480 | 312 | 168 |
| oled | 2 (2) | r | 33.5 | 672 | 204 | 468 |
| long_op | 1 (1) | B | 0.0 | 624 | 340 | 284 |
| starve | 6 (6) | D | 9.2 | 256 | 148 | 108 |
| IDLE | 0 (0) | r | 0.0 | 256 | 164 | 92 |

    HEAP total=7168 free=1144 min_ever_free=56 used_max=7112 malloc_fail=2

    OSTICK n=3230 min_ns=18750 avg_ns=28145 max_ns=75500

    CTXSW n=2388 per_s=796

    MSP stack_used=624 stack_size=768

    ==== LR4 FreeRTOS | fw=6956c414-445c53b | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CTXSW_COST n=200 min_cyc=4704 avg_cyc=6651 max_cyc=123676 avg_ns=138562
    INV kind=mutex h_wait_us=49516 l_hold_us=50037 m_busy_us=300000 heap_ok=1
    INV kind=binsem h_wait_us=300604 l_hold_us=300828 m_busy_us=300000 heap_ok=1
    STARVE start ms=300
    STARVE end
    STARVE;
    QFLOOD n=50 len=8 consumer_ms=10 nb_sent=21 nb_dropped=29 nb_us=164707 blk_sent=50 blk_max_wait_us=20095 consumed=62
    HEAPX blocks=8 block=128 free_before=1144 free_after=56 hook_calls=2 queue_create=FAIL
    OVF start stack_words=64 need_bytes=~400 pad=OK
    FATAL stack overflow task=ovf

### s3_os.log

| Задача | prio (base) | стан | CPU, % | стек, Б | мін. вільно, Б | використано, Б |
|---|---|---|---:|---:|---:|---:|
| btn | 6 (6) | B | 2.3 | 416 | 96 | 320 |
| imu | 5 (5) | B | 48.3 | 560 | 196 | 364 |
| journal | 4 (4) | B | 1.0 | 448 | 152 | 296 |
| cmd | 3 (3) | R | 3.7 | 880 | 364 | 516 |
| telem | 2 (2) | r | 3.4 | 480 | 172 | 308 |
| oled | 2 (2) | r | 33.1 | 672 | 204 | 468 |
| long_op | 1 (1) | r | 7.9 | 624 | 204 | 420 |
| IDLE | 0 (0) | r | 0.1 | 256 | 164 | 92 |

    HEAP total=5888 free=216 min_ever_free=216 used_max=5672 malloc_fail=0

    OSTICK n=12718 min_ns=18750 avg_ns=27770 max_ns=117687

    CTXSW n=8300 per_s=691

    MSP stack_used=576 stack_size=768

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====

Хронологія перемикань (trace, від `trace start`):

| t, мкс від першого | Δ, мкс | задача |
|---:|---:|---|
| 0 | 0 | imu |
| 2462 | 2462 | cmd |
| 2880 | 418 | oled |
| 4994 | 2114 | imu |
| 5294 | 300 | oled |
| 6846 | 1552 | imu |
| 44394 | 37548 | journal |
| 44567 | 173 | oled |
| 44975 | 408 | telem |
| 45973 | 998 | oled |
| 46975 | 1002 | telem |
| 47973 | 998 | oled |
| 48975 | 1002 | telem |
| 49973 | 998 | oled |
| 50975 | 1002 | telem |
| 51231 | 256 | oled |
| 63994 | 12763 | journal |
| 64156 | 162 | oled |
| 83994 | 19838 | journal |
| 84157 | 163 | oled |
| 93994 | 9837 | imu |
| 94211 | 217 | oled |
| 97323 | 3112 | long_op |
| 98993 | 1670 | imu |
| 101425 | 2432 | long_op |
| 103483 | 2058 | imu |
| 106818 | 3335 | imu |
| 109423 | 2605 | imu |
| 111811 | 2388 | journal |
| 112020 | 209 | cmd |
| 112181 | 161 | telem |
| 112975 | 794 | oled |
| 113995 | 1020 | imu |
| 114300 | 305 | oled |
| 115140 | 840 | imu |
| 117495 | 2355 | oled |
| 117975 | 480 | telem |
| 118992 | 1017 | imu |
| 119289 | 297 | oled |
| 122561 | 3272 | imu |
| 124869 | 2308 | imu |
| 127345 | 2476 | oled |
| 127975 | 630 | telem |
| 128991 | 1016 | imu |
| 129293 | 302 | oled |
| 132464 | 3171 | imu |
| 134768 | 2304 | imu |
| 137233 | 2465 | journal |

### s3_os_telem20_timeout.log

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====

### s3_tick100.log

    ==== LR4 FreeRTOS | fw=f2375c6d-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=100 | reset=0x00 ====

### vcd_trace.log

    ==== LR4 FreeRTOS | fw=fd9e8eb3-827d8d5 | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====

