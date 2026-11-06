#include "pot.h"

uint32_t pot_raw_to_mv(uint32_t raw)
{
  if (raw > POT_ADC_MAX) raw = POT_ADC_MAX;
  return (raw * POT_VREF_MV + POT_ADC_MAX / 2) / POT_ADC_MAX;
}

int32_t pot_raw_to_deadzone_cdeg(uint32_t raw)
{
  if (raw > POT_ADC_MAX) raw = POT_ADC_MAX;
  return (int32_t)((raw * (uint32_t)POT_DZ_MAX_CDEG + POT_ADC_MAX / 2) / POT_ADC_MAX);
}
