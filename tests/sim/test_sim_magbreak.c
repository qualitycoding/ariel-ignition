/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-220..T-222 firmware-in-the-loop, battery-less electronic-breaker variant
 * (VARIANT=MAGBREAK, D-016). PB1 HIGH = breaker switch closed (primary
 * shorted, like closed points); HIGH->LOW = breaker opens = spark.
 * Default config: breaker re-closes magbreak_open_cdeg (60 deg) after opening.
 * Enforces SC-1 (battery-less), SC-4 (kill), SC-5. Category: integration. */
#include "unity.h"
#include "harness.h"
#ifndef FW_ELF_MAG
#define FW_ELF_MAG "firmware/build/MAGBREAK/ariel_ign.elf"
#endif
#define LEAD 5000
#define TRAIL 0
static sim_t *s;
void setUp(void) { s = sim_new(FW_ELF_MAG); }
void tearDown(void) { sim_free(s); }

/* T-220: first kick: breaker open until the first lead edge, closed at the
 * lead edge, opens at TDC (+/-50 us), re-closes 60 deg later. */
static void t220_first_kick(void) {
    double tdc = 30000.0 + sim_angle_us(250, LEAD);
    sim_cycles(s, tdc, 250, 1, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc + 80000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_ON, 0, 29000));
    TEST_ASSERT_DOUBLE_WITHIN(200.0, 30100.0, sim_first(s, EV_ON, 29000, tdc));
    TEST_ASSERT_DOUBLE_WITHIN(50.0, tdc, sim_first(s, EV_FIRE, 0, tdc + 80000));
    TEST_ASSERT_DOUBLE_WITHIN(500.0, tdc + sim_angle_us(250, 6000), sim_first(s, EV_ON, tdc, tdc + 80000));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_SOFT, 0, tdc + 80000));
}

/* T-221: running at 3000 rpm: opens at 30.0 +/-0.5 deg BTDC, re-closes 60 +/-1 deg later. */
static void t221_running(void) {
    double tdc0 = 50000, cyc = 120e6 / 3000;
    sim_cycles(s, tdc0, 3000, 30, LEAD, TRAIL);
    TEST_ASSERT_TRUE(sim_run_until(s, tdc0 + 30 * cyc));
    for (int k = 10; k < 30; k++) {
        double tdc = tdc0 + k * cyc;
        double f = sim_first(s, EV_FIRE, tdc - sim_angle_us(3000, 36000), tdc + 5000);
        TEST_ASSERT_TRUE(f >= 0);
        TEST_ASSERT_DOUBLE_WITHIN(0.5, 30.0, (tdc - f) / sim_angle_us(3000, 100));
        double on = sim_first(s, EV_ON, f, f + 10000);
        TEST_ASSERT_DOUBLE_WITHIN(sim_angle_us(3000, 100), sim_angle_us(3000, 6000), on - f);
    }
}

/* T-222: kill switch: breaker held CLOSED (primary shorted, magneto cut-out
 * convention) and never opened while active; normal operation after release. */
static void t222_kill_holds_closed(void) {
    double cyc = 120e6 / 3000;
    double t = sim_cycles(s, 50000, 3000, 30, LEAD, TRAIL);
    double k0 = 50000 + 10 * cyc + 7000, k1 = k0 + 300000;
    sim_kill_at(s, k0, true); sim_kill_at(s, k1, false);
    TEST_ASSERT_TRUE(sim_run_until(s, t));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_FIRE, k0 + 20000 + cyc, k1));
    TEST_ASSERT_EQUAL_INT(0, sim_count(s, EV_SOFT, 0, t));
    TEST_ASSERT_TRUE(sim_count(s, EV_FIRE, k1 + 20000 + cyc, t) >= 5);
}
int main(void) { UNITY_BEGIN(); RUN_TEST(t220_first_kick); RUN_TEST(t221_running); RUN_TEST(t222_kill_holds_closed); return UNITY_END(); }
