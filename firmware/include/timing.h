/* timing.h — pure timing arithmetic. Interface frozen by D-013. */
#ifndef ARIEL_TIMING_H
#define ARIEL_TIMING_H
#include "common.h"
#include "config.h"

/* rpm from the period between successive lead edges.
 * cycle_div=2 => period spans 720 crank deg; cycle_div=1 => 360.
 * rpm = 60e6 * cycle_div / period_us, truncated; 0 if period_us==0;
 * saturates at 65535. */
uint16_t rpm_from_period_us(uint32_t period_us, uint8_t cycle_div);

/* Convert a crank angle (cdeg, may be negative) to microseconds at the
 * given lead-to-lead period. us = cdeg * period_us / (36000 * cycle_div),
 * rounded half away from zero. */
int32_t  angle_to_us(int32_t cdeg, uint32_t period_us, uint8_t cycle_div);

/* Linear interpolation on a curve. rpm <= rpm[0] -> adv[0];
 * rpm >= rpm[last] -> adv[last]; otherwise
 * adv[i] + (adv[i+1]-adv[i])*(rpm-rpm[i])/(rpm[i+1]-rpm[i]) with C integer
 * division (truncation toward zero). */
uint16_t curve_lookup(const curve_t *cv, uint16_t rpm);

/* Advance actually commanded: curve(active map) + trim, then clamped to
 * [adv_min_cdeg, min(adv_max_cdeg, lead_cdeg - 100)]. Result never
 * negative. */
uint16_t effective_advance(const config_t *c, uint16_t rpm);

/* Delay from the lead edge to the spark: angle_to_us(lead - adv) minus
 * sensor_latency_us. Returns -1 if adv >= lead or the result would be < 0. */
int32_t  spark_delay_from_lead_us(uint16_t lead_cdeg, uint16_t adv_cdeg,
                                  uint32_t period_us, uint8_t cycle_div,
                                  uint16_t latency_us);

/* Segment method (D-017): the previous cycle's lead->trail time seg_us
 * spans (lead_cdeg - trail_cdeg). Delay from lead edge to spark =
 * seg_us * (lead_cdeg - adv_cdeg) / (lead_cdeg - trail_cdeg), rounded half
 * away from zero, minus latency_us. Returns -1 if lead_cdeg <= trail_cdeg,
 * adv_cdeg >= lead_cdeg, or the result would be < 0. */
int32_t  segment_delay_us(uint32_t seg_us, uint16_t lead_cdeg,
                          uint16_t trail_cdeg, uint16_t adv_cdeg,
                          uint16_t latency_us);

/* Dwell for battery voltage: linear interpolation over dwell_mv/dwell_us
 * (clamped at the ends), then clamped to dwell_max_us. */
uint16_t dwell_for_vbat(const config_t *c, uint16_t vbat_mv);
#endif
