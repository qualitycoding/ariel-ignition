/* STUB — implemented by plan step S-012. */
#include <string.h>
#include "config.h"
void config_defaults(config_t *c) { memset(c, 0, sizeof *c); NOT_IMPLEMENTED(); }
cfg_err_t config_validate(const config_t *c) { (void)c; NOT_IMPLEMENTED(); return CFG_ERR_RESERVED; }
void config_seal(config_t *c) { (void)c; NOT_IMPLEMENTED(); }
cfg_err_t config_check_image(const config_t *c) { (void)c; NOT_IMPLEMENTED(); return CFG_ERR_RESERVED; }
bool config_load_or_default(config_t *dst, const config_t *image) { (void)image; memset(dst, 0, sizeof *dst); NOT_IMPLEMENTED(); return false; }
