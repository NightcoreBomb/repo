/**
 * status.h — єдині коди повернення для драйверів і прикладного шару.
 * Шар Interfaces: лише стандартні заголовки, жодного HAL.
 */
#ifndef IF_STATUS_H
#define IF_STATUS_H

typedef enum {
  ST_OK       =  0,
  ST_ENACK    = -1,  /* пристрій не підтвердив адресу/байт (I2C NACK)          */
  ST_ETIMEOUT = -2,  /* шина або пристрій не відповіли вчасно                   */
  ST_EBUSY    = -3,  /* шина зайнята                                            */
  ST_EIO      = -4,  /* інша помилка шини (arbitration lost, bus error, ...)   */
  ST_EID      = -5,  /* ідентифікатор чипа не той, що очікувався                */
  ST_EPARAM   = -6,  /* неприпустимий аргумент                                  */
  ST_ENORESP  = -7,  /* пристрій не дав очікуваної відповіді протоколу          */
  ST_EVERIFY  = -8,  /* прочитане не збігається з записаним / неправдоподібне   */
} status_t;

static inline const char *st_str(int s)
{
  switch (s) {
    case ST_OK:       return "OK";
    case ST_ENACK:    return "NACK";
    case ST_ETIMEOUT: return "TIMEOUT";
    case ST_EBUSY:    return "BUSY";
    case ST_EIO:      return "EIO";
    case ST_EID:      return "BAD_ID";
    case ST_EPARAM:   return "EPARAM";
    case ST_ENORESP:  return "NORESP";
    case ST_EVERIFY:  return "VERIFY";
    default:          return "?";
  }
}

#endif
