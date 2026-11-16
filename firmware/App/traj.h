/**
 * traj.h — формат журналу траєкторії на microSD (FR-07..FR-09). Чиста логіка.
 *
 * Сира розкладка блоків (без файлової системи — RAM 12 КБ не вистачить на FatFs):
 *   LBA TRAJ_DIR_LBA        — блок-каталог: скільки блоків і записів, тривалість, CRC
 *   LBA TRAJ_DIR_LBA+1 ...  — блоки даних
 * Блок даних (512 Б): заголовок 16 Б + Δ-кодовані пари (pan, tilt):
 *   0 magic 'TJ' | 2 seq | 4 n | 6 pan0 | 8 tilt0 | 10 t0_ms (u32) | 14 crc16
 *   16.. n−1 пар int8 (Δpan, Δtilt) у 0.1° — до 248 пар → 249 записів/блок
 * Кут — десяті градуса 0..1800 (роздільність серво 0.09°/мкс). Δ насичується до ±127
 * (12.7° за 20 мс = 635 °/с — більше, ніж дозволяє обмеження швидкості 360 °/с);
 * кодер стежить за відновленим значенням, тож похибка не накопичується.
 * CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) — по всьому блоку, крім поля crc.
 */
#ifndef APP_TRAJ_H
#define APP_TRAJ_H

#include <stdbool.h>
#include <stdint.h>

#define TRAJ_BLOCK          512u
#define TRAJ_HDR            16u
#define TRAJ_PER_BLOCK      (1u + (TRAJ_BLOCK - TRAJ_HDR) / 2u)   /* 249 */
#define TRAJ_MAGIC_DATA     0x4A54u   /* 'TJ' */
#define TRAJ_MAGIC_DIR      0x5244u   /* 'DR' */
#define TRAJ_PERIOD_MS      20u       /* 50 Гц (FR-07) */
#define TRAJ_DIR_LBA        2048u     /* 1 МБ від початку карти */
#define TRAJ_MAX_BLOCKS     256u      /* ≈ 21 хв запису */

typedef struct {
  uint8_t *buf;                       /* 512 Б, належить викликачу */
  uint16_t seq, n;
  int16_t  last_pan, last_tilt;       /* відновлені (декодером) значення */
} traj_enc_t;

typedef struct {
  uint16_t blocks;
  uint32_t records;
  uint32_t duration_ms;
  uint16_t period_ms;
} traj_dir_t;

uint16_t crc16_ccitt(const uint8_t *p, uint32_t n, uint16_t crc);

/* Кодер: почати блок seq; додати запис; true — блок заповнено (треба записати) */
void traj_enc_begin(traj_enc_t *e, uint8_t *buf, uint16_t seq, uint32_t t0_ms);
bool traj_enc_add(traj_enc_t *e, int16_t pan_dd, int16_t tilt_dd);
/* Завершити блок: записати n і CRC. Повертає кількість записів у блоці */
uint16_t traj_enc_finish(traj_enc_t *e);

/* Перевірка блоку (magic + CRC); повертає n або −1 */
int  traj_block_check(const uint8_t *buf, uint16_t expect_seq);
/* i-й запис блоку (0..n−1), O(i) — для перевірок і тестів */
void traj_block_get(const uint8_t *buf, uint16_t i, int16_t *pan_dd, int16_t *tilt_dd);
/* Послідовне відтворення, O(1): i = 0 — ключовий кадр, i ≥ 1 — додати Δ до pan/tilt */
void traj_block_step(const uint8_t *buf, uint16_t i, int16_t *pan_dd, int16_t *tilt_dd);

void traj_dir_write(uint8_t *buf, const traj_dir_t *d);
bool traj_dir_read(const uint8_t *buf, traj_dir_t *d);

#endif
