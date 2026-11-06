/**
 * mock_i2c — MOCK шини I²C: тест заздалегідь задає ОЧІКУВАНУ послідовність
 * транзакцій (адреса, байти запису, що повернути), а mock перевіряє, що драйвер
 * виконав саме її, у тому ж порядку. Порушення порядку = провал тесту, навіть
 * якщо підсумковий результат функції правильний.
 */
#ifndef MOCK_I2C_H
#define MOCK_I2C_H
#include "bus_i2c.h"

typedef enum { MI_WRITE, MI_READ, MI_WRITE_READ, MI_PROBE } mi_op_t;

void mock_i2c_reset(void);
/* очікування: op, addr, байти запису (w,nw), байти відповіді (r,nr), код повернення */
void mock_i2c_expect(mi_op_t op, uint8_t addr, const uint8_t *w, size_t nw,
                     const uint8_t *r, size_t nr, int ret);
/* NULL — якщо всі очікування виконані і зайвих викликів не було, інакше текст помилки */
const char *mock_i2c_verify(void);
extern const bus_i2c_t mock_i2c_bus;

#endif
