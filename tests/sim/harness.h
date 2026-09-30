/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* harness.h — simavr-based firmware-in-the-loop harness (D-020).
 * Runs the REAL firmware ELF on a simulated ATmega328P at 8 MHz and drives
 * the pins defined in D-015. Time base: microseconds since reset. */
#ifndef ARIEL_SIM_HARNESS_H
#define ARIEL_SIM_HARNESS_H
#include <stdint.h>
#include <stdbool.h>

enum { OUT_LOW = 0, OUT_HIGH = 1, OUT_HIZ = 2 };        /* PB1 state        */
enum { EV_ON = 1, EV_FIRE = 2, EV_SOFT = 3 };          /* PB1 transitions  */
/* EV_ON  : PB1 becomes driven HIGH (coil charging / breaker closed)
 * EV_FIRE: PB1 driven HIGH -> driven LOW (spark)
 * EV_SOFT: PB1 driven HIGH -> high impedance (soft shutdown, TCI only) */

typedef struct { double t_us; int kind; } sim_event_t;

typedef struct sim sim_t;

sim_t *sim_new(const char *elf_path);          /* PB0 high, PD2 high, PD3 high, vbat 6300 mV */
void   sim_free(sim_t *s);
/* Schedule input changes (any order; applied when simulated time reaches t). */
void   sim_trigger_at(sim_t *s, double t_us, int level);   /* PB0 (ICP1) */
void   sim_kill_at(sim_t *s, double t_us, bool active);    /* PD2 low = active */
void   sim_map1_at(sim_t *s, double t_us, bool grounded);  /* PD3 low = map 1  */
void   sim_vbat_at(sim_t *s, double t_us, uint16_t mv);    /* via 68k/4.7k divider */
void   sim_trigger_initial(sim_t *s, int level);           /* PB0 level at reset   */
/* Run until simulated time t_us. Returns false if the CPU crashed. */
bool   sim_run_until(sim_t *s, double t_us);
int    sim_events(const sim_t *s, const sim_event_t **ev); /* recorded transitions */
int    sim_out_state(const sim_t *s);

/* Engine helper: n four-stroke cycles at constant crank rpm, first TDC at
 * tdc0_us, trigger driven at half engine speed (one lead + one trail edge per
 * 720 deg): lead edge (PB0 -> 0) at lead_cdeg BTDC, trail edge (PB0 -> 1) at
 * trail_cdeg BTDC. Returns the time of the TDC following the last cycle. */
double sim_cycles(sim_t *s, double tdc0_us, double rpm, int n,
                  int lead_cdeg, int trail_cdeg);
/* Time (us) that crank angle `cdeg` occupies at `rpm`. */
double sim_angle_us(double rpm, int cdeg);

/* Query helpers over recorded events in [t0, t1). */
int    sim_count(const sim_t *s, int kind, double t0, double t1);
/* First event of `kind` at or after t0 (and before t1); returns -1.0 if none. */
double sim_first(const sim_t *s, int kind, double t0, double t1);
#endif
