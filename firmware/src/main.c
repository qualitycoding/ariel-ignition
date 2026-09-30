/* Main loop: serial tuning port and EEPROM. All spark timing is in runtime.c. */
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "hal.h"
#include "config.h"
#include "cli.h"
#include "runtime.h"

#define RX_SIZE 64u
static volatile uint8_t rx_buf[RX_SIZE];
static volatile uint8_t rx_head, rx_tail;

ISR(USART_RX_vect)
{
    uint8_t c = UDR0;
    uint8_t n = (uint8_t)((rx_head + 1u) % RX_SIZE);
    if (n != rx_tail) { rx_buf[rx_head] = c; rx_head = n; }
}

static void tx_c(char c) { while (!(UCSR0A & _BV(UDRE0))) { } UDR0 = (uint8_t)c; }
static void tx_s(const char *s) { while (*s) tx_c(*s++); }

static config_t working;                                 /* CLI copy; the active copy is in runtime.c */

static bool do_save(void)
{
    config_t rb;
    if (runtime_rpm() != 0) return false;               /* engine started meanwhile */
    eeprom_update_block(&working, (void *)0, sizeof working);
    eeprom_read_block(&rb, (const void *)0, sizeof rb);
    if (config_check_image(&rb) != CFG_OK || memcmp(&rb, &working, sizeof rb) != 0) return false;
    runtime_apply_config(&working);
    return true;
}

static void process_line(const char *line, bool overlong)
{
    char out[128];
    bool save = false;
    if (overlong) {
        strcpy(out, "ERR 5");
    } else if (strcmp(line, "status") == 0) {
        runtime_status(out, sizeof out);
    } else {
        (void)cli_exec(line, &working, runtime_rpm(), out, sizeof out, &save);
        if (save && !do_save()) strcpy(out, "ERR 7");   /* EEPROM write or verify failed */
    }
    tx_s(out); tx_s("\r\n");
}

int main(void)
{
    uint8_t mcusr = MCUSR;
    config_t image;
    bool ok;
    char line[CLI_MAX_LINE + 2];
    uint8_t len = 0;
    bool over = false;

    MCUSR = 0;
    wdt_disable();
    hal_init();
    eeprom_read_block(&image, (const void *)0, sizeof image);
    ok = config_load_or_default(&working, &image);
    runtime_init(&working, !ok, mcusr);
    sei();

    for (;;) {
        while (rx_tail != rx_head) {
            uint8_t c = rx_buf[rx_tail];
            rx_tail = (uint8_t)((rx_tail + 1u) % RX_SIZE);
            if (c == '\r' || c == '\n') {
                if (len || over) { line[len] = 0; process_line(line, over); }
                len = 0; over = false;
            } else if (len < CLI_MAX_LINE + 1u) {
                line[len++] = (char)c;
                if (len > CLI_MAX_LINE) over = true;
            }
        }
    }
}
