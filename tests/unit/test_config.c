/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-002..T-004 configuration defaults, validation, image loading.
 * Enforces: SC-4 (programmable, persistent), SC-7 (safe defaults on corrupt
 * storage), D-012. Categories: unit, operational (T-004). */
#include <string.h>
#include <stddef.h>
#include "unity.h"
#include "config.h"
#include "crc16.h"
void setUp(void) {} void tearDown(void) {}
static config_t c;

/* T-002: defaults are exactly the D-012 table and are valid + sealed. */
static void t002_defaults_table(void) {
    static const uint16_t rpm[8] = {500,800,1200,1800,2400,3000,3600,4500};
    static const uint16_t a0[8]  = {0,600,1200,2000,2600,3000,3400,3400};
    static const uint16_t a1[8]  = {0,200,800,1600,2200,2600,3000,3000};
    config_defaults(&c);
    TEST_ASSERT_EQUAL_HEX16(CFG_MAGIC, c.magic);
    TEST_ASSERT_EQUAL_UINT8(CFG_VERSION, c.version);
    TEST_ASSERT_EQUAL_UINT8(VARIANT_TCI, c.variant);      /* host build = TCI */
    TEST_ASSERT_EQUAL_UINT8(2, c.cycle_div);
    TEST_ASSERT_EQUAL_UINT8(0, c.active_map);
    TEST_ASSERT_EQUAL_UINT8(3, c.crank_exit_cycles);
    TEST_ASSERT_EQUAL_UINT8(0, c.reserved0);
    TEST_ASSERT_EQUAL_UINT16(5000, c.lead_cdeg);
    TEST_ASSERT_EQUAL_UINT16(0, c.trail_cdeg);
    TEST_ASSERT_EQUAL_INT16(0, c.trim_cdeg);
    TEST_ASSERT_EQUAL_UINT16(0, c.sensor_latency_us);
    TEST_ASSERT_EQUAL_UINT16(600, c.crank_exit_rpm);
    TEST_ASSERT_EQUAL_UINT16(450, c.crank_enter_rpm);
    TEST_ASSERT_EQUAL_UINT16(6000, c.rev_limit_rpm);
    TEST_ASSERT_EQUAL_UINT16(5700, c.rev_resume_rpm);
    TEST_ASSERT_EQUAL_UINT16(0, c.adv_min_cdeg);
    TEST_ASSERT_EQUAL_UINT16(3800, c.adv_max_cdeg);
    { static const uint16_t mv[4] = {4500,5500,6500,7500}, us[4] = {6000,4500,3500,3000};
      TEST_ASSERT_EQUAL_UINT16_ARRAY(mv, c.dwell_mv, 4); TEST_ASSERT_EQUAL_UINT16_ARRAY(us, c.dwell_us, 4); }
    TEST_ASSERT_EQUAL_UINT16(8000, c.dwell_max_us);
    TEST_ASSERT_EQUAL_UINT16(10000, c.dwell_crank_us);
    TEST_ASSERT_EQUAL_UINT16(50000, c.crank_dwell_max_us);
    TEST_ASSERT_EQUAL_UINT16(120, c.ssd_hold_ms);
    TEST_ASSERT_EQUAL_UINT16(1500, c.stall_timeout_ms);
    TEST_ASSERT_EQUAL_UINT16(4500, c.vbat_min_mv);
    TEST_ASSERT_EQUAL_UINT16(6000, c.magbreak_open_cdeg);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(rpm, c.maps[0].rpm, 8);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(a0,  c.maps[0].adv_cdeg, 8);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(rpm, c.maps[1].rpm, 8);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(a1,  c.maps[1].adv_cdeg, 8);
    TEST_ASSERT_EQUAL_INT(CFG_OK, config_validate(&c));
    TEST_ASSERT_EQUAL_INT(CFG_OK, config_check_image(&c));
    TEST_ASSERT_EQUAL_HEX16(crc16_ccitt((const uint8_t *)&c, offsetof(config_t, crc)), c.crc);
}

/* T-003: each validation rule, in the D-012 order. */
#define EXPECT(err, mutation) do { config_defaults(&c); mutation;     TEST_ASSERT_EQUAL_INT_MESSAGE(err, config_validate(&c), #mutation); } while (0)
static void t003_validation_rules(void) {
    EXPECT(CFG_ERR_RESERVED, c.reserved0 = 1);
    EXPECT(CFG_ERR_VARIANT,  c.variant = VARIANT_MAGBREAK);
    EXPECT(CFG_ERR_VARIANT,  c.variant = 7);
    EXPECT(CFG_ERR_CYCLE_DIV, c.cycle_div = 0);
    EXPECT(CFG_ERR_CYCLE_DIV, c.cycle_div = 3);
    EXPECT(CFG_OK,           c.cycle_div = 1);
    EXPECT(CFG_ERR_MAP,      c.active_map = 2);
    EXPECT(CFG_ERR_ANGLES,   c.lead_cdeg = 999);
    EXPECT(CFG_ERR_ANGLES,   c.lead_cdeg = 6001);
    EXPECT(CFG_ERR_ANGLES,   c.trail_cdeg = 501);
    EXPECT(CFG_ERR_ANGLES,   c.trim_cdeg = 501);
    EXPECT(CFG_ERR_ANGLES,   c.trim_cdeg = -501);
    EXPECT(CFG_OK,           c.trim_cdeg = -500);
    EXPECT(CFG_ERR_ANGLES,   c.sensor_latency_us = 201);
    EXPECT(CFG_ERR_ANGLES,   (c.adv_min_cdeg = 1000, c.maps[0].adv_cdeg[0] = 1000, c.maps[1].adv_cdeg[0] = 1000, c.adv_max_cdeg = 900));
    EXPECT(CFG_ERR_ANGLES,   c.adv_max_cdeg = 4501);            /* > 45 deg and > lead - 500 */
    EXPECT(CFG_OK,           c.adv_max_cdeg = 4500);
    EXPECT(CFG_ERR_ANGLES,   (c.lead_cdeg = 4000, c.adv_max_cdeg = 3501));  /* > lead - 500 */
    EXPECT(CFG_OK,           (c.lead_cdeg = 4000, c.adv_max_cdeg = 3500));
    EXPECT(CFG_ERR_ANGLES,   (c.lead_cdeg = 1400, c.adv_max_cdeg = 900, c.maps[0].adv_cdeg[7] = 900, c.maps[0].adv_cdeg[6] = 900, c.maps[0].adv_cdeg[5] = 900, c.maps[0].adv_cdeg[4] = 900, c.maps[0].adv_cdeg[3] = 900, c.maps[1].adv_cdeg[7] = 900, c.maps[1].adv_cdeg[6] = 900, c.maps[1].adv_cdeg[5] = 900, c.maps[1].adv_cdeg[4] = 900, c.maps[1].adv_cdeg[3] = 900, c.trail_cdeg = 500)); /* lead - trail < 1000 */
    EXPECT(CFG_ERR_ANGLES,   (c.lead_cdeg = 6000, c.adv_max_cdeg = 4501));  /* > 45 deg */
    EXPECT(CFG_ERR_ANGLES,   c.magbreak_open_cdeg = 1999);
    EXPECT(CFG_ERR_ANGLES,   c.magbreak_open_cdeg = 12001);
    EXPECT(CFG_ERR_CURVE,    c.maps[0].rpm[3] = c.maps[0].rpm[2]);
    EXPECT(CFG_ERR_CURVE,    c.maps[1].rpm[7] = 3000);
    EXPECT(CFG_ERR_CURVE,    c.maps[0].rpm[0] = 99);
    EXPECT(CFG_ERR_CURVE,    c.maps[1].adv_cdeg[5] = 3801);          /* > adv_max */
    EXPECT(CFG_ERR_CURVE,    c.maps[0].adv_cdeg[1] = 0xFFFF);
    EXPECT(CFG_ERR_CRANK,    c.crank_enter_rpm = 600);
    EXPECT(CFG_ERR_CRANK,    (c.crank_exit_rpm = 299, c.crank_enter_rpm = 200));
    EXPECT(CFG_ERR_CRANK,    c.crank_exit_rpm = 1501);
    EXPECT(CFG_ERR_CRANK,    c.crank_exit_cycles = 0);
    EXPECT(CFG_ERR_CRANK,    c.crank_exit_cycles = 11);
    EXPECT(CFG_ERR_REVLIMIT, c.rev_resume_rpm = 6000);
    EXPECT(CFG_ERR_REVLIMIT, (c.rev_limit_rpm = 8001));
    EXPECT(CFG_ERR_REVLIMIT, (c.rev_limit_rpm = 1999, c.rev_resume_rpm = 1500));
    EXPECT(CFG_ERR_DWELL,    c.dwell_mv[2] = 5500);
    EXPECT(CFG_ERR_DWELL,    c.dwell_us[0] = 499);
    EXPECT(CFG_ERR_DWELL,    c.dwell_us[0] = 8001);
    EXPECT(CFG_ERR_DWELL,    c.dwell_max_us = 15001);
    EXPECT(CFG_ERR_DWELL,    c.dwell_crank_us = 999);
    EXPECT(CFG_ERR_DWELL,    c.crank_dwell_max_us = 50001);
    EXPECT(CFG_ERR_DWELL,    (c.crank_dwell_max_us = 9000));  /* < dwell_crank */
    EXPECT(CFG_ERR_TIMEOUTS, c.ssd_hold_ms = 99);
    EXPECT(CFG_ERR_TIMEOUTS, c.ssd_hold_ms = 501);
    EXPECT(CFG_ERR_TIMEOUTS, c.stall_timeout_ms = 499);
    EXPECT(CFG_ERR_TIMEOUTS, c.stall_timeout_ms = 5001);
    EXPECT(CFG_ERR_VBAT,     c.vbat_min_mv = 3499);
    EXPECT(CFG_ERR_VBAT,     c.vbat_min_mv = 11001);
}

/* T-004 (operational): corrupt / foreign images fall back to defaults. */
static void t004_image_loading(void) {
    config_t img, out, def;
    config_defaults(&def);
    img = def; img.trim_cdeg = 150; config_seal(&img);
    TEST_ASSERT_EQUAL_INT(CFG_OK, config_check_image(&img));
    TEST_ASSERT_TRUE(config_load_or_default(&out, &img));
    TEST_ASSERT_EQUAL_MEMORY(&img, &out, sizeof img);

    img.trim_cdeg = 151;                       /* bit rot after sealing */
    TEST_ASSERT_EQUAL_INT(CFG_ERR_CRC, config_check_image(&img));
    TEST_ASSERT_FALSE(config_load_or_default(&out, &img));
    TEST_ASSERT_EQUAL_MEMORY(&def, &out, sizeof def);

    memset(&img, 0xFF, sizeof img);            /* blank EEPROM */
    TEST_ASSERT_EQUAL_INT(CFG_ERR_MAGIC, config_check_image(&img));
    TEST_ASSERT_FALSE(config_load_or_default(&out, &img));
    TEST_ASSERT_EQUAL_MEMORY(&def, &out, sizeof def);

    img = def; img.version = 2; config_seal(&img);
    TEST_ASSERT_EQUAL_INT(CFG_ERR_VERSION, config_check_image(&img));

    img = def; img.lead_cdeg = 900; config_seal(&img);    /* sealed but invalid */
    TEST_ASSERT_EQUAL_INT(CFG_ERR_ANGLES, config_check_image(&img));
    TEST_ASSERT_FALSE(config_load_or_default(&out, &img));
    TEST_ASSERT_EQUAL_MEMORY(&def, &out, sizeof def);
}
int main(void) { UNITY_BEGIN(); RUN_TEST(t002_defaults_table); RUN_TEST(t003_validation_rules); RUN_TEST(t004_image_loading); return UNITY_END(); }
