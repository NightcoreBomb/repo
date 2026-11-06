# Записи логічного аналізатора (Форма B: `wokwi-logic-analyzer` → VCD)

Формат — VCD (Wokwi), відкривається в PulseView (`File → Import → Value Change Dump`;
для великих файлів у діалозі імпорту встановіть *Downsample* = 20 → 50 МГц).
Поруч із кожним `.vcd` лежить `.txt` — текстове декодування (`tools/decode_capture.py`:
sigrok-cli + незалежний декодер `vcdtool.py` з вимірами таймінгів).

## Іменування

`<інтерфейс>_<пристрій>_<операція>.vcd`, наприклад `i2c_mpu6050_burst.vcd`,
`spi_sd_init-read0.vcd`, `pwm_servo_1000-2000us.vcd`, `adc_pot_mark-5ms.vcd`,
`i2c_nack_addr69.vcd`, `uart_vcp_pattern.vcd`. Запис ЛР1 — `lr1_bringup_all.vcd` (усе підряд від старту).

## Параметри запису (однакові для всіх файлів, якщо не сказано інше)

| Параметр | Значення | Пояснення |
|---|---|---|
| Частота дискретизації | 1 ГГц за форматом (timescale 1 ns); фактичний крок міток у записах Wokwi — 25 нс | VCD зберігає лише зміни рівнів, тож «глибина» = кількість фронтів, а не час × частота |
| Буфер | `bufferSize = 2 000 000` фронтів ≈ 18 МБ RAM симулятора | скан I²C + 1 с UART ≈ 10⁴ фронтів; запас для `cap all` |
| Тригер | за замовчуванням вимкнено (`probe.py bus`); для «чистих» записів — `make probe P=bus GATE=1`: `triggerMode=level`, `triggerPin=D7` (MARK), запис лише поки MARK=1 | MARK піднімається прошивкою рівно на час цікавої транзакції (`App/capture.c`) |
| GND | `logic:GND` ↔ `nucleo:GND.7` | у Wokwi рівні логічні, але з'єднання лишаємо — як на залізі (контрольне питання про спільну землю) |

## Розкладка каналів (профілі `tools/probe.py`)

| Профіль | D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7 |
|---|---|---|---|---|---|---|---|---|
| `bus` | SCL PB8 | SDA PB9 | SCK PA1 | MOSI PA7 | MISO PA6 | CS PB0 | TX PA2 | MARK PB3 |
| `pwm` | PWM_PAN PB4 | PWM_TILT PB5 | MARK PB3 | BTN PA8 | TX PA2 | LED PA5 | SCL PB8 | SDA PB9 |

У VS Code Wokwi зберігає запис лише одного аналізатора у `wokwi.vcd`, а 8 каналів не вистачає
на всі інтерфейси (журнал проблем, П-5) — тому щупи переставляються між записами, як на реальному стенді.

## Як отримати кожен запис (відтворюваність)

Кожну транзакцію ініціює команда консолі; у логу вона обрамлена рядками `CAP <name> begin/end`,
а на каналі MARK — імпульсом.

| Файл | Профіль | Команда в Serial Monitor | Що прошивка робить у вікні MARK |
|---|---|---|---|
| `i2c_mpu6050_burst.vcd` | `bus` | `cap i2c` | probe 0x68, probe 0x3C, WHO_AM_I, burst 14 байтів з 0x3B; байти і перетворені значення — в UART |
| `i2c_nack_addr69.vcd` | `bus` | `cap nack` | WHO_AM_I на 0x69 (AD0=1, пристрою немає) — свідома аномалія |
| `i2c_mpu6050_restart.vcd` | `bus` | `cap rs` | ті самі читання з repeated START (`HAL_I2C_Mem_Read`) |
| `spi_sd_init-read0.vcd` | `bus` | `cap spi` | CMD0, CMD8, (CMD55+ACMD41)×N, CMD58, CMD17 блок 0 |
| `spi_sd_write.vcd` | `bus` | `cap spiw` | CMD24 + токен 0xFE + 512 байтів + data response (запис з «попередньою командою») |
| `uart_vcp_pattern.vcd` | `bus` | `cap uart` | рядок з 0x55 'U' (чергування біт — зручно міряти тривалість біта) |
| `pwm_servo_1000-2000us.vcd` | `pwm` | `cap pwm` | pan 1000 мкс (100 мс), потім 2000 мкс (100 мс), tilt 1250 мкс |
| `adc_pot_mark-5ms.vcd` | `pwm` | `cap adc` | 20 перетворень з кроком 5 мс; MARK=1 рівно на час перетворення |

Послідовність:

```bash
make probe P=bus            # або P=pwm; GATE=1 — запис лише у вікнах MARK
make build                  # (diagram.json змінився — перезапустити симулятор)
# VS Code: F1 → Wokwi: Start Simulator; дочекатися "bring-up done"; ввести команду; зупинити симуляцію
make capture NAME=i2c_mpu6050_burst     # wokwi.vcd → docs/captures/i2c_mpu6050_burst.{vcd,txt}
make vectors                # байти з запису → tests/vectors/capture_vectors.h
make test                   # тест test_capture_vectors_burst_match_uart_log перестане бути IGNORED
```

Сесію PulseView з декодерами збережіть як `.sr` поруч (`File → Save As`), скриншоти — у `docs/lab_2/img/`.
