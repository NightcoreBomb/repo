#include "stub_i2c.h"
static int ret_code = ST_ENACK;
void stub_i2c_set(int r) { ret_code = r; }
static int s_w(void *c, uint8_t a, const uint8_t *d, size_t n) { (void)c; (void)a; (void)d; (void)n; return ret_code; }
static int s_r(void *c, uint8_t a, uint8_t *d, size_t n) { (void)c; (void)a; (void)d; (void)n; return ret_code; }
static int s_wr(void *c, uint8_t a, const uint8_t *w, size_t nw, uint8_t *r, size_t nr)
{ (void)c; (void)a; (void)w; (void)nw; (void)r; (void)nr; return ret_code; }
static int s_p(void *c, uint8_t a) { (void)c; (void)a; return ret_code; }
const bus_i2c_t stub_i2c_bus = { NULL, s_w, s_r, s_wr, s_p };
