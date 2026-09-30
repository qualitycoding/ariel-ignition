/* Ignition runtime (D-018). Two variants share this file:
 *   TCI      : PB1 HIGH = coil charging, LOW = spark, Hi-Z = soft shutdown.
 *   MAGBREAK : PB1 HIGH = breaker closed (magneto primary shorted),
 *              HIGH -> LOW = spark, re-closed magbreak_open_cdeg later. */
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "hal.h"
#include "runtime.h"
#include "timing.h"

#ifdef DEFAULT_VARIANT_MAGBREAK
#define IS_MAGBREAK 1
#else
#define IS_MAGBREAK 0
#endif

enum { M_STOP = 0, M_CRANK, M_RUN };
enum { EV_NONE = 0, EV_DWELL, EV_ABORT, EV_RECLOSE };

#define EV_MARGIN_US   16      /* closer than this: do it now, don't arm a compare */
#define SPARK_MARGIN_US 12
#define KILL_DEBOUNCE_MS 20

static config_t cfg;                         /* active configuration                 */
static uint8_t  stored_map;                  /* map from the saved configuration     */
static uint32_t glitch_us;                   /* minimum plausible lead-to-lead time  */

static volatile uint8_t  mode;
static volatile uint16_t rpm_now;
static volatile uint16_t vbat_mv;
static volatile bool     killed;
static volatile uint16_t hold_ms;            /* TCI: soft-shutdown hold remaining    */

static bool     coil_on;                     /* PB1 driven HIGH                      */
static bool     spark_armed;                 /* OC1A compare armed                   */
static bool     spark_sched;                 /* spark_target valid for this cycle    */
static bool     lead_seen, have_lead, period_ok, seg_ok, limiting, crank_cycle;
static uint32_t last_lead, last_period, last_seg, spark_target;
static uint8_t  good;
static uint16_t since_edge_ms, boot_ms, tick16;
static uint8_t  kill_cnt;
static uint32_t ev_target;
static uint8_t  ev_act;
static bool     defaults_flag;

static uint16_t nz_magic __attribute__((section(".noinit")));
static uint8_t  nz_resets __attribute__((section(".noinit")));

static bool low_vbat(void) { return !IS_MAGBREAK && vbat_mv < cfg.vbat_min_mv; }

/* ---- output stage ------------------------------------------------------ */
static bool out_on(void)
{
    if (!IS_MAGBREAK && hold_ms) return false;      /* still in soft-shutdown hold */
    hal_out_high();
    coil_on = true;
    return true;
}
static void out_spark_sw(void)                      /* software spark / breaker open */
{
    hal_out_low();
    coil_on = false; spark_armed = false; spark_sched = false;
}
static void out_abort(void)                         /* TCI soft shutdown (D-019) */
{
    hal_out_hiz();
    coil_on = false; spark_armed = false; spark_sched = false;
    hold_ms = cfg.ssd_hold_ms;
}

/* ---- OC1B software event timer ---------------------------------------- */
static void ev_dispatch(void);
static void ev_cancel(void) { hal_ev_off(); ev_act = EV_NONE; }
static void ev_program(void)
{
    uint32_t now = hal_now_us();
    int32_t d = (int32_t)(ev_target - now);
    if (d <= EV_MARGIN_US) { ev_dispatch(); return; }
    if (d > 60000L) d = 60000L;                     /* re-armed from the ISR if longer */
    hal_ev_set((uint16_t)(now + (uint32_t)d));
}
static void ev_arm(uint32_t target, uint8_t act)
{
    ev_target = target; ev_act = act;
    ev_program();
}

/* ---- spark bookkeeping -------------------------------------------------- */
static uint32_t magbreak_open_us(void)
{
    if (period_ok) return (uint32_t)angle_to_us(cfg.magbreak_open_cdeg, last_period, cfg.cycle_div);
    if (seg_ok) {
        uint32_t span = (uint32_t)(cfg.lead_cdeg - cfg.trail_cdeg);
        return (last_seg * cfg.magbreak_open_cdeg + span / 2u) / span;
    }
    return 20000UL;
}

/* Called after every spark (hardware edge or software). t = spark time. */
static void spark_done(uint32_t t)
{
    if (IS_MAGBREAK) {
        ev_arm(t + magbreak_open_us(), EV_RECLOSE);
        return;
    }
    if (mode == M_RUN && !crank_cycle && !killed && period_ok && hold_ms == 0) {
        uint16_t dw = dwell_for_vbat(&cfg, vbat_mv);
        uint32_t next_on = t + last_period - dw;             /* next spark ~ t + period */
        uint32_t exp_lead = last_lead + last_period;
        if ((int32_t)(next_on - exp_lead) < 0) ev_arm(next_on, EV_DWELL);
    }
}

static void spark_arm(uint32_t target)
{
    int32_t d = (int32_t)(target - hal_now_us());
    if (d < SPARK_MARGIN_US) {                       /* too late for the compare: fire now */
        out_spark_sw();
        spark_done(target);
        return;
    }
    hal_spark_arm((uint16_t)target);
    spark_armed = true;
}

static void dwell_start(void)
{
    uint32_t now;
    if (killed || limiting || coil_on || (!IS_MAGBREAK && hold_ms)) return;
    now = hal_now_us();
    if (spark_sched) {                               /* spark time known (scheduled at the lead edge) */
        if ((int32_t)(spark_target - now) < SPARK_MARGIN_US) { spark_sched = false; return; }
        if (!out_on()) return;
        spark_arm(spark_target);
    } else {                                         /* predicted RUN dwell or delayed crank dwell */
        if (!out_on()) return;
        ev_arm(now + (crank_cycle ? cfg.crank_dwell_max_us : cfg.dwell_max_us), EV_ABORT);
    }
}

static void ev_dispatch(void)
{
    uint8_t act = ev_act;
    ev_cancel();
    switch (act) {
    case EV_DWELL:   dwell_start(); break;
    case EV_ABORT:   if (!IS_MAGBREAK && coil_on && !spark_armed) out_abort(); break;
    case EV_RECLOSE: if (!coil_on && !killed) out_on(); break;
    default: break;
    }
}

/* ---- trigger edges ------------------------------------------------------ */
static void crank_lead(uint32_t tL)
{
    spark_sched = false;
    if (IS_MAGBREAK) { if (!coil_on) out_on(); return; }     /* closed; opens at the trail edge */
    if (coil_on || hold_ms) return;
    if (seg_ok && last_seg > cfg.dwell_crank_us) {
        uint32_t d = last_seg - cfg.dwell_crank_us;
        if (d > 60000UL) d = 60000UL;
        ev_arm(tL + d, EV_DWELL);
    } else if (out_on()) {
        ev_arm(tL + cfg.crank_dwell_max_us, EV_ABORT);
    }
}

static void run_lead(uint32_t tL)
{
    uint16_t adv = effective_advance(&cfg, rpm_now);
    int32_t delay = seg_ok
        ? segment_delay_us(last_seg, cfg.lead_cdeg, cfg.trail_cdeg, adv, cfg.sensor_latency_us)
        : spark_delay_from_lead_us(cfg.lead_cdeg, adv, last_period, cfg.cycle_div, cfg.sensor_latency_us);
    uint32_t target;

    if (delay < 0 || delay > 60000L) {               /* cannot predict: spark at TDC instead */
        crank_cycle = true;
        crank_lead(tL);
        return;
    }
    if (spark_armed) return;
    target = tL + (uint32_t)delay;

    if (IS_MAGBREAK) {
        if (!coil_on) out_on();
        if (limiting) return;                        /* rev limit: stay closed, no spark */
        spark_sched = true; spark_target = target;
        spark_arm(target);
        return;
    }
    if (coil_on) {                                   /* predicted dwell running: always end in a spark */
        spark_sched = true; spark_target = target;
        spark_arm(target);
        return;
    }
    if (limiting || hold_ms) return;
    {
        uint16_t dwell = dwell_for_vbat(&cfg, vbat_mv);
        uint32_t on_at = target - dwell;
        spark_sched = true; spark_target = target;
        if ((int32_t)(on_at - hal_now_us()) <= EV_MARGIN_US) { if (out_on()) spark_arm(target); }
        else ev_arm(on_at, EV_DWELL);
    }
}

static void on_lead(uint32_t tL)
{
    uint32_t stall_us = (uint32_t)cfg.stall_timeout_ms * 1000UL;
    bool have_period = false;
    uint32_t dt = 0;

    if (have_lead) {
        dt = tL - last_lead;
        if (dt < glitch_us) return;                  /* faster than the rev limit allows: glitch */
        have_period = dt < stall_us;
    }
    period_ok = have_period;
    if (have_period) {
        last_period = dt;
        rpm_now = rpm_from_period_us(dt, cfg.cycle_div);
    } else {
        rpm_now = 0;
    }
    last_lead = tL; have_lead = true; lead_seen = true;

    if (mode == M_STOP) mode = M_CRANK;
    if (period_ok) {
        uint16_t r = rpm_now;
        if (r >= cfg.crank_exit_rpm) {
            if (good < 255u) good++;
            if (good >= cfg.crank_exit_cycles) mode = M_RUN;
        } else {
            good = 0;
        }
        if (r < cfg.crank_enter_rpm) mode = M_CRANK;
        if (r >= cfg.rev_limit_rpm) limiting = true;
        else if (r <= cfg.rev_resume_rpm) limiting = false;
    } else {
        good = 0; mode = M_CRANK;
    }

    ev_cancel();                                     /* stale predicted dwell / abort / reclose */
    if (killed) { if (IS_MAGBREAK && !coil_on) out_on(); return; }
    crank_cycle = (mode == M_CRANK) || low_vbat();
    if (crank_cycle) crank_lead(tL); else run_lead(tL);
}

static void on_trail(uint32_t tT)
{
    uint32_t s;
    if (!lead_seen) return;                          /* boot inside the window, or a glitch */
    lead_seen = false;
    s = tT - last_lead;
    if (s < 300000UL) { last_seg = s; seg_ok = true; }
    if (killed) return;
    if (IS_MAGBREAK) {
        if (crank_cycle && coil_on) { out_spark_sw(); spark_done(tT); }
        return;                                      /* RUN: the compare already opened it */
    }
    if (coil_on && !spark_armed) { out_spark_sw(); spark_done(tT); }   /* crank / limp / late backstop */
}

ISR(TIMER1_CAPT_vect)
{
    uint16_t c = ICR1;
    bool lead = hal_trig_low();                      /* latch is LOW between LEAD and TRAIL */
    uint32_t t = hal_extend_us(c);
    hal_ic_follow_pin();                             /* next edge = opposite of what we just saw */
    since_edge_ms = 0;
    if (lead) on_lead(t); else on_trail(t);
}

ISR(TIMER1_COMPA_vect)                               /* the hardware edge just produced the spark */
{
    hal_spark_done();
    coil_on = false; spark_armed = false; spark_sched = false;
    ev_cancel();
    spark_done(spark_target);
}

ISR(TIMER1_COMPB_vect)
{
    if (ev_act == EV_NONE) { hal_ev_off(); return; }
    if ((int32_t)(ev_target - hal_now_us()) > EV_MARGIN_US) { ev_program(); return; }
    ev_dispatch();
}

/* ---- 1 ms tick ---------------------------------------------------------- */
static void enter_stopped(void)
{
    mode = M_STOP; rpm_now = 0;
    have_lead = false; period_ok = false; seg_ok = false; lead_seen = false;
    good = 0; limiting = false; crank_cycle = false;
    ev_cancel(); spark_sched = false;
    if (IS_MAGBREAK) { hal_spark_cancel(); spark_armed = false; }
    else if (coil_on || spark_armed) out_abort();
}

static void kill_engage(void)
{
    killed = true;
    ev_cancel(); spark_sched = false;
    if (IS_MAGBREAK) {                               /* hold the breaker closed (cut-out convention) */
        hal_spark_cancel(); spark_armed = false;
        if (!coil_on) out_on();
    } else if (coil_on || spark_armed) {
        out_abort();
    }
}

ISR(TIMER0_COMPA_vect)
{
    bool raw, led;
    wdt_reset();
    tick16++;
    if (boot_ms < 65535u) boot_ms++;

    if (mode != M_STOP) {
        if (since_edge_ms < 65535u) since_edge_ms++;
        if (since_edge_ms >= cfg.stall_timeout_ms) enter_stopped();
    }

    raw = hal_kill_raw();
    if (raw != killed) {
        if (++kill_cnt >= KILL_DEBOUNCE_MS) {
            kill_cnt = 0;
            if (raw) kill_engage(); else killed = false;
        }
    } else {
        kill_cnt = 0;
    }

    cfg.active_map = hal_map_pin() ? 1u : stored_map;

    if (hold_ms && --hold_ms == 0 && !IS_MAGBREAK) hal_out_drive_low();

    if (!(ADCSRA & _BV(ADSC))) {
        vbat_mv = hal_vbat_from_adc(ADC);
        ADCSRA |= _BV(ADSC);
    }

    if (mode == M_STOP && !(TIFR1 & _BV(ICF1))) hal_ic_follow_pin();   /* resync if an edge was missed */

    if (defaults_flag && boot_ms < 700u) led = (boot_ms % 300u) < 150u;
    else if (killed) led = (tick16 >> 6) & 1u;
    else if (mode == M_STOP) led = hal_trig_low();
    else led = false;
    hal_led(led);
}

/* ---- public API ---------------------------------------------------------- */
static void set_glitch(void)
{
    glitch_us = (60000000UL * cfg.cycle_div) / (2UL * cfg.rev_limit_rpm);
}

void runtime_init(const config_t *c, bool defaults_loaded, uint8_t mcusr)
{
    cfg = *c;
    stored_map = c->active_map;
    defaults_flag = defaults_loaded;
    set_glitch();
    vbat_mv = hal_vbat_from_adc(ADC);
    mode = M_STOP;
    if (mcusr & _BV(PORF)) { nz_magic = 0xBEEF; nz_resets = 0; }       /* fresh power-up */
    else if (nz_magic == 0xBEEF) { if (nz_resets < 255u) nz_resets++; } /* reset while powered */
    else { nz_magic = 0xBEEF; nz_resets = 0; }
}

void runtime_apply_config(const config_t *c)
{
    uint8_t s = SREG;
    cli();
    cfg = *c;
    stored_map = c->active_map;
    set_glitch();
    SREG = s;
}

uint16_t runtime_rpm(void)
{
    uint8_t s = SREG; uint16_t v;
    cli(); v = rpm_now; SREG = s;
    return v;
}

static char *put_u(char *p, uint32_t v)
{
    char t[10]; uint8_t k = 0;
    do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (k) *p++ = t[--k];
    return p;
}
static char *put_s(char *p, const char *s) { while (*s) *p++ = *s++; return p; }

void runtime_status(char *out, size_t n)
{
    uint8_t s = SREG;
    uint16_t rpm, vb, adv; uint8_t m, map; bool k, h, lv; char buf[64]; char *p = buf;
    const char *name;
    cli();
    rpm = rpm_now; vb = vbat_mv; m = mode; k = killed; h = hold_ms != 0; lv = low_vbat();
    map = cfg.active_map;
    adv = rpm ? effective_advance(&cfg, rpm) : 0u;
    SREG = s;
    name = k ? "KILL" : (m == M_STOP) ? "STOP" : lv ? "LIMP" : h ? "HOLD" : (m == M_RUN) ? "RUN" : "CRANK";
    p = put_s(p, "rpm="); p = put_u(p, rpm);
    p = put_s(p, " mode="); p = put_s(p, name);
    p = put_s(p, " adv="); p = put_u(p, adv);
    p = put_s(p, " vbat="); p = put_u(p, vb);
    p = put_s(p, " map="); p = put_u(p, map);
    p = put_s(p, " resets="); p = put_u(p, nz_resets);
    *p = 0;
    if (n) { strncpy(out, buf, n - 1); out[n - 1] = 0; }
}
