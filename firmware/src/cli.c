/* Serial tuning interpreter (D-014). No printf: bounded formatter only. */
#include <string.h>
#include "cli.h"

#define MAX_TOK 6u

typedef struct { char *p; size_t cap; size_t n; } obuf_t;   /* cap = out_len - 1 */

static void ob_c(obuf_t *o, char ch) { if (o->n < o->cap) o->p[o->n++] = ch; }
static void ob_s(obuf_t *o, const char *s) { while (*s) ob_c(o, *s++); }
static void ob_i(obuf_t *o, int32_t v)
{
    char tmp[12]; uint8_t k = 0;
    uint32_t u = v < 0 ? (uint32_t)(-(int64_t)v) : (uint32_t)v;
    if (v < 0) ob_c(o, '-');
    do { tmp[k++] = (char)('0' + (u % 10u)); u /= 10u; } while (u);
    while (k) ob_c(o, tmp[--k]);
}

typedef struct { const char *s; uint8_t n; } tok_t;

static bool tok_is(const tok_t *t, const char *name)
{
    uint8_t i = 0;
    while (name[i]) { if (i >= t->n || t->s[i] != name[i]) return false; i++; }
    return i == t->n;
}

/* number: optional '-' then >=1 digits. Returns SYNTAX, RANGE (too large for
 * any field) or OK. */
static cli_status_t parse_num(const tok_t *t, int32_t *out)
{
    uint8_t i = 0; bool neg = false, big = false; int32_t v = 0;
    if (t->n && t->s[0] == '-') { neg = true; i = 1; }
    if (i >= t->n) return CLI_ERR_SYNTAX;
    for (; i < t->n; i++) {
        char ch = t->s[i];
        if (ch < '0' || ch > '9') return CLI_ERR_SYNTAX;
        if (!big) { v = v * 10 + (ch - '0'); if (v > 100000000L) big = true; }
    }
    if (big) return CLI_ERR_RANGE;
    *out = neg ? -v : v;
    return CLI_OK;
}

typedef enum { K_U8, K_U16, K_I16 } kind_t;
typedef struct { const char *name; uint8_t off; kind_t kind; } key_t_;

#define KEY(nm, field, kd) { nm, (uint8_t)offsetof(config_t, field), kd }
static const key_t_ k_keys[] = {
    KEY("map", active_map, K_U8), KEY("cyclediv", cycle_div, K_U8),
    KEY("crankcycles", crank_exit_cycles, K_U8),
    KEY("lead", lead_cdeg, K_U16), KEY("trail", trail_cdeg, K_U16), KEY("trim", trim_cdeg, K_I16),
    KEY("latency", sensor_latency_us, K_U16),
    KEY("crankexit", crank_exit_rpm, K_U16), KEY("crankenter", crank_enter_rpm, K_U16),
    KEY("revlimit", rev_limit_rpm, K_U16), KEY("revresume", rev_resume_rpm, K_U16),
    KEY("advmin", adv_min_cdeg, K_U16), KEY("advmax", adv_max_cdeg, K_U16),
    KEY("dwellmax", dwell_max_us, K_U16), KEY("dwellcrank", dwell_crank_us, K_U16),
    KEY("crankdwellmax", crank_dwell_max_us, K_U16),
    KEY("ssdhold", ssd_hold_ms, K_U16), KEY("stall", stall_timeout_ms, K_U16),
    KEY("vbatmin", vbat_min_mv, K_U16), KEY("magopen", magbreak_open_cdeg, K_U16),
};
#define NKEYS (sizeof k_keys / sizeof k_keys[0])

static const key_t_ *find_key(const tok_t *t)
{
    for (uint8_t i = 0; i < NKEYS; i++) if (tok_is(t, k_keys[i].name)) return &k_keys[i];
    return NULL;
}

static bool in_range(kind_t k, int32_t v)
{
    switch (k) {
    case K_U8:  return v >= 0 && v <= 255;
    case K_U16: return v >= 0 && v <= 65535;
    default:    return v >= -32768 && v <= 32767;
    }
}

static int32_t key_read(const config_t *c, const key_t_ *k)
{
    const uint8_t *p = (const uint8_t *)c + k->off;
    if (k->kind == K_U8) return *p;
    if (k->kind == K_U16) { uint16_t u; memcpy(&u, p, 2); return u; }
    { int16_t s; memcpy(&s, p, 2); return s; }
}

static void key_write(config_t *c, const key_t_ *k, int32_t v)
{
    uint8_t *p = (uint8_t *)c + k->off;
    if (k->kind == K_U8) *p = (uint8_t)v;
    else if (k->kind == K_U16) { uint16_t u = (uint16_t)v; memcpy(p, &u, 2); }
    else { int16_t s = (int16_t)v; memcpy(p, &s, 2); }
}

static bool is_mutating(const tok_t *t)
{
    return tok_is(t, "set") || tok_is(t, "curve") || tok_is(t, "dwell") ||
           tok_is(t, "save") || tok_is(t, "defaults");
}

static cli_status_t run(const tok_t *t, uint8_t nt, config_t *w, uint16_t rpm,
                        obuf_t *ob, bool *save)
{
    int32_t a, b, c, d;
    cli_status_t st;
    if (nt == 0) return CLI_ERR_SYNTAX;

    if (tok_is(&t[0], "set") || tok_is(&t[0], "get") || tok_is(&t[0], "curve") ||
        tok_is(&t[0], "dwell") || tok_is(&t[0], "save") || tok_is(&t[0], "defaults") ||
        tok_is(&t[0], "help")) {
        if (is_mutating(&t[0]) && rpm > 0) return CLI_ERR_BUSY;
    } else {
        return CLI_ERR_UNKNOWN;
    }

    if (tok_is(&t[0], "help")) {
        if (nt != 1) return CLI_ERR_SYNTAX;
        ob_s(ob, "set get curve dwell save defaults status");
        return CLI_OK;
    }
    if (tok_is(&t[0], "defaults")) {
        if (nt != 1) return CLI_ERR_SYNTAX;
        config_defaults(w);
        ob_s(ob, "OK");
        return CLI_OK;
    }
    if (tok_is(&t[0], "save")) {
        if (nt != 1) return CLI_ERR_SYNTAX;
        if (config_validate(w) != CFG_OK) return CLI_ERR_INVALID_CONFIG;
        config_seal(w);
        *save = true;
        ob_s(ob, "OK");
        return CLI_OK;
    }
    if (tok_is(&t[0], "set")) {
        const key_t_ *k;
        if (nt != 3) return CLI_ERR_SYNTAX;
        k = find_key(&t[1]);
        if (!k) return CLI_ERR_UNKNOWN;
        if ((st = parse_num(&t[2], &a)) != CLI_OK) return st;
        if (!in_range(k->kind, a)) return CLI_ERR_RANGE;
        key_write(w, k, a);
        ob_s(ob, "OK");
        return CLI_OK;
    }
    if (tok_is(&t[0], "curve")) {
        if (nt != 5) return CLI_ERR_SYNTAX;
        if ((st = parse_num(&t[1], &a)) != CLI_OK) return st;
        if ((st = parse_num(&t[2], &b)) != CLI_OK) return st;
        if ((st = parse_num(&t[3], &c)) != CLI_OK) return st;
        if ((st = parse_num(&t[4], &d)) != CLI_OK) return st;
        if (a < 0 || a > 1 || b < 0 || b >= (int32_t)CURVE_POINTS ||
            !in_range(K_U16, c) || !in_range(K_U16, d)) return CLI_ERR_RANGE;
        w->maps[a].rpm[b] = (uint16_t)c;
        w->maps[a].adv_cdeg[b] = (uint16_t)d;
        ob_s(ob, "OK");
        return CLI_OK;
    }
    if (tok_is(&t[0], "dwell")) {
        if (nt != 4) return CLI_ERR_SYNTAX;
        if ((st = parse_num(&t[1], &a)) != CLI_OK) return st;
        if ((st = parse_num(&t[2], &b)) != CLI_OK) return st;
        if ((st = parse_num(&t[3], &c)) != CLI_OK) return st;
        if (a < 0 || a >= (int32_t)DWELL_POINTS || !in_range(K_U16, b) || !in_range(K_U16, c))
            return CLI_ERR_RANGE;
        w->dwell_mv[a] = (uint16_t)b;
        w->dwell_us[a] = (uint16_t)c;
        ob_s(ob, "OK");
        return CLI_OK;
    }
    /* get */
    if (nt < 2 || nt > 3) return CLI_ERR_SYNTAX;
    if (tok_is(&t[1], "curve")) {
        if (nt != 3) return CLI_ERR_SYNTAX;
        if ((st = parse_num(&t[2], &a)) != CLI_OK) return st;
        if (a < 0 || a > 1) return CLI_ERR_RANGE;
        ob_s(ob, "curve"); ob_i(ob, a);
        for (uint8_t i = 0; i < CURVE_POINTS; i++) {
            ob_c(ob, ' '); ob_i(ob, w->maps[a].rpm[i]); ob_c(ob, ':'); ob_i(ob, w->maps[a].adv_cdeg[i]);
        }
        return CLI_OK;
    }
    if (tok_is(&t[1], "dwell")) {
        if (nt != 2) return CLI_ERR_SYNTAX;
        ob_s(ob, "dwell");
        for (uint8_t i = 0; i < DWELL_POINTS; i++) {
            ob_c(ob, ' '); ob_i(ob, w->dwell_mv[i]); ob_c(ob, ':'); ob_i(ob, w->dwell_us[i]);
        }
        return CLI_OK;
    }
    {
        const key_t_ *k = find_key(&t[1]);
        if (nt != 2) return CLI_ERR_SYNTAX;
        if (!k) return CLI_ERR_UNKNOWN;
        ob_s(ob, k->name); ob_c(ob, '='); ob_i(ob, key_read(w, k));
        return CLI_OK;
    }
}

cli_status_t cli_exec(const char *line, config_t *working, uint16_t rpm,
                      char *out, size_t out_len, bool *save_requested)
{
    obuf_t ob;
    tok_t tk[MAX_TOK];
    uint8_t nt = 0;
    size_t len, i = 0;
    cli_status_t st;
    bool save = false;

    ob.p = out; ob.n = 0; ob.cap = out_len ? out_len - 1 : 0;
    if (save_requested) *save_requested = false;

    len = strlen(line);
    while (len && (line[len - 1] == '\r' || line[len - 1] == '\n')) len--;
    if (len > CLI_MAX_LINE) {
        st = CLI_ERR_TOOLONG;
    } else {
        while (i < len) {
            while (i < len && line[i] == ' ') i++;
            if (i >= len) break;
            if (nt == MAX_TOK) { nt = (uint8_t)(MAX_TOK + 1u); break; }
            tk[nt].s = line + i;
            { size_t j = i; while (j < len && line[j] != ' ') j++; tk[nt].n = (uint8_t)(j - i); i = j; }
            nt++;
        }
        st = (nt > MAX_TOK) ? CLI_ERR_SYNTAX : run(tk, nt, working, rpm, &ob, &save);
    }
    if (st != CLI_OK) {
        ob.n = 0;
        ob_s(&ob, "ERR "); ob_i(&ob, (int32_t)st);
    } else if (save_requested) {
        *save_requested = save;
    }
    if (out_len) out[ob.n] = 0;
    return st;
}
