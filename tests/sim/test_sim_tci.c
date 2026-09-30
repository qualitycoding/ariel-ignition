/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-202..T-211 firmware-in-the-loop (simavr), battery variant (VARIANT=TCI).
 * Uses DEFAULT configuration (blank EEPROM): lead 50 deg, trail 0 deg (TDC),
 * crank exit 600 rpm x3 cycles, dwell 3700 us at 6300 mV, crank dwell
 * abort 50 ms, rev limit 6000/5700, kill debounce 20 ms, map0 (D-012).
 * Enforces SC-1, SC-2, SC-5, SC-6 and D-018 (runtime behaviour).
 * Categories: integration, operational, performance (T-205/T-206 timing). */
#include "unity.h"
#include "harness.h"
#ifndef FW_ELF
#define FW_ELF "firmware/build/TCI/ariel_ign.elf"
#endif
#define LEAD 5000
#define TRAIL 0
static sim_t *s;
void setUp(void) { s = sim_new(FW_ELF); }
void tearDown(void) { sim_free(s); }

/* spark angle BTDC (deg) of the first FIRE in [tdc - 720deg/2, tdc + 5 ms) */
static double fire_angle(double tdc, double rpm) {
    double f = sim_first(s, EV_FIRE, tdc - sim_angle_us(rpm, 36000), tdc + 5000);
    TEST_ASSERT_TRUE_MESSAGE(f >= 0, "no FIRE near TDC");
    return (tdc - f) / sim_angle_us(rpm, 100);
}

/* T-202: first kick after power-on (boot <= 30 ms): coil on at lead
 * edge, spark at the trail edge (TDC) within 50 us. SC-1, SC-5. */
static void t202_first_kick_fires_at_tdc(void) {
    double tdc = 30000.0 + sim_angle_us(250, LEAD);         /* lead edge at 30 ms */
    sim_cycles(s, tdc, 250, 1, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc + 20000));
    double on = sim_first(s, EV_ON, 29000, tdc);
    TEST_ASSERT_TRUE_MESSAGE(on >= 30000 && on <= 30200, "coil not switched on at first lead edge");
    double f = sim_first(s, EV_FIRE, 0, tdc + 20000);
    TEST_ASSERT_TRUE_MESSAGE(f >= 0, "no spark on first kick");
    TEST_ASSERT_DOUBLE_WITHIN(50.0, tdc, f);
    TEST_ASSERT_EQUAL_INT(1, sim_count(s, EV_FIRE, 0, tdc + 20000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_SOFT, 0, tdc + 20000));
}

/* T-203: kick that stops before TDC -> soft shutdown at lead+50 ms, never a spark. SC-5. */
static void t203_stalled_kick_soft_off(void) {
    sim_trigger_at(s, 30000, 0);                            /* lead edge only */
    TEST_ASSERT_TRUE(sim_run_until(s, 2000000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, 0, 2000000));
    double soft = sim_first(s, EV_SOFT, 30000, 2000000);
    TEST_ASSERT_TRUE_MESSAGE(soft >= 0, "coil left on / no soft shutdown");
    TEST_ASSERT_DOUBLE_WITHIN(2000.0, 80000.0, soft);
    TEST_ASSERT_EQUAL_INT(1, sim_count(s, EV_ON, 0, 2000000));
    TEST_ASSERT_TRUE(sim_out_state(s) != OUT_HIGH);
}

/* T-204: powered up with the S magnet under the sensor (PB0 low): the first
 * (trail) edge must not fire; the next complete cycle must. SC-5. */
static void t204_boot_inside_window(void) {
    sim_trigger_initial(s, 0);
    sim_trigger_at(s, 30000, 1);                            /* trail edge first */
    double tdc = 400000.0;
    sim_cycles(s, tdc, 250, 1, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc + 20000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_ON, 0, 300000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, 0, 300000));
    TEST_ASSERT_DOUBLE_WITHIN(50.0, tdc, sim_first(s, EV_FIRE, 300000, tdc + 20000));
}

/* T-205 (performance): 3000 rpm steady -> 30.0 deg BTDC +/-0.5 deg; dwell 3700 +/-150 us. SC-2. */
static void t205_advance_3000(void) {
    double tdc0 = 50000, cyc = 120e6 / 3000;
    sim_cycles(s, tdc0, 3000, 40, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc0 + 40 * cyc));
    for (int k = 10; k < 40; k++) {
        double tdc = tdc0 + k * cyc;
        TEST_ASSERT_DOUBLE_WITHIN(0.5, 30.0, fire_angle(tdc, 3000));
        double f = sim_first(s, EV_FIRE, tdc - 3000, tdc + 1000);
        double on = sim_first(s, EV_ON, f - 8000, f);
        TEST_ASSERT_TRUE(on >= 0);
        TEST_ASSERT_DOUBLE_WITHIN(150.0, 3700.0, f - on);
    }
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_SOFT, 0, tdc0 + 40 * cyc));
}

/* T-206 (performance): 5500 rpm steady -> 34.0 deg +/-0.5 deg. SC-2. */
static void t206_advance_5500(void) {
    double tdc0 = 50000, cyc = 120e6 / 5500;
    double t = sim_cycles(s, tdc0, 3000, 8, LEAD, TRAIL);    /* get into RUN first */
    sim_cycles(s, t, 5500, 40, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, t + 40 * cyc));
    for (int k = 10; k < 40; k++) TEST_ASSERT_DOUBLE_WITHIN(0.5, 34.0, fire_angle(t + k * cyc, 5500));
}

/* T-207: rev limiter: no spark and no coil current at 6200 rpm; resumes at 5500. SC-6. */
static void t207_rev_limiter(void) {
    double t = sim_cycles(s, 50000, 3000, 8, LEAD, TRAIL);
    double t1 = sim_cycles(s, t, 6200, 20, LEAD, TRAIL);
    double t2 = sim_cycles(s, t1, 5500, 20, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, t2));
    double c62 = 120e6 / 6200, c55 = 120e6 / 5500;
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, t + 3 * c62, t1));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_ON,   t + 3 * c62, t1 - c62));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_SOFT, 0, t2));
    TEST_ASSERT_TRUE(sim_count(s, EV_FIRE, t1 + 3 * c55, t2) >= 15);
}

/* T-208: kill switch: no spark while active (after 20 ms debounce + 1 cycle),
 * any charging coil soft-shut down, sparks resume on release. SC-4. */
static void t208_kill_switch(void) {
    double cyc = 120e6 / 3000;
    double t = sim_cycles(s, 50000, 3000, 30, LEAD, TRAIL);
    double k0 = 50000 + 10 * cyc + 7000, k1 = k0 + 300000;
    sim_kill_at(s, k0, true); sim_kill_at(s, k1, false);
    TEST_ASSERT_TRUE(sim_run_until(s, t));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, k0 + 20000 + cyc, k1));
    TEST_ASSERT_TRUE(sim_count(s, EV_FIRE, k1 + 20000 + cyc, t) >= 5);
    /* kill during a crank-mode dwell */
    sim_free(s); s = sim_new(FW_ELF);
    sim_trigger_at(s, 30000, 0); sim_kill_at(s, 35000, true); sim_trigger_at(s, 60000, 1);
    TEST_ASSERT_TRUE(sim_run_until(s, 200000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, 0, 200000));
    double soft = sim_first(s, EV_SOFT, 35000, 200000);
    TEST_ASSERT_TRUE_MESSAGE(soft >= 0 && soft <= 35000 + 25000, "kill did not soft-shutdown the coil");
}

/* T-209: supply below vbat_min (4500 mV) while running -> limp mode: fire at trail (TDC). SC-5. */
static void t209_low_voltage_limp(void) {
    double cyc = 120e6 / 3000;
    sim_vbat_at(s, 0, 4200);
    sim_cycles(s, 50000, 3000, 20, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, 50000 + 20 * cyc));
    for (int k = 5; k < 20; k++) {
        double tdc = 50000 + k * cyc;
        TEST_ASSERT_DOUBLE_WITHIN(50.0, tdc, sim_first(s, EV_FIRE, tdc - 3000, tdc + 1000));
    }
}

/* T-210: stall while running: predicted dwell with no lead edge -> soft
 * shutdown within dwell_max (8 ms) + 1 ms, no spark; restart fires at TDC. SC-5. */
static void t210_stall_while_running(void) {
    double cyc = 120e6 / 3000;
    double tend = sim_cycles(s, 50000, 3000, 20, LEAD, TRAIL);   /* last trail at tend - cyc */
    double last = tend - cyc;
    double tdc = last + 3000000;                                   /* re-kick after 3 s */
    sim_cycles(s, tdc, 250, 1, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc + 20000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, last + 1000, tdc - 1000));
    double on = sim_first(s, EV_ON, last + 1000, tdc - 100000);
    if (on >= 0) {
        double soft = sim_first(s, EV_SOFT, on, tdc - 100000);
        TEST_ASSERT_TRUE_MESSAGE(soft >= 0 && soft - on <= 9000, "stall: coil not soft-shut down");
    }
    TEST_ASSERT_TRUE(sim_out_state(s) != OUT_HIGH || sim_first(s, EV_ON, tdc - 100000, tdc) >= 0);
    TEST_ASSERT_DOUBLE_WITHIN(50.0, tdc, sim_first(s, EV_FIRE, tdc - 100000, tdc + 20000));
}

/* T-211: map switch grounded -> map 1 (26.0 deg at 3000 rpm). SC-3. */
static void t211_map_switch(void) {
    double cyc = 120e6 / 3000;
    sim_map1_at(s, 0, true);
    sim_cycles(s, 50000, 3000, 30, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, 50000 + 30 * cyc));
    for (int k = 10; k < 30; k++) TEST_ASSERT_DOUBLE_WITHIN(0.5, 26.0, fire_angle(50000 + k * cyc, 3000));
}
int main(void) { UNITY_BEGIN();
    RUN_TEST(t202_first_kick_fires_at_tdc); RUN_TEST(t203_stalled_kick_soft_off); RUN_TEST(t204_boot_inside_window);
    RUN_TEST(t205_advance_3000); RUN_TEST(t206_advance_5500); RUN_TEST(t207_rev_limiter);
    RUN_TEST(t208_kill_switch); RUN_TEST(t209_low_voltage_limp); RUN_TEST(t210_stall_while_running);
    RUN_TEST(t211_map_switch); return UNITY_END(); }
