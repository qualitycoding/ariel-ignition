/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* harness.c — see harness.h. Verified mechanics: research/spikes/simavr-icp,
 * research/spikes/simavr-adc (C-030, C-031). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_avr.h"
#include "sim_elf.h"
#include "avr_ioport.h"
#include "avr_adc.h"
#include "harness.h"

#define F_SIM      8000000.0
#define DDRB_ADDR  0x24
#define MAX_IN     20000
#define MAX_EV     20000
enum { CH_PB0, CH_PD2, CH_PD3, CH_VBAT };
typedef struct { double t; int ch; int v; } in_t;

struct sim {
    avr_t *avr;
    elf_firmware_t fw;
    avr_irq_t *pb0, *pd2, *pd3, *adc6;
    int pb1_level, state;
    in_t in[MAX_IN]; int nin, iin; bool sorted;
    sim_event_t ev[MAX_EV]; int nev;
};

static double now_us(const sim_t *s) { return (double)s->avr->cycle * 1e6 / F_SIM; }

static void pb1_cb(struct avr_irq_t *irq, uint32_t v, void *p) {
    (void)irq; ((sim_t *)p)->pb1_level = v ? 1 : 0;
}

static void push_in(sim_t *s, double t, int ch, int v) {
    if (s->nin >= MAX_IN) { fprintf(stderr, "harness: input queue full\n"); abort(); }
    s->in[s->nin++] = (in_t){ t, ch, v }; s->sorted = false;
}
static int cmp_in(const void *a, const void *b) {
    const in_t *x = a, *y = b; return (x->t > y->t) - (x->t < y->t);
}
static uint32_t vbat_to_pin_mv(uint16_t mv) { return (uint32_t)((double)mv * 4700.0 / 72700.0 + 0.5); }

sim_t *sim_new(const char *elf_path) {
    sim_t *s = calloc(1, sizeof *s);
    if (!s || elf_read_firmware(elf_path, &s->fw) != 0) { fprintf(stderr, "harness: cannot load %s\n", elf_path); exit(2); }
    s->avr = avr_make_mcu_by_name("atmega328p");
    avr_init(s->avr);
    avr_load_firmware(s->avr, &s->fw);
    s->avr->frequency = (uint32_t)F_SIM;
    s->avr->avcc = 5000; s->avr->aref = 5000;
    s->pb0  = avr_io_getirq(s->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 0);
    s->pd2  = avr_io_getirq(s->avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 2);
    s->pd3  = avr_io_getirq(s->avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 3);
    s->adc6 = avr_io_getirq(s->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC6);
    avr_irq_register_notify(avr_io_getirq(s->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 1), pb1_cb, s);
    avr_raise_irq(s->pb0, 1); avr_raise_irq(s->pd2, 1); avr_raise_irq(s->pd3, 1);
    avr_raise_irq(s->adc6, vbat_to_pin_mv(6300));
    s->state = OUT_HIZ; s->sorted = true;
    return s;
}
void sim_free(sim_t *s) { if (s) { avr_terminate(s->avr); free(s); } }
void sim_trigger_initial(sim_t *s, int level) { avr_raise_irq(s->pb0, level ? 1 : 0); }
void sim_trigger_at(sim_t *s, double t, int level) { push_in(s, t, CH_PB0, level ? 1 : 0); }
void sim_kill_at(sim_t *s, double t, bool a)      { push_in(s, t, CH_PD2, a ? 0 : 1); }
void sim_map1_at(sim_t *s, double t, bool g)      { push_in(s, t, CH_PD3, g ? 0 : 1); }
void sim_vbat_at(sim_t *s, double t, uint16_t mv) { push_in(s, t, CH_VBAT, (int)mv); }

static void apply(sim_t *s, const in_t *e) {
    switch (e->ch) {
    case CH_PB0: avr_raise_irq(s->pb0, (uint32_t)e->v); break;
    case CH_PD2: avr_raise_irq(s->pd2, (uint32_t)e->v); break;
    case CH_PD3: avr_raise_irq(s->pd3, (uint32_t)e->v); break;
    default:     avr_raise_irq(s->adc6, vbat_to_pin_mv((uint16_t)e->v)); break;
    }
}
static void sample(sim_t *s) {
    int st = (s->avr->data[DDRB_ADDR] & 0x02) ? (s->pb1_level ? OUT_HIGH : OUT_LOW) : OUT_HIZ;
    if (st == s->state) return;
    int kind = 0;
    if (st == OUT_HIGH) kind = EV_ON;
    else if (s->state == OUT_HIGH) kind = (st == OUT_LOW) ? EV_FIRE : EV_SOFT;
    s->state = st;
    if (kind && s->nev < MAX_EV) s->ev[s->nev++] = (sim_event_t){ now_us(s), kind };
}
bool sim_run_until(sim_t *s, double t_us) {
    if (!s->sorted) {           /* keep already-applied prefix; sort the rest */
        qsort(s->in + s->iin, (size_t)(s->nin - s->iin), sizeof s->in[0], cmp_in);
        s->sorted = true;
    }
    while (now_us(s) < t_us) {
        while (s->iin < s->nin && s->in[s->iin].t <= now_us(s)) apply(s, &s->in[s->iin++]);
        int r = avr_run(s->avr);
        sample(s);
        if (r == cpu_Crashed) return false;
        if (r == cpu_Done) {    /* firmware stopped: let time pass so tests see no events */
            s->avr->cycle += 8;
        }
    }
    return true;
}
int sim_events(const sim_t *s, const sim_event_t **ev) { *ev = s->ev; return s->nev; }
int sim_out_state(const sim_t *s) { return s->state; }

double sim_angle_us(double rpm, int cdeg) { return (double)cdeg / 100.0 / 360.0 * 60e6 / rpm; }
double sim_cycles(sim_t *s, double tdc, double rpm, int n, int lead, int trail) {
    double cyc = 120e6 / rpm;
    for (int k = 0; k < n; k++, tdc += cyc) {
        sim_trigger_at(s, tdc - sim_angle_us(rpm, lead), 0);
        sim_trigger_at(s, tdc - sim_angle_us(rpm, trail), 1);
    }
    return tdc;
}
int sim_count(const sim_t *s, int kind, double t0, double t1) {
    int n = 0;
    for (int i = 0; i < s->nev; i++) if (s->ev[i].kind == kind && s->ev[i].t_us >= t0 && s->ev[i].t_us < t1) n++;
    return n;
}
double sim_first(const sim_t *s, int kind, double t0, double t1) {
    for (int i = 0; i < s->nev; i++) if (s->ev[i].kind == kind && s->ev[i].t_us >= t0 && s->ev[i].t_us < t1) return s->ev[i].t_us;
    return -1.0;
}
