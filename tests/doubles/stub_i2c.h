/**
 * stub_i2c — STUB: нічого не моделює і нічого не перевіряє, лише повертає
 * заданий код помилки на будь-який виклик. Потрібен, щоб перевірити обробку
 * помилок шини (NACK, timeout) без побудови сценарію.
 */
#ifndef STUB_I2C_H
#define STUB_I2C_H
#include "bus_i2c.h"
void stub_i2c_set(int ret);
extern const bus_i2c_t stub_i2c_bus;
#endif
