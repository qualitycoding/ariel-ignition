/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-001 CRC-16/CCITT-FALSE. Enforces: D-011, C-020. Category: unit. */
#include "unity.h"
#include "crc16.h"
void setUp(void) {} void tearDown(void) {}
static void t001_check_value(void) {
    const uint8_t s[] = "123456789";
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt(s, 9));
}
static void t001_empty_is_init(void) { TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_ccitt((const uint8_t *)"", 0)); }
static void t001_single_byte(void) { const uint8_t a = 'A'; TEST_ASSERT_EQUAL_HEX16(0xB915, crc16_ccitt(&a, 1)); }
int main(void) { UNITY_BEGIN(); RUN_TEST(t001_check_value); RUN_TEST(t001_empty_is_init); RUN_TEST(t001_single_byte); return UNITY_END(); }
