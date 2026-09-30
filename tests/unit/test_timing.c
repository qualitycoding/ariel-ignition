/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-005..T-010 pure timing arithmetic (firmware/include/timing.h, D-013, D-017).
 * Enforces: SC-2 (timing accuracy), SC-3 (programmable curve), C-006.
 * Expected values computed by research/spikes/timing-reference/ref.py
 * (exact integer arithmetic). Category: unit. */
#include "unity.h"
#include "timing.h"
void setUp(void) {} void tearDown(void) {}
static config_t c;

/* T-005: rpm from lead-to-lead period; saturation; zero period. */
static void t005_rpm_from_period(void) {
    TEST_ASSERT_EQUAL_UINT16(3000,  rpm_from_period_us(40000, 2));
    TEST_ASSERT_EQUAL_UINT16(6000,  rpm_from_period_us(20000, 2));
    TEST_ASSERT_EQUAL_UINT16(1500,  rpm_from_period_us(40000, 1));
    TEST_ASSERT_EQUAL_UINT16(250,   rpm_from_period_us(480000, 2));
    TEST_ASSERT_EQUAL_UINT16(65535, rpm_from_period_us(1831, 2));
    TEST_ASSERT_EQUAL_UINT16(65535, rpm_from_period_us(1, 2));
    TEST_ASSERT_EQUAL_UINT16(0,     rpm_from_period_us(0, 2));
}

/* T-006: angle to microseconds, rounding half away from zero. */
static void t006_angle_to_us(void) {
    TEST_ASSERT_EQUAL_INT32(2222,  angle_to_us(4000, 40000, 2));
    TEST_ASSERT_EQUAL_INT32(-83,   angle_to_us(-150, 40000, 2));
    TEST_ASSERT_EQUAL_INT32(1125,  angle_to_us(3375, 24000, 2));
    TEST_ASSERT_EQUAL_INT32(0,     angle_to_us(1, 7, 2));
    TEST_ASSERT_EQUAL_INT32(1,     angle_to_us(1, 36000, 2));     /* exactly 0.5 */
    TEST_ASSERT_EQUAL_INT32(-1,    angle_to_us(-1, 36000, 2));    /* exactly -0.5 */
    TEST_ASSERT_EQUAL_INT32(33333, angle_to_us(5000, 480000, 2)); /* no 32-bit overflow */
    TEST_ASSERT_EQUAL_INT32(1667,  angle_to_us(3000, 20000, 1));
    TEST_ASSERT_EQUAL_INT32(400000, angle_to_us(72000, 400000, 2)); /* full cycle */
}

/* T-007: curve interpolation with end clamping and truncation. */
static void t007_curve_lookup(void) {
    config_defaults(&c);
    const curve_t *m = &c.maps[0];
    TEST_ASSERT_EQUAL_UINT16(0,    curve_lookup(m, 0));
    TEST_ASSERT_EQUAL_UINT16(0,    curve_lookup(m, 500));
    TEST_ASSERT_EQUAL_UINT16(300,  curve_lookup(m, 650));
    TEST_ASSERT_EQUAL_UINT16(900,  curve_lookup(m, 1000));
    TEST_ASSERT_EQUAL_UINT16(1998, curve_lookup(m, 1799));
    TEST_ASSERT_EQUAL_UINT16(2000, curve_lookup(m, 1800));
    TEST_ASSERT_EQUAL_UINT16(2300, curve_lookup(m, 2100));
    TEST_ASSERT_EQUAL_UINT16(3200, curve_lookup(m, 3300));
    TEST_ASSERT_EQUAL_UINT16(3400, curve_lookup(m, 4500));
    TEST_ASSERT_EQUAL_UINT16(3400, curve_lookup(m, 65535));
    /* decreasing segment: truncation toward zero */
    curve_t d = { {100,200,300,400,500,600,700,800}, {1000,0,0,0,0,0,0,0} };
    TEST_ASSERT_EQUAL_UINT16(330, curve_lookup(&d, 167)); /* 1000 + (-1000*67)/100 = 1000-670 */
    TEST_ASSERT_EQUAL_UINT16(990, curve_lookup(&d, 101)); /* 1000 + (-1000*1)/100 = 990 */
}

/* T-008: effective advance = curve(active map) + trim, clamped. */
static void t008_effective_advance(void) {
    config_defaults(&c);
    TEST_ASSERT_EQUAL_UINT16(3000, effective_advance(&c, 3000));
    c.trim_cdeg = 300;  TEST_ASSERT_EQUAL_UINT16(3300, effective_advance(&c, 3000));
    c.adv_max_cdeg = 3100; TEST_ASSERT_EQUAL_UINT16(3100, effective_advance(&c, 3000));
    config_defaults(&c);
    c.trim_cdeg = -500; TEST_ASSERT_EQUAL_UINT16(0, effective_advance(&c, 500));   /* never negative */
    c.adv_min_cdeg = 200; TEST_ASSERT_EQUAL_UINT16(200, effective_advance(&c, 500));
    config_defaults(&c);
    c.active_map = 1;   TEST_ASSERT_EQUAL_UINT16(2600, effective_advance(&c, 3000));
    config_defaults(&c);                          /* lead - 100 ceiling */
    c.lead_cdeg = 3000; c.adv_max_cdeg = 4500;    /* (config invalid, function must still clamp) */
    TEST_ASSERT_EQUAL_UINT16(2900, effective_advance(&c, 4500));
}

/* T-009: spark delay from lead edge, period method and segment method. */
static void t009_spark_delays(void) {
    TEST_ASSERT_EQUAL_INT32(1111, spark_delay_from_lead_us(5000, 3000, 40000, 2, 0));
    TEST_ASSERT_EQUAL_INT32(1086, spark_delay_from_lead_us(5000, 3000, 40000, 2, 25));
    TEST_ASSERT_EQUAL_INT32(-1,   spark_delay_from_lead_us(5000, 5000, 40000, 2, 0));
    TEST_ASSERT_EQUAL_INT32(-1,   spark_delay_from_lead_us(5000, 4990, 40000, 2, 100));
    TEST_ASSERT_EQUAL_INT32(508,  spark_delay_from_lead_us(5000, 3400, 24000, 2, 25));
    TEST_ASSERT_EQUAL_INT32(1111, segment_delay_us(2778, 5000, 0, 3000, 0));
    TEST_ASSERT_EQUAL_INT32(1086, segment_delay_us(2778, 5000, 0, 3000, 25));
    TEST_ASSERT_EQUAL_INT32(10000, segment_delay_us(10000, 5000, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT32(-1,   segment_delay_us(100, 5000, 0, 4990, 30));
    TEST_ASSERT_EQUAL_INT32(2503, segment_delay_us(3333, 5000, 0, 1234, 7));
    TEST_ASSERT_EQUAL_INT32(-1,   segment_delay_us(3333, 500, 500, 100, 0));   /* lead <= trail */
    TEST_ASSERT_EQUAL_INT32(-1,   segment_delay_us(3333, 5000, 0, 5000, 0));   /* adv >= lead   */
}

/* T-010: dwell from battery voltage (6 V default table). */
static void t010_dwell_for_vbat(void) {
    config_defaults(&c);
    TEST_ASSERT_EQUAL_UINT16(6000, dwell_for_vbat(&c, 4000));
    TEST_ASSERT_EQUAL_UINT16(6000, dwell_for_vbat(&c, 4500));
    TEST_ASSERT_EQUAL_UINT16(5250, dwell_for_vbat(&c, 5000));
    TEST_ASSERT_EQUAL_UINT16(3250, dwell_for_vbat(&c, 7000));
    TEST_ASSERT_EQUAL_UINT16(3000, dwell_for_vbat(&c, 9000));
    c.dwell_max_us = 5000;
    TEST_ASSERT_EQUAL_UINT16(5000, dwell_for_vbat(&c, 4000));
}
int main(void) { UNITY_BEGIN(); RUN_TEST(t005_rpm_from_period); RUN_TEST(t006_angle_to_us);
    RUN_TEST(t007_curve_lookup); RUN_TEST(t008_effective_advance); RUN_TEST(t009_spark_delays);
    RUN_TEST(t010_dwell_for_vbat); return UNITY_END(); }
