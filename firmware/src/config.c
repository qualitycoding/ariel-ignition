/* Configuration defaults, validation and EEPROM image checks (D-012). */
#include <string.h>
#include "config.h"
#include "crc16.h"

#if defined(DEFAULT_VARIANT_MAGBREAK)
#define COMPILED_VARIANT VARIANT_MAGBREAK
#elif defined(DEFAULT_VARIANT_TCI)
#define COMPILED_VARIANT VARIANT_TCI
#else
#error "define DEFAULT_VARIANT_TCI or DEFAULT_VARIANT_MAGBREAK (D-016)"
#endif

static const uint16_t k_rpm[CURVE_POINTS] = {500, 800, 1200, 1800, 2400, 3000, 3600, 4500};
static const uint16_t k_adv0[CURVE_POINTS] = {0, 600, 1200, 2000, 2600, 3000, 3400, 3400};
static const uint16_t k_adv1[CURVE_POINTS] = {0, 200, 800, 1600, 2200, 2600, 3000, 3000};
static const uint16_t k_dwell_mv[DWELL_POINTS] = {4500, 5500, 6500, 7500};
static const uint16_t k_dwell_us[DWELL_POINTS] = {6000, 4500, 3500, 3000};

void config_seal(config_t *c)
{
    c->crc = crc16_ccitt((const uint8_t *)c, offsetof(config_t, crc));
}

void config_defaults(config_t *c)
{
    memset(c, 0, sizeof *c);
    c->magic = CFG_MAGIC;
    c->version = CFG_VERSION;
    c->variant = COMPILED_VARIANT;
    c->cycle_div = 2;
    c->active_map = 0;
    c->crank_exit_cycles = 3;
    c->lead_cdeg = 5000;
    c->trail_cdeg = 0;
    c->trim_cdeg = 0;
    c->sensor_latency_us = 0;
    c->crank_exit_rpm = 600;
    c->crank_enter_rpm = 450;
    c->rev_limit_rpm = 6000;
    c->rev_resume_rpm = 5700;
    c->adv_min_cdeg = 0;
    c->adv_max_cdeg = 3800;
    memcpy(c->dwell_mv, k_dwell_mv, sizeof k_dwell_mv);
    memcpy(c->dwell_us, k_dwell_us, sizeof k_dwell_us);
    c->dwell_max_us = 8000;
    c->dwell_crank_us = 10000;
    c->crank_dwell_max_us = 50000;
    c->ssd_hold_ms = 120;
    c->stall_timeout_ms = 1500;
    c->vbat_min_mv = 4500;
    c->magbreak_open_cdeg = 6000;
    memcpy(c->maps[0].rpm, k_rpm, sizeof k_rpm);
    memcpy(c->maps[0].adv_cdeg, k_adv0, sizeof k_adv0);
    memcpy(c->maps[1].rpm, k_rpm, sizeof k_rpm);
    memcpy(c->maps[1].adv_cdeg, k_adv1, sizeof k_adv1);
    config_seal(c);
}

static bool curve_ok(const curve_t *cv, uint16_t adv_min, uint16_t adv_max)
{
    if (cv->rpm[0] < 100u) return false;
    for (uint8_t i = 0; i < CURVE_POINTS; i++) {
        if (i && cv->rpm[i] <= cv->rpm[i - 1]) return false;
        if (cv->adv_cdeg[i] < adv_min || cv->adv_cdeg[i] > adv_max) return false;
    }
    return true;
}

cfg_err_t config_validate(const config_t *c)
{
    if (c->reserved0 != 0) return CFG_ERR_RESERVED;
    if (c->variant != COMPILED_VARIANT) return CFG_ERR_VARIANT;
    if (c->cycle_div != 1 && c->cycle_div != 2) return CFG_ERR_CYCLE_DIV;
    if (c->active_map > 1) return CFG_ERR_MAP;

    /* ANGLES */
    if (c->lead_cdeg < 1000u || c->lead_cdeg > 6000u) return CFG_ERR_ANGLES;
    if (c->trail_cdeg > 500u) return CFG_ERR_ANGLES;
    if ((int32_t)c->lead_cdeg - (int32_t)c->trail_cdeg < 1000) return CFG_ERR_ANGLES;
    if (c->trim_cdeg < -500 || c->trim_cdeg > 500) return CFG_ERR_ANGLES;
    if (c->sensor_latency_us > 200u) return CFG_ERR_ANGLES;
    if (c->adv_min_cdeg > c->adv_max_cdeg) return CFG_ERR_ANGLES;
    {
        uint16_t ceiling = (uint16_t)(c->lead_cdeg - 500u);
        if (ceiling > 4500u) ceiling = 4500u;
        if (c->adv_max_cdeg > ceiling) return CFG_ERR_ANGLES;
    }
    if (c->magbreak_open_cdeg < 2000u || c->magbreak_open_cdeg > 12000u) return CFG_ERR_ANGLES;

    /* CURVE */
    if (!curve_ok(&c->maps[0], c->adv_min_cdeg, c->adv_max_cdeg)) return CFG_ERR_CURVE;
    if (!curve_ok(&c->maps[1], c->adv_min_cdeg, c->adv_max_cdeg)) return CFG_ERR_CURVE;

    /* CRANK */
    if (c->crank_exit_rpm < 300u || c->crank_exit_rpm > 1500u) return CFG_ERR_CRANK;
    if (c->crank_enter_rpm >= c->crank_exit_rpm) return CFG_ERR_CRANK;
    if (c->crank_exit_cycles < 1u || c->crank_exit_cycles > 10u) return CFG_ERR_CRANK;

    /* REVLIMIT */
    if (c->rev_limit_rpm < 2000u || c->rev_limit_rpm > 8000u) return CFG_ERR_REVLIMIT;
    if (c->rev_resume_rpm >= c->rev_limit_rpm) return CFG_ERR_REVLIMIT;

    /* DWELL */
    for (uint8_t i = 0; i < DWELL_POINTS; i++) {
        if (i && c->dwell_mv[i] <= c->dwell_mv[i - 1]) return CFG_ERR_DWELL;
        if (c->dwell_us[i] < 500u || c->dwell_us[i] > 8000u) return CFG_ERR_DWELL;
    }
    if (c->dwell_max_us > 15000u) return CFG_ERR_DWELL;
    if (c->dwell_crank_us < 1000u) return CFG_ERR_DWELL;
    if (c->crank_dwell_max_us > 50000u || c->crank_dwell_max_us < c->dwell_crank_us) return CFG_ERR_DWELL;

    /* TIMEOUTS */
    if (c->ssd_hold_ms < 100u || c->ssd_hold_ms > 500u) return CFG_ERR_TIMEOUTS;
    if (c->stall_timeout_ms < 500u || c->stall_timeout_ms > 5000u) return CFG_ERR_TIMEOUTS;

    /* VBAT */
    if (c->vbat_min_mv < 3500u || c->vbat_min_mv > 11000u) return CFG_ERR_VBAT;
    return CFG_OK;
}

cfg_err_t config_check_image(const config_t *c)
{
    if (c->magic != CFG_MAGIC) return CFG_ERR_MAGIC;
    if (c->version != CFG_VERSION) return CFG_ERR_VERSION;
    if (c->crc != crc16_ccitt((const uint8_t *)c, offsetof(config_t, crc))) return CFG_ERR_CRC;
    return config_validate(c);
}

bool config_load_or_default(config_t *dst, const config_t *image)
{
    if (config_check_image(image) == CFG_OK) {
        if (dst != image) memcpy(dst, image, sizeof *dst);
        return true;
    }
    config_defaults(dst);
    return false;
}
