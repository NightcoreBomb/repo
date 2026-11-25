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
| boot_power | BOOT: джитер IMU max | 63.8 мс | 32 мкс | -63.8 мс | -100 % | **краще** |
| boot_power | BOOT: джитер IMU p99 | 100.0 мс | 100 мкс | -99.9 мс | -100 % | **краще** |
| boot_power | BOOT: пропущені періоди IMU | 1 | 0 | -1 | -100 % | **краще** |
| boot_power | BOOT: LAT cmd avg | 23.7 мс | 71.1 мс | 47.4 мс | +200 % | **гірше** |
| boot_power | BOOT: LAT cmd max | 68.3 мс | 105.5 мс | 37.2 мс | +55 % | **гірше** |
| boot_power | BOOT: cmd max | 1.29 мс | 1.28 мс | -7 мкс | -1 % | довідково |
| boot_power | BOOT: imu max | 2.06 мс | 2.14 мс | 78 мкс | +4 % | довідково |
| boot_power | BOOT: pot max | 23 мкс | 22 мкс | -1 мкс | -4 % | довідково |
| boot_power | BOOT: i2c_burst max | 1.63 мс | 1.71 мс | 83 мкс | +5 % | довідково |
| boot_power | BOOT: ctrl max | 24 мкс | 25 мкс | 1 мкс | +4 % | довідково |
| boot_power | BOOT: CPU max | 0 % | 0 % | 0 % | — | без змін |
| boot_power | BOOT: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| boot_power | BOOT: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.22 мс | +4 % | без змін |
| boot_power | BOOT: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| breakpoint | RATE_200: джитер IMU max | 66.8 мс | 956 мкс | -65.8 мс | -99 % | **краще** |
| breakpoint | RATE_200: джитер IMU p99 | 100.0 мс | 1.00 мс | -99.0 мс | -99 % | **краще** |
| breakpoint | RATE_200: пропущені періоди IMU | 16 | 0 | -16 | -100 % | **краще** |
| breakpoint | RATE_200: LAT cmd avg | 32.8 мс | 1.89 мс | -30.9 мс | -94 % | **краще** |
| breakpoint | RATE_200: LAT cmd max | 63.7 мс | 3.56 мс | -60.1 мс | -94 % | **краще** |
| breakpoint | RATE_200: cmd max | 1.67 мс | 6.34 мс | 4.67 мс | +279 % | довідково |
| breakpoint | RATE_200: imu max | 2.13 мс | 5.77 мс | 3.64 мс | +171 % | довідково |
| breakpoint | RATE_200: pot max | 23 мкс | 22 мкс | -1 мкс | -4 % | довідково |
| breakpoint | RATE_200: oled max | 62.9 мс | 141.2 мс | 78.4 мс | +125 % | довідково |
| breakpoint | RATE_200: telem max | 2.62 мс | 34.5 мс | 31.9 мс | +1219 % | довідково |
| breakpoint | RATE_200: i2c_burst max | 1.73 мс | 5.38 мс | 3.65 мс | +211 % | довідково |
| breakpoint | RATE_200: ctrl max | 24 мкс | 54 мкс | 30 мкс | +125 % | довідково |
| breakpoint | RATE_200: oled_txn max | 3.53 мс | 9.95 мс | 6.42 мс | +182 % | довідково |
| breakpoint | RATE_200: uart_line max | 2.58 мс | 34.4 мс | 31.8 мс | +1233 % | довідково |
| breakpoint | RATE_200: CPU max | 63 % | 84 % | 21 % | +33 % | **гірше** |
| breakpoint | RATE_200: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| breakpoint | RATE_200: OLED кадр/с | 5.2 | 4.9 | -0.3 | -6 % | **гірше** |
| breakpoint | RATE_200: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.27 мс | +4 % | без змін |
| breakpoint | RATE_200: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| breakpoint | RATE_300: джитер IMU max | 65.6 мс | 8.28 мс | -57.3 мс | -87 % | **краще** |
| breakpoint | RATE_300: джитер IMU p99 | 100.0 мс | 5.00 мс | -95.0 мс | -95 % | **краще** |
| breakpoint | RATE_300: пропущені періоди IMU | 15 | 7 | -8 | -53 % | **краще** |
| breakpoint | RATE_300: LAT cmd avg | 952 мкс | 4.34 мс | 3.39 мс | +356 % | **гірше** |
| breakpoint | RATE_300: LAT cmd max | 1.85 мс | 6.59 мс | 4.74 мс | +256 % | **гірше** |
| breakpoint | RATE_300: cmd max | 1.66 мс | 18.1 мс | 16.4 мс | +989 % | довідково |
| breakpoint | RATE_300: imu max | 2.07 мс | 11.1 мс | 9.06 мс | +438 % | довідково |
| breakpoint | RATE_300: pot max | 23 мкс | 22 мкс | -1 мкс | -4 % | довідково |
| breakpoint | RATE_300: oled max | 62.9 мс | 372.1 мс | 309.2 мс | +492 % | довідково |
| breakpoint | RATE_300: telem max | 2.62 мс | 216.0 мс | 213.4 мс | +8149 % | довідково |
| breakpoint | RATE_300: i2c_burst max | 1.63 мс | 10.7 мс | 9.08 мс | +557 % | довідково |
| breakpoint | RATE_300: ctrl max | 38 мкс | 54 мкс | 16 мкс | +42 % | довідково |
| breakpoint | RATE_300: oled_txn max | 3.53 мс | 41.5 мс | 37.9 мс | +1074 % | довідково |
| breakpoint | RATE_300: uart_line max | 2.58 мс | 192.0 мс | 189.4 мс | +7328 % | довідково |
| breakpoint | RATE_300: CPU max | 75 % | 100 % | 25 % | +33 % | **гірше** |
| breakpoint | RATE_300: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| breakpoint | RATE_300: OLED кадр/с | 5.0 | 2.6 | -2.4 | -48 % | **гірше** |
| breakpoint | RATE_300: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.27 мс | +4 % | без змін |
| breakpoint | RATE_300: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| fr12_noimu | NOIMU: джитер IMU max | 74.8 мс | 62 мкс | -74.8 мс | -100 % | **краще** |
| fr12_noimu | NOIMU: джитер IMU p99 | 100.0 мс | 100 мкс | -99.9 мс | -100 % | **краще** |
| fr12_noimu | NOIMU: пропущені періоди IMU | 11 | 0 | -11 | -100 % | **краще** |
| fr12_noimu | NOIMU: LAT cmd avg | 23.9 мс | 2.77 мс | -21.1 мс | -88 % | **краще** |
| fr12_noimu | NOIMU: LAT cmd max | 64.0 мс | 8.01 мс | -56.0 мс | -87 % | **краще** |
| fr12_noimu | NOIMU: cmd max | 9.80 мс | 10.6 мс | 776 мкс | +8 % | довідково |
| fr12_noimu | NOIMU: imu max | 103 мкс | 69 мкс | -34 мкс | -33 % | довідково |
| fr12_noimu | NOIMU: pot max | 36 мкс | 22 мкс | -14 мкс | -39 % | довідково |
| fr12_noimu | NOIMU: oled max | 62.9 мс | 71.4 мс | 8.49 мс | +14 % | довідково |
| fr12_noimu | NOIMU: telem max | 2.61 мс | 6.06 мс | 3.45 мс | +132 % | довідково |
| fr12_noimu | NOIMU: oled_txn max | 3.52 мс | 4.84 мс | 1.31 мс | +37 % | довідково |
| fr12_noimu | NOIMU: uart_line max | 2.57 мс | 6.02 мс | 3.44 мс | +134 % | довідково |
| fr12_noimu | NOIMU: CPU max | 39 % | 41 % | 2 % | +5 % | **гірше** |
| fr12_noimu | NOIMU: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| fr12_noimu | NOIMU: OLED кадр/с | 5.2 | 4.4 | -0.8 | -15 % | **гірше** |
| fr12_noimu | NOIMU: перший відлік IMU після скиду | 0 мкс | 0 мкс | 0 мкс | — | без змін |
| fr12_noimu | NOIMU: стек MSP виміряно | 724 Б | 568 Б | -156 Б | -22 % | довідково |
| s1_idle | S1: джитер IMU max | 66.2 мс | 954 мкс | -65.2 мс | -99 % | **краще** |
| s1_idle | S1: джитер IMU p99 | 100.0 мс | 1.00 мс | -99.0 мс | -99 % | **краще** |
| s1_idle | S1: пропущені періоди IMU | 50 | 0 | -50 | -100 % | **краще** |
| s1_idle | S1: LAT cmd avg | 32.6 мс | 666 мкс | -31.9 мс | -98 % | **краще** |
| s1_idle | S1: LAT cmd max | 63.2 мс | 1.16 мс | -62.0 мс | -98 % | **краще** |
| s1_idle | S1: cmd max | 1.48 мс | 1.57 мс | 92 мкс | +6 % | довідково |
| s1_idle | S1: imu max | 2.17 мс | 5.77 мс | 3.60 мс | +166 % | довідково |
| s1_idle | S1: pot max | 23 мкс | 52 мкс | 29 мкс | +126 % | довідково |
| s1_idle | S1: oled max | 62.9 мс | 141.6 мс | 78.7 мс | +125 % | довідково |
| s1_idle | S1: telem max | 2.71 мс | 34.6 мс | 31.9 мс | +1175 % | довідково |
| s1_idle | S1: i2c_burst max | 1.73 мс | 5.38 мс | 3.65 мс | +211 % | довідково |
| s1_idle | S1: ctrl max | 37 мкс | 54 мкс | 17 мкс | +46 % | довідково |
| s1_idle | S1: oled_txn max | 3.53 мс | 10.0 мс | 6.49 мс | +184 % | довідково |
| s1_idle | S1: uart_line max | 2.68 мс | 34.5 мс | 31.8 мс | +1190 % | довідково |
| s1_idle | S1: CPU max | 61 % | 85 % | 24 % | +39 % | **гірше** |
| s1_idle | S1: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s1_idle | S1: OLED кадр/с | 4.9 | 4.9 | 0 | +0 % | без змін |
| s1_idle | S1: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.27 мс | +4 % | без змін |
| s1_idle | S1: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| s2_nominal | S2_rec: джитер IMU max | 79.0 мс | 9.97 мс | -69.0 мс | -87 % | **краще** |
| s2_nominal | S2_rec: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| s2_nominal | S2_rec: пропущені періоди IMU | 110 | 1 | -109 | -99 % | **краще** |
| s2_nominal | S2_rec: LAT btn_db avg | 17.5 мс | 99 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_rec: LAT btn_db max | 17.5 мс | 99 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_rec: LAT btn_edge avg | 68.5 мс | 52.8 мс | -15.7 мс | -23 % | **краще** |
| s2_nominal | S2_rec: LAT btn_edge max | 68.5 мс | 52.8 мс | -15.7 мс | -23 % | **краще** |
| s2_nominal | S2_rec: LAT cmd avg | 1.00 мс | 1.51 мс | 515 мкс | +52 % | **гірше** |
| s2_nominal | S2_rec: LAT cmd max | 2.02 мс | 2.34 мс | 318 мкс | +16 % | **гірше** |
| s2_nominal | S2_rec: LAT imu2pwm avg | 2.18 мс | 3.34 мс | 1.16 мс | +53 % | **гірше** |
| s2_nominal | S2_rec: LAT imu2pwm max | 2.30 мс | 6.00 мс | 3.70 мс | +161 % | **гірше** |
| s2_nominal | S2_rec: cmd max | 10.0 мс | 20.8 мс | 10.8 мс | +108 % | довідково |
| s2_nominal | S2_rec: btn max | 10.3 мс | 10.2 мс | -93 мкс | -1 % | довідково |
| s2_nominal | S2_rec: imu max | 38.3 мс | 14.8 мс | -23.5 мс | -61 % | довідково |
| s2_nominal | S2_rec: pot max | 36 мкс | 71 мкс | 35 мкс | +97 % | довідково |
| s2_nominal | S2_rec: rec max | 14.0 мс | 27.2 мс | 13.1 мс | +94 % | довідково |
| s2_nominal | S2_rec: oled max | 63.0 мс | 181.3 мс | 118.4 мс | +188 % | довідково |
| s2_nominal | S2_rec: telem max | 3.05 мс | 55.1 мс | 52.1 мс | +1709 % | довідково |
| s2_nominal | S2_rec: i2c_burst max | 35.1 мс | 14.4 мс | -20.7 мс | -59 % | довідково |
| s2_nominal | S2_rec: ctrl max | 171 мкс | 206 мкс | 35 мкс | +20 % | довідково |
| s2_nominal | S2_rec: sd_write max | 3.99 мс | 6.61 мс | 2.62 мс | +66 % | довідково |
| s2_nominal | S2_rec: oled_txn max | 3.54 мс | 39.4 мс | 35.8 мс | +1013 % | довідково |
| s2_nominal | S2_rec: uart_line max | 3.00 мс | 34.6 мс | 31.6 мс | +1053 % | довідково |
| s2_nominal | S2_rec: CPU max | 67 % | 92 % | 25 % | +37 % | **гірше** |
| s2_nominal | S2_rec: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s2_nominal | S2_rec: журнал пропусків >25 мс | 101 | 0 | -101 | -100 % | **краще** |
| s2_nominal | S2_rec: журнал макс. інтервал | 82 мс | 24 мс | -58 мс | -71 % | **краще** |
| s2_nominal | S2_rec: OLED кадр/с | 5.0 | 4.9 | -0.1 | -2 % | без змін |
| s2_nominal | S2_rec: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.27 мс | +4 % | без змін |
| s2_nominal | S2_rec: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| s2_nominal | S2_play: джитер IMU max | 60.2 мс | 55.1 мс | -5.13 мс | -9 % | **краще** |
| s2_nominal | S2_play: джитер IMU p99 | 100.0 мс | 1.00 мс | -99.0 мс | -99 % | **краще** |
| s2_nominal | S2_play: пропущені періоди IMU | 106 | 1 | -105 | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_db avg | 17.5 мс | 101 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_db max | 17.5 мс | 101 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | S2_play: LAT btn_edge avg | 70.2 мс | 50.1 мс | -20.0 мс | -29 % | **краще** |
| s2_nominal | S2_play: LAT btn_edge max | 70.2 мс | 50.1 мс | -20.0 мс | -29 % | **краще** |
| s2_nominal | S2_play: LAT cmd avg | 4.08 мс | 9.63 мс | 5.54 мс | +136 % | **гірше** |
| s2_nominal | S2_play: LAT cmd max | 6.33 мс | 19.1 мс | 12.8 мс | +202 % | **гірше** |
| s2_nominal | S2_play: cmd max | 1.56 мс | 4.04 мс | 2.47 мс | +158 % | довідково |
| s2_nominal | S2_play: btn max | 53.9 мс | 54.9 мс | 1.05 мс | +2 % | довідково |
| s2_nominal | S2_play: imu max | 2.10 мс | 5.86 мс | 3.76 мс | +179 % | довідково |
| s2_nominal | S2_play: pot max | 36 мкс | 22 мкс | -14 мкс | -39 % | довідково |
| s2_nominal | S2_play: play max | 13.8 мс | 31.4 мс | 17.6 мс | +128 % | довідково |
| s2_nominal | S2_play: oled max | 63.0 мс | 581.9 мс | 518.9 мс | +824 % | довідково |
| s2_nominal | S2_play: telem max | 3.07 мс | 120.8 мс | 117.8 мс | +3834 % | довідково |
| s2_nominal | S2_play: i2c_burst max | 1.63 мс | 5.43 мс | 3.80 мс | +233 % | довідково |
| s2_nominal | S2_play: sd_write max | 5.07 мс | 4.06 мс | -1.01 мс | -20 % | довідково |
| s2_nominal | S2_play: sd_read max | 3.78 мс | 11.8 мс | 7.98 мс | +211 % | довідково |
| s2_nominal | S2_play: oled_txn max | 3.54 мс | 320.3 мс | 316.7 мс | +8950 % | довідково |
| s2_nominal | S2_play: uart_line max | 3.04 мс | 120.8 мс | 117.7 мс | +3879 % | довідково |
| s2_nominal | S2_play: CPU max | 66 % | 100 % | 34 % | +52 % | **гірше** |
| s2_nominal | S2_play: UART RX відкинуто | 0 Б | 0 Б | 0 Б | — | без змін |
| s2_nominal | S2_play: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| s2_nominal | S2_play: журнал макс. інтервал | 0 мс | 21 мс | 21 мс | — | **гірше** |
| s2_nominal | S2_play: OLED кадр/с | 4.9 | 5.1 | 0.2 | +4 % | без змін |
| s2_nominal | S2_play: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.27 мс | +4 % | без змін |
| s2_nominal | S2_play: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| s2_nominal | кнопка debounce→реакція median | 17.5 мс | 100 мкс | -17.4 мс | -99 % | **краще** |
| s2_nominal | кнопка debounce→реакція max | 53.4 мс | 101 мкс | -53.3 мс | -100 % | **краще** |
| uart_flood | FLOOD_idle: джитер IMU max | 70.2 мс | 1.10 мс | -69.1 мс | -98 % | **краще** |
| uart_flood | FLOOD_idle: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| uart_flood | FLOOD_idle: пропущені періоди IMU | 20 | 0 | -20 | -100 % | **краще** |
| uart_flood | FLOOD_idle: LAT cmd avg | 24.8 мс | 20.7 мс | -4.12 мс | -17 % | **краще** |
| uart_flood | FLOOD_idle: LAT cmd max | 51.3 мс | 59.8 мс | 8.47 мс | +17 % | **гірше** |
| uart_flood | FLOOD_idle: cmd max | 9.97 мс | 20.1 мс | 10.1 мс | +101 % | довідково |
| uart_flood | FLOOD_idle: imu max | 2.26 мс | 5.88 мс | 3.62 мс | +160 % | довідково |
| uart_flood | FLOOD_idle: pot max | 36 мкс | 22 мкс | -14 мкс | -39 % | довідково |
| uart_flood | FLOOD_idle: oled max | 62.9 мс | 606.6 мс | 543.7 мс | +864 % | довідково |
| uart_flood | FLOOD_idle: i2c_burst max | 1.63 мс | 5.40 мс | 3.78 мс | +232 % | довідково |
| uart_flood | FLOOD_idle: ctrl max | 157 мкс | 25 мкс | -132 мкс | -84 % | довідково |
| uart_flood | FLOOD_idle: oled_txn max | 3.53 мс | 466.9 мс | 463.3 мс | +13114 % | довідково |
| uart_flood | FLOOD_idle: CPU max | 0 % | 0 % | 0 % | — | без змін |
| uart_flood | FLOOD_idle: UART RX відкинуто | 10 Б | 6 Б | -4 Б | -40 % | **краще** |
| uart_flood | FLOOD_idle: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| uart_flood | FLOOD_idle: журнал макс. інтервал | 0 мс | 0 мс | 0 мс | — | без змін |
| uart_flood | FLOOD_idle: OLED кадр/с | 5.4 | 1.9 | -3.5 | -65 % | **гірше** |
| uart_flood | FLOOD_idle: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.23 мс | +4 % | без змін |
| uart_flood | FLOOD_idle: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| uart_flood | FLOOD_longop: джитер IMU max | 300.2 мс | 1.13 мс | -299.1 мс | -100 % | **краще** |
| uart_flood | FLOOD_longop: джитер IMU p99 | 100.0 мс | 2.00 мс | -98.0 мс | -98 % | **краще** |
| uart_flood | FLOOD_longop: пропущені періоди IMU | 62 | 0 | -62 | -100 % | **краще** |
| uart_flood | FLOOD_longop: LAT cmd avg | 197.1 мс | 26.6 мс | -170.5 мс | -87 % | **краще** |
| uart_flood | FLOOD_longop: LAT cmd max | 375.3 мс | 62.6 мс | -312.7 мс | -83 % | **краще** |
| uart_flood | FLOOD_longop: cmd max | 302.9 мс | 23.1 мс | -279.8 мс | -92 % | довідково |
| uart_flood | FLOOD_longop: imu max | 2.21 мс | 5.94 мс | 3.73 мс | +169 % | довідково |
| uart_flood | FLOOD_longop: pot max | 23 мкс | 30 мкс | 7 мкс | +30 % | довідково |
| uart_flood | FLOOD_longop: oled max | 62.9 мс | 754.3 мс | 691.4 мс | +1099 % | довідково |
| uart_flood | FLOOD_longop: i2c_burst max | 1.73 мс | 5.43 мс | 3.71 мс | +214 % | довідково |
| uart_flood | FLOOD_longop: ctrl max | 37 мкс | 44 мкс | 7 мкс | +19 % | довідково |
| uart_flood | FLOOD_longop: sd_read max | 3.78 мс | 1225.9 мс | 1222.1 мс | +32365 % | довідково |
| uart_flood | FLOOD_longop: oled_txn max | 3.53 мс | 385.9 мс | 382.4 мс | +10819 % | довідково |
| uart_flood | FLOOD_longop: long_op max | 297.9 мс | 3044.8 мс | 2746.8 мс | +922 % | довідково |
| uart_flood | FLOOD_longop: CPU max | 87 % | 100 % | 13 % | +15 % | **гірше** |
| uart_flood | FLOOD_longop: UART RX відкинуто | 469 Б | 424 Б | -45 Б | -10 % | **краще** |
| uart_flood | FLOOD_longop: журнал пропусків >25 мс | 0 | 0 | 0 | — | без змін |
| uart_flood | FLOOD_longop: журнал макс. інтервал | 0 мс | 0 мс | 0 мс | — | без змін |
| uart_flood | FLOOD_longop: OLED кадр/с | 3.8 | 5.2 | 1.4 | +37 % | **краще** |
| uart_flood | FLOOD_longop: перший відлік IMU після скиду | 208.8 мс | 218.1 мс | 9.23 мс | +4 % | без змін |
| uart_flood | FLOOD_longop: стек MSP виміряно | 724 Б | 592 Б | -132 Б | -18 % | довідково |
| uart_flood | довга операція dump max | 298.0 мс | 3044.8 мс | 2746.8 мс | +922 % | **гірше** |

Підсумок: краще 51, гірше 24, без змін 26, довідково 88.

## Метрики ОС

### heapsmall.log

    ==== LR4 FreeRTOS | fw=cf43d3e3-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    FATAL os_task_new cmd
    FATAL os_task_new oled
    FATAL os_task_new long_op
    FATAL assert Middlewares/FreeRTOS/Source/tasks.c:3753 task=-

### nocheck.log

    ==== LR4 FreeRTOS | fw=73e646c3-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    OVF start stack_words=64 need_bytes=~400 pad=OK
    OVF survived (check disabled?)
    OVF end (no hook)

### os_tests.log

| Задача | prio (base) | стан | CPU, % | стек, Б | мін. вільно, Б | використано, Б |
|---|---|---|---:|---:|---:|---:|
| btn | 6 (6) | B | 0.0 | 416 | 292 | 124 |
| imu | 5 (5) | B | 47.1 | 560 | 216 | 344 |
| journal | 4 (4) | B | 0.5 | 448 | 268 | 180 |
| cmd | 3 (3) | R | 8.0 | 880 | 468 | 412 |
| telem | 2 (2) | B | 0.1 | 480 | 328 | 152 |
| oled | 2 (2) | r | 35.0 | 672 | 204 | 468 |
| long_op | 1 (1) | B | 0.0 | 624 | 340 | 284 |
| starve | 6 (6) | D | 8.6 | 256 | 148 | 108 |
| IDLE | 0 (0) | r | 0.0 | 256 | 164 | 92 |

    HEAP total=6912 free=960 min_ever_free=0 used_max=6912 malloc_fail=2

    OSTICK n=3469 min_ns=16000 avg_ns=25395 max_ns=73000

    CTXSW n=2523 per_s=841

    MSP stack_used=624 stack_size=1024

    ==== LR4 FreeRTOS | fw=9066d964-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====
    CTXSW_COST n=200 min_cyc=3840 avg_cyc=4026 max_cyc=7284 avg_ns=83875
    INV kind=mutex h_wait_us=51 l_hold_us=0 m_busy_us=300000 heap_ok=1
    INV kind=binsem h_wait_us=49 l_hold_us=0 m_busy_us=300000 heap_ok=1
    STARVE start ms=300
    STARVE end
    STARVE;
    QFLOOD n=50 len=8 consumer_ms=10 nb_sent=19 nb_dropped=31 nb_us=151037 blk_sent=50 blk_max_wait_us=20093 consumed=60
    HEAPX blocks=7 block=128 free_before=960 free_after=0 hook_calls=2 queue_create=FAIL
    OVF start stack_words=64 need_bytes=~400 pad=OK
    FATAL stack overflow task=ovf

### s3_os.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====

### s3_tick100.log

    ==== LR4 FreeRTOS | fw=5b701a3e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=100 | reset=0x00 ====

### vcd_trace.log

    ==== LR4 FreeRTOS | fw=18a8168e-nogit | NUCLEO-C031C6 | SYSCLK=48000000 | tick=1000 | reset=0x00 ====

