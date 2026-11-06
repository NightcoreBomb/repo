/**
 * spi_log — записувач SPI-транзакцій (основа mock-перевірок послідовності) +
 * підключення байтової моделі пристрою (fake_sd, fake_w25q).
 * Кожна подія: CS↓, CS↑, байт (MOSI, MISO), зміна частоти.
 */
#ifndef SPI_LOG_H
#define SPI_LOG_H
#include <stdbool.h>
#include "bus_spi.h"

typedef enum { EV_CS_LOW, EV_CS_HIGH, EV_BYTE, EV_FAST, EV_SLOW } ev_t;
typedef struct { ev_t ev; uint8_t mosi, miso; } spi_event_t;

typedef struct {
  void    (*cs)(bool low);
  uint8_t (*byte)(uint8_t mosi);   /* викликається лише при CS = low */
} spi_device_t;

#define SPI_LOG_MAX 20000
extern spi_event_t spi_log[SPI_LOG_MAX];
extern int spi_log_n;
extern bool spi_cs_low;
extern bool spi_fast;

void spi_log_reset(const spi_device_t *dev);
extern const bus_spi_t spi_log_bus;

/* Витягнути k-й кадр (байти MOSI між CS↓ і CS↑), повертає довжину або -1 */
int spi_frame(int k, uint8_t *mosi, uint8_t *miso, int max);
int spi_frame_count(void);
/* Пошук кадру (від from), перший не-0xFF байт якого = first */
int spi_find_frame(uint8_t first, int from);

#endif
