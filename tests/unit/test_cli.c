/* FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). */
/* T-011..T-016 serial tuning interpreter (firmware/include/cli.h, D-014).
 * Enforces: SC-3 (programmable), SC-6 (no tuning while engine turns),
 * threat model TM-1/TM-2 (malformed input). Categories: unit, security (T-015, T-016). */
#include <string.h>
#include "unity.h"
#include "cli.h"
void setUp(void) {} void tearDown(void) {}
static config_t w, before;
static char out[128];
static bool save;
static cli_status_t run(const char *line, uint16_t rpm) {
    memset(out, 0x5A, sizeof out); save = true;           /* poison */
    return cli_exec(line, &w, rpm, out, sizeof out, &save);
}

/* T-011: every scalar key round-trips through set/get and replies "OK". */
static void t011_set_get_keys(void) {
    static const struct { const char *key; long v; } k[] = {
        {"map",1},{"cyclediv",1},{"crankcycles",5},{"lead",4800},{"trail",200},{"trim",-250},
        {"latency",30},{"crankexit",700},{"crankenter",500},{"revlimit",5500},{"revresume",5200},
        {"advmin",100},{"advmax",3700},{"dwellmax",7000},{"dwellcrank",9000},{"crankdwellmax",40000},
        {"ssdhold",200},{"stall",1200},{"vbatmin",5000},{"magopen",5000} };
    char line[64], expect[48];
    for (unsigned i = 0; i < sizeof k / sizeof k[0]; i++) {
        config_defaults(&w);
        snprintf(line, sizeof line, "set %s %ld", k[i].key, k[i].v);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLI_OK, run(line, 0), line);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("OK", out, line);
        TEST_ASSERT_FALSE(save);
        snprintf(line, sizeof line, "get %s", k[i].key);
        snprintf(expect, sizeof expect, "%s=%ld", k[i].key, k[i].v);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLI_OK, run(line, 0), line);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expect, out, line);
    }
    config_defaults(&w);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("set trim -250", 0)); TEST_ASSERT_EQUAL_INT16(-250, w.trim_cdeg);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("set lead 4800\r\n", 0)); TEST_ASSERT_EQUAL_UINT16(4800, w.lead_cdeg);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("  set   latency   30  ", 0)); TEST_ASSERT_EQUAL_UINT16(30, w.sensor_latency_us);
}

/* T-012: curve and dwell table editing and readback formats. */
static void t012_tables(void) {
    config_defaults(&w);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("curve 1 7 4800 3100", 0));
    TEST_ASSERT_EQUAL_UINT16(4800, w.maps[1].rpm[7]); TEST_ASSERT_EQUAL_UINT16(3100, w.maps[1].adv_cdeg[7]);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("get curve 0", 0));
    TEST_ASSERT_EQUAL_STRING("curve0 500:0 800:600 1200:1200 1800:2000 2400:2600 3000:3000 3600:3400 4500:3400", out);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("dwell 3 7600 2900", 0));
    TEST_ASSERT_EQUAL_UINT16(7600, w.dwell_mv[3]); TEST_ASSERT_EQUAL_UINT16(2900, w.dwell_us[3]);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("get dwell", 0));
    TEST_ASSERT_EQUAL_STRING("dwell 4500:6000 5500:4500 6500:3500 7600:2900", out);
    TEST_ASSERT_EQUAL_INT(CLI_ERR_RANGE, run("curve 2 0 500 0", 0));
    TEST_ASSERT_EQUAL_INT(CLI_ERR_RANGE, run("curve 0 8 500 0", 0));
    TEST_ASSERT_EQUAL_INT(CLI_ERR_RANGE, run("dwell 4 5000 3000", 0));
    TEST_ASSERT_EQUAL_INT(CLI_ERR_SYNTAX, run("curve 0 1 500", 0));
}

/* T-013: engine turning => every mutating command refused, reads allowed. */
static void t013_busy_rule(void) {
    static const char *mut[] = { "set trim 10", "curve 0 0 500 0", "dwell 0 4500 6000", "save", "defaults" };
    for (unsigned i = 0; i < 5; i++) {
        config_defaults(&w); before = w;
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLI_ERR_BUSY, run(mut[i], 1), mut[i]);
        TEST_ASSERT_EQUAL_MEMORY(&before, &w, sizeof w);
        TEST_ASSERT_FALSE(save);
        TEST_ASSERT_EQUAL_STRING("ERR 3", out);
    }
    config_defaults(&w);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("get lead", 4000));
    TEST_ASSERT_EQUAL_STRING("lead=5000", out);
}

/* T-014: save only a valid configuration; defaults restores the table. */
static void t014_save_and_defaults(void) {
    config_t d; config_defaults(&d);
    config_defaults(&w);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("set crankenter 900", 0));        /* now invalid (>= exit) */
    TEST_ASSERT_EQUAL_INT(CLI_ERR_INVALID_CONFIG, run("save", 0));
    TEST_ASSERT_FALSE(save); TEST_ASSERT_EQUAL_STRING("ERR 6", out);
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("set crankexit 1000", 0));
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("save", 0));
    TEST_ASSERT_TRUE(save); TEST_ASSERT_EQUAL_STRING("OK", out);
    TEST_ASSERT_EQUAL_INT(CFG_OK, config_check_image(&w));             /* sealed */
    TEST_ASSERT_EQUAL_INT(CLI_OK, run("defaults", 0));
    TEST_ASSERT_EQUAL_MEMORY(&d, &w, sizeof w); TEST_ASSERT_FALSE(save);
}

/* T-015 (security): malformed input rejected, config untouched, reply "ERR n". */
static void t015_malformed(void) {
    static const struct { const char *l; cli_status_t e; } m[] = {
        {"", CLI_ERR_SYNTAX}, {"   ", CLI_ERR_SYNTAX}, {"set", CLI_ERR_SYNTAX}, {"set lead", CLI_ERR_SYNTAX},
        {"set lead 4800 1", CLI_ERR_SYNTAX}, {"set lead 48x0", CLI_ERR_SYNTAX}, {"set lead +4800", CLI_ERR_SYNTAX},
        {"set lead -", CLI_ERR_SYNTAX}, {"set lead 65536", CLI_ERR_RANGE}, {"set lead -1", CLI_ERR_RANGE},
        {"set map 256", CLI_ERR_RANGE}, {"set trim 32768", CLI_ERR_RANGE}, {"set trim -32769", CLI_ERR_RANGE},
        {"set lead 99999999999999999999", CLI_ERR_RANGE}, {"set nosuch 1", CLI_ERR_UNKNOWN},
        {"get nosuch", CLI_ERR_UNKNOWN}, {"frobnicate", CLI_ERR_UNKNOWN}, {"SET lead 4800", CLI_ERR_UNKNOWN},
        {"set\tlead 4800", CLI_ERR_UNKNOWN},
        {"set lead 1234567890123456789012345678901234567890123456789012345678", CLI_ERR_TOOLONG} };
    char exp[8];
    for (unsigned i = 0; i < sizeof m / sizeof m[0]; i++) {
        config_defaults(&w); before = w;
        TEST_ASSERT_EQUAL_INT_MESSAGE(m[i].e, run(m[i].l, 0), m[i].l);
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&before, &w, sizeof w, m[i].l);
        TEST_ASSERT_FALSE(save);
        snprintf(exp, sizeof exp, "ERR %d", (int)m[i].e);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(exp, out, m[i].l);
    }
}

/* T-016 (security): deterministic fuzz — no overflow of out, no change
 * unless CLI_OK, reply always NUL-terminated within out_len. */
static uint32_t lcg = 12345u;
static uint32_t rnd(void) { lcg = lcg * 1103515245u + 12345u; return lcg >> 8; }
static void t016_fuzz(void) {
    static const char *tok[] = {"set","get","curve","dwell","save","defaults","help","lead","trim","map",
        "-","0","1","7","65535","-32768","99999","x","  ","curve","dwell","\r","\n","4500","advmax"};
    char line[96], small[16]; int oks = 0;
    for (int n = 0; n < 20000; n++) {
        int len = 0, parts = 1 + (int)(rnd() % 6);
        for (int p = 0; p < parts && len < 90; p++) {
            const char *t = tok[rnd() % (sizeof tok / sizeof tok[0])];
            int l = (int)strlen(t); if (len + l + 1 >= 95) break;
            memcpy(line + len, t, (size_t)l); len += l; line[len++] = ' ';
        }
        if (rnd() % 4 == 0 && len < 94) line[len++] = (char)(rnd() % 256 ? rnd() % 256 : 1);
        line[len] = 0;
        config_defaults(&w); before = w;
        memset(small, 0x5A, sizeof small); save = true;
        cli_status_t st = cli_exec(line, &w, (uint16_t)(rnd() % 3), small, 8, &save);
        TEST_ASSERT_TRUE(memchr(small, 0, 8) != NULL);              /* terminated within 8 */
        for (int b = 8; b < 16; b++) TEST_ASSERT_EQUAL_HEX8(0x5A, (uint8_t)small[b]);
        if (st != CLI_OK) { TEST_ASSERT_EQUAL_MEMORY(&before, &w, sizeof w); TEST_ASSERT_FALSE(save); }
        else oks++;
    }
    TEST_ASSERT_TRUE_MESSAGE(oks >= 50, "fuzz corpus produced too few accepted commands");
}
int main(void) { UNITY_BEGIN(); RUN_TEST(t011_set_get_keys); RUN_TEST(t012_tables); RUN_TEST(t013_busy_rule);
    RUN_TEST(t014_save_and_defaults); RUN_TEST(t015_malformed); RUN_TEST(t016_fuzz); return UNITY_END(); }
