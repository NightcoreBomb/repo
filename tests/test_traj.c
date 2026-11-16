/**
 * Тести App/traj.c: формат журналу на SD (FR-07..FR-09) — CRC, Δ-кодування,
 * насичення, каталог, виявлення пошкоджень.
 */
#include <string.h>
#include "unity.h"
#include "traj.h"

static uint8_t blk[TRAJ_BLOCK];

void setUp(void) { memset(blk, 0, sizeof blk); }
void tearDown(void) { }

/* CRC-16/CCITT-FALSE, check value для "123456789" — 0x29B1 (каталог Williams/RevEng) */
static void test_crc16_check_value(void)
{
  TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt((const uint8_t *)"123456789", 9, 0xFFFF));
}

static void test_roundtrip_full_block(void)
{
  traj_enc_t e;
  traj_enc_begin(&e, blk, 7, 12340);
  int16_t p = 900, t = 900;
  bool full = false;
  for (unsigned i = 0; i < TRAJ_PER_BLOCK; i++) {
    full = traj_enc_add(&e, p, t);
    p = (int16_t)(p + (i % 7) - 3);
    t = (int16_t)(t - (i % 5) + 2);
  }
  TEST_ASSERT_TRUE(full);
  TEST_ASSERT_EQUAL_UINT16(TRAJ_PER_BLOCK, traj_enc_finish(&e));
  TEST_ASSERT_EQUAL_INT(TRAJ_PER_BLOCK, traj_block_check(blk, 7));
  int16_t q = 900, u = 900, gp = 0, gt = 0;
  for (unsigned i = 0; i < TRAJ_PER_BLOCK; i++) {
    traj_block_step(blk, (uint16_t)i, &gp, &gt);
    TEST_ASSERT_EQUAL_INT16(q, gp);
    TEST_ASSERT_EQUAL_INT16(u, gt);
    q = (int16_t)(q + (i % 7) - 3);
    u = (int16_t)(u - (i % 5) + 2);
  }
  int16_t rp, rt;
  traj_block_get(blk, 100, &rp, &rt);
  traj_block_step(blk, 0, &gp, &gt);
  for (uint16_t i = 1; i <= 100; i++) traj_block_step(blk, i, &gp, &gt);
  TEST_ASSERT_EQUAL_INT16(gp, rp);
}

/* Стрибок більший за ±12.7° насичується, але похибка не накопичується */
static void test_saturation_tracks_reconstructed_value(void)
{
  traj_enc_t e;
  traj_enc_begin(&e, blk, 0, 0);
  traj_enc_add(&e, 0, 1800);
  traj_enc_add(&e, 1000, 0);                              /* +100° / −180° за 20 мс */
  for (int i = 0; i < 20; i++) traj_enc_add(&e, 1000, 0);
  traj_enc_finish(&e);
  int16_t p = 0, t = 0;
  traj_block_step(blk, 0, &p, &t);
  traj_block_step(blk, 1, &p, &t);
  TEST_ASSERT_EQUAL_INT16(127, p);
  TEST_ASSERT_EQUAL_INT16(1800 - 127, t);
  for (uint16_t i = 2; i < 22; i++) traj_block_step(blk, i, &p, &t);
  TEST_ASSERT_EQUAL_INT16(1000, p);                       /* наздогнав ціль */
  TEST_ASSERT_EQUAL_INT16(0, t);
}

static void test_corruption_and_wrong_seq_detected(void)
{
  traj_enc_t e;
  traj_enc_begin(&e, blk, 3, 0);
  for (int i = 0; i < 10; i++) traj_enc_add(&e, (int16_t)(i * 10), 900);
  traj_enc_finish(&e);
  TEST_ASSERT_EQUAL_INT(10, traj_block_check(blk, 3));
  TEST_ASSERT_EQUAL_INT(-1, traj_block_check(blk, 4));     /* не той блок */
  blk[300] ^= 0x01;                                        /* навіть за межами n: CRC по всьому блоку */
  TEST_ASSERT_EQUAL_INT(-1, traj_block_check(blk, 3));
  blk[300] ^= 0x01;
  blk[0] = 0;
  TEST_ASSERT_EQUAL_INT(-1, traj_block_check(blk, 3));
}

static void test_dir_roundtrip_and_crc(void)
{
  traj_dir_t d = { 13, 3000, 60000, 20 }, r;
  traj_dir_write(blk, &d);
  TEST_ASSERT_TRUE(traj_dir_read(blk, &r));
  TEST_ASSERT_EQUAL_UINT16(13, r.blocks);
  TEST_ASSERT_EQUAL_UINT32(3000, r.records);
  TEST_ASSERT_EQUAL_UINT32(60000, r.duration_ms);
  TEST_ASSERT_EQUAL_UINT16(20, r.period_ms);
  blk[5] ^= 0x80;
  TEST_ASSERT_FALSE(traj_dir_read(blk, &r));
  memset(blk, 0, sizeof blk);
  TEST_ASSERT_FALSE(traj_dir_read(blk, &r));               /* порожня карта */
}

/* FR-07: 60 с @50 Гц = 3000 записів = 13 блоків */
static void test_capacity_60s(void)
{
  TEST_ASSERT_EQUAL_UINT(249, TRAJ_PER_BLOCK);
  TEST_ASSERT_EQUAL_UINT(13, (3000 + TRAJ_PER_BLOCK - 1) / TRAJ_PER_BLOCK);
  TEST_ASSERT_TRUE(TRAJ_MAX_BLOCKS * TRAJ_PER_BLOCK >= 3000);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_crc16_check_value);
  RUN_TEST(test_roundtrip_full_block);
  RUN_TEST(test_saturation_tracks_reconstructed_value);
  RUN_TEST(test_corruption_and_wrong_seq_detected);
  RUN_TEST(test_dir_roundtrip_and_crc);
  RUN_TEST(test_capacity_60s);
  return UNITY_END();
}
