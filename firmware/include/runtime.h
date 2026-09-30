/* runtime.h — ignition runtime state machine (D-016, D-018, D-019).
 * All spark timing is done in interrupts: Timer1 input capture (trigger edges),
 * OC1A (hardware spark edge), OC1B (software event timer), Timer0 (1 ms tick).
 * The main loop only handles the serial port and EEPROM. */
#ifndef ARIEL_RUNTIME_H
#define ARIEL_RUNTIME_H
#include "common.h"
#include "config.h"

/* `mcusr` = reset flags captured at boot (for the unexpected-reset counter). */
void     runtime_init(const config_t *c, bool defaults_loaded, uint8_t mcusr);
void     runtime_apply_config(const config_t *c);   /* atomically replace the active config */
uint16_t runtime_rpm(void);                          /* 0 = engine stopped                  */
/* "rpm=<n> mode=<STOP|CRANK|RUN|LIMP|KILL|HOLD> adv=<cdeg> vbat=<mV> map=<m> resets=<n>" */
void     runtime_status(char *out, size_t n);
#endif
