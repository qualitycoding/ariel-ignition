/* Pure timing arithmetic (D-013, D-017). Integer only. 32-bit paths are used
 * whenever the product fits (AVR has no 64-bit hardware); 64-bit otherwise. */
#include "timing.h"

/* round(a*b/d), d > 0, saturating at UINT32_MAX. */
static uint32_t muldiv_round(uint32_t a, uint32_t b, uint32_t d)
{
    uint32_t half = d / 2u;
    if (b == 0u || a <= (UINT32_MAX - half) / b)
        return (a * b + half) / d;
    {
        uint64_t q = ((uint64_t)a * b + half) / d;
        return q > UINT32_MAX ? UINT32_MAX : (uint32_t)q;
    }
}

uint16_t rpm_from_period_us(uint32_t period_us, uint8_t cycle_div)
{
    uint32_t q;
    if (period_us == 0u) return 0u;
    if (60000000UL <= UINT32_MAX / (cycle_div ? cycle_div : 1u))
        q = (60000000UL * cycle_div) / period_us;               /* truncating */
    else
        q = (uint32_t)(((uint64_t)60000000UL * cycle_div) / period_us);
    return q > 65535u ? 65535u : (uint16_t)q;
}

int32_t angle_to_us(int32_t cdeg, uint32_t period_us, uint8_t cycle_div)
{
    uint32_t mag = cdeg < 0 ? (uint32_t)(-(int64_t)cdeg) : (uint32_t)cdeg;
    uint32_t den = 36000UL * cycle_div;
    uint32_t r;
    if (den == 0u) return 0;
    r = muldiv_round(mag, period_us, den);
    if (r > (uint32_t)INT32_MAX) r = (uint32_t)INT32_MAX;
    return cdeg < 0 ? -(int32_t)r : (int32_t)r;
}

uint16_t curve_lookup(const curve_t *cv, uint16_t rpm)
{
    if (rpm <= cv->rpm[0]) return cv->adv_cdeg[0];
    if (rpm >= cv->rpm[CURVE_POINTS - 1]) return cv->adv_cdeg[CURVE_POINTS - 1];
    for (uint8_t i = 0; i < CURVE_POINTS - 1; i++) {
        if (rpm < cv->rpm[i + 1]) {
            int32_t da = (int32_t)cv->adv_cdeg[i + 1] - (int32_t)cv->adv_cdeg[i];
            int32_t dr = (int32_t)rpm - (int32_t)cv->rpm[i];
            int32_t span = (int32_t)cv->rpm[i + 1] - (int32_t)cv->rpm[i];
            int32_t q;
            if (da >= -32767 && da <= 32767)
                q = (da * dr) / span;                             /* truncates toward zero */
            else
                q = (int32_t)(((int64_t)da * dr) / span);
            return (uint16_t)((int32_t)cv->adv_cdeg[i] + q);
        }
    }
    return cv->adv_cdeg[CURVE_POINTS - 1];
}

uint16_t effective_advance(const config_t *c, uint16_t rpm)
{
    int32_t v = (int32_t)curve_lookup(&c->maps[c->active_map ? 1 : 0], rpm) + c->trim_cdeg;
    int32_t hi = (int32_t)c->adv_max_cdeg;
    int32_t lo = (int32_t)c->adv_min_cdeg;
    int32_t lead_ceiling = (int32_t)c->lead_cdeg - 100;
    if (lead_ceiling < hi) hi = lead_ceiling;
    if (hi < 0) hi = 0;
    if (v < lo) v = lo;
    if (v > hi) v = hi;                 /* ceiling wins over floor */
    if (v < 0) v = 0;
    return (uint16_t)v;
}

int32_t spark_delay_from_lead_us(uint16_t lead_cdeg, uint16_t adv_cdeg,
                                 uint32_t period_us, uint8_t cycle_div, uint16_t latency_us)
{
    int32_t us;
    if (adv_cdeg >= lead_cdeg) return -1;
    us = angle_to_us((int32_t)lead_cdeg - (int32_t)adv_cdeg, period_us, cycle_div) - (int32_t)latency_us;
    return us < 0 ? -1 : us;
}

int32_t segment_delay_us(uint32_t seg_us, uint16_t lead_cdeg, uint16_t trail_cdeg,
                         uint16_t adv_cdeg, uint16_t latency_us)
{
    uint32_t r;
    int32_t us;
    if (lead_cdeg <= trail_cdeg || adv_cdeg >= lead_cdeg) return -1;
    r = muldiv_round(seg_us, (uint32_t)(lead_cdeg - adv_cdeg), (uint32_t)(lead_cdeg - trail_cdeg));
    if (r > (uint32_t)INT32_MAX) r = (uint32_t)INT32_MAX;
    us = (int32_t)r - (int32_t)latency_us;
    return us < 0 ? -1 : us;
}

uint16_t dwell_for_vbat(const config_t *c, uint16_t vbat_mv)
{
    uint16_t d;
    if (vbat_mv <= c->dwell_mv[0]) {
        d = c->dwell_us[0];
    } else if (vbat_mv >= c->dwell_mv[DWELL_POINTS - 1]) {
        d = c->dwell_us[DWELL_POINTS - 1];
    } else {
        d = c->dwell_us[DWELL_POINTS - 1];
        for (uint8_t i = 0; i < DWELL_POINTS - 1; i++) {
            if (vbat_mv < c->dwell_mv[i + 1]) {
                int32_t da = (int32_t)c->dwell_us[i + 1] - (int32_t)c->dwell_us[i];
                int32_t dv = (int32_t)vbat_mv - (int32_t)c->dwell_mv[i];
                int32_t span = (int32_t)c->dwell_mv[i + 1] - (int32_t)c->dwell_mv[i];
                d = (uint16_t)((int32_t)c->dwell_us[i] + (da * dv) / span);
                break;
            }
        }
    }
    return d > c->dwell_max_us ? c->dwell_max_us : d;
}
