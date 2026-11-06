/**
 * pot.h — потенціометр на АЦП: сирий код → мВ і → ширина dead-zone (FR-04).
 */
#ifndef DEV_POT_H
#define DEV_POT_H

#include <stdint.h>

#define POT_ADC_MAX     4095u    /* 12 біт (RM0490, ADC) */
#define POT_VREF_MV     3300u    /* VDDA на Nucleo = 3V3 */
#define POT_DZ_MAX_CDEG 1000     /* 0..10° (FR-04)        */

uint32_t pot_raw_to_mv(uint32_t raw);
int32_t  pot_raw_to_deadzone_cdeg(uint32_t raw);

#endif
