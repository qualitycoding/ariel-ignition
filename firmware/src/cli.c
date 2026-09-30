/* STUB — implemented by plan step S-014. */
#include "cli.h"
cli_status_t cli_exec(const char *line, config_t *w, uint16_t rpm, char *out, size_t n, bool *save)
{ (void)line; (void)w; (void)rpm; if (out && n) out[0] = 0; if (save) *save = false; NOT_IMPLEMENTED(); return CLI_ERR_UNKNOWN; }
