/* config.h — persistent configuration (EEPROM image) and validation.
 * Interface frozen by D-012. Field meanings, defaults, limits and the
 * validation order: plan/DECISIONS.md D-012. The compiled variant is selected
 * by -DDEFAULT_VARIANT_TCI or -DDEFAULT_VARIANT_MAGBREAK (D-016); a config
 * whose `variant` differs from the compiled one is invalid (CFG_ERR_VARIANT). */
#ifndef ARIEL_CONFIG_H
#define ARIEL_CONFIG_H
#include "common.h"

#define CFG_MAGIC        0xA351u
#define CFG_VERSION      1u
#define CURVE_POINTS     8u
#define DWELL_POINTS     4u

#define VARIANT_TCI      0u   /* battery: inductive coil, ignition IGBT     */
#define VARIANT_MAGBREAK 1u   /* battery-less: electronic breaker in magneto */

typedef struct {
    uint16_t rpm[CURVE_POINTS];       /* strictly increasing                  */
    uint16_t adv_cdeg[CURVE_POINTS];  /* advance BTDC, cdeg                   */
} curve_t;

typedef struct {
    uint16_t magic;                   /* CFG_MAGIC                             */
    uint8_t  version;                 /* CFG_VERSION                           */
    uint8_t  variant;                 /* VARIANT_TCI | VARIANT_MAGBREAK        */
    uint8_t  cycle_div;               /* 2: one trigger per 720 deg crank; 1: per 360 */
    uint8_t  active_map;              /* 0 or 1                                */
    uint8_t  crank_exit_cycles;       /* consecutive cycles >= crank_exit_rpm  */
    uint8_t  reserved0;               /* must be 0                             */
    uint16_t lead_cdeg;               /* physical angle BTDC of lead edge      */
    uint16_t trail_cdeg;              /* physical angle BTDC of trail edge     */
    int16_t  trim_cdeg;               /* static trim, -500..+500               */
    uint16_t sensor_latency_us;       /* 0..200                                */
    uint16_t crank_exit_rpm;
    uint16_t crank_enter_rpm;
    uint16_t rev_limit_rpm;
    uint16_t rev_resume_rpm;
    uint16_t adv_min_cdeg;
    uint16_t adv_max_cdeg;
    uint16_t dwell_mv[DWELL_POINTS];  /* strictly increasing battery mV        */
    uint16_t dwell_us[DWELL_POINTS];  /* dwell at that voltage                 */
    uint16_t dwell_max_us;            /* run-mode dwell ceiling                */
    uint16_t dwell_crank_us;          /* crank-mode target dwell               */
    uint16_t crank_dwell_max_us;      /* crank-mode abort threshold            */
    uint16_t ssd_hold_ms;             /* TCI: after a soft shutdown, no new dwell
                                         for this long (gate RC discharge)     */
    uint16_t stall_timeout_ms;        /* no trigger for this long => stopped   */
    uint16_t vbat_min_mv;             /* no new dwell below this               */
    uint16_t magbreak_open_cdeg;      /* MAGBREAK: breaker-open duration       */
    curve_t  maps[2];
    uint16_t crc;                     /* crc16_ccitt over all preceding bytes  */
} config_t;

typedef enum {
    CFG_OK = 0,
    CFG_ERR_MAGIC,
    CFG_ERR_VERSION,
    CFG_ERR_CRC,
    CFG_ERR_VARIANT,
    CFG_ERR_CYCLE_DIV,
    CFG_ERR_MAP,
    CFG_ERR_ANGLES,       /* lead/trail/trim/adv limits inconsistent           */
    CFG_ERR_CURVE,        /* rpm not strictly increasing or adv out of range   */
    CFG_ERR_CRANK,        /* crank thresholds inconsistent                     */
    CFG_ERR_REVLIMIT,
    CFG_ERR_DWELL,
    CFG_ERR_TIMEOUTS,
    CFG_ERR_VBAT,
    CFG_ERR_RESERVED
} cfg_err_t;

void      config_defaults(config_t *c);                 /* fills + seals       */
cfg_err_t config_validate(const config_t *c);           /* semantic checks only (not crc) */
void      config_seal(config_t *c);                     /* sets crc            */
cfg_err_t config_check_image(const config_t *c);        /* magic, version, crc, then validate */
/* Copy image to dst if config_check_image()==CFG_OK and return true;
 * otherwise load defaults into dst and return false (safe defaults). */
bool      config_load_or_default(config_t *dst, const config_t *image);
#endif
