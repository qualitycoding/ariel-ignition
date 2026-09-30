/* cli.h — serial tuning command interpreter. Interface frozen by D-014.
 * Grammar and ranges: plan/DECISIONS.md D-014. */
#ifndef ARIEL_CLI_H
#define ARIEL_CLI_H
#include "common.h"
#include "config.h"

#define CLI_MAX_LINE 63u   /* characters, excluding the terminating NUL */

typedef enum {
    CLI_OK = 0,
    CLI_ERR_SYNTAX,
    CLI_ERR_RANGE,
    CLI_ERR_BUSY,           /* engine turning: command not allowed         */
    CLI_ERR_UNKNOWN,
    CLI_ERR_TOOLONG,
    CLI_ERR_INVALID_CONFIG  /* save refused: config_validate() failed       */
} cli_status_t;

/* Execute one command line (no trailing newline required; a trailing
 * "\r" and/or "\n" is ignored). `working` is modified only on CLI_OK.
 * `rpm` is the current engine speed (0 = stopped). `out` receives a
 * NUL-terminated reply (never overflowing out_len). *save_requested is
 * set true only by a successful "save". */
cli_status_t cli_exec(const char *line, config_t *working, uint16_t rpm,
                      char *out, size_t out_len, bool *save_requested);
#endif
