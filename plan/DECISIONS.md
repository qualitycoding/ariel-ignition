# Decisions, interfaces and decision rules

Rule-8 note: every ambiguity is resolved here or in ASSUMPTIONS.md. Claims (C-###)
are in `research/claims.json`; risks (R-###) in `premortem/RISK_REGISTER.md`.

## Architecture and component choices

**D-001 Profiles.** `software` with `software.deploys=true` (firmware is flashed
into a vehicle; rollback runbook required). `math`, `computational` and
`publication` do not apply (plan/PROFILE.md).

**D-002 Ignition architectures.** Battery case: **TCI** (inductive coil + ignition
IGBT, MCU timing). Battery-less case: **MAGBREAK** (retained magneto as generator
and HT source, MCU-timed electronic breaker), conditional on spike S-020 and
gate G-003; fallbacks BL-3 (alternator + capacitor + TCI) then BL-2
(self-generating CDI mag-box). Ranking tables: `hardware/BOM.md`. Basis:
C-016, C-017, C-018, C-019.

**D-003 Controller.** Arduino Pro Mini 5 V/16 MHz (ATmega328P). Fuse CKDIV8 on
(2 MHz from reset), firmware sets CLKPR to /2 → **8 MHz** before anything else,
so the chip is within its speed grade down to the 2.7 V brown-out level
(C-023). **No bootloader**, flashed by ISP (USBasp): boot in milliseconds
(needed for battery-less kick starts). On-board regulator unused; +5 V from
U1 (LM2936-5.0, C-025) into VCC. Alternatives ranked in BOM.md.

**D-004 Trigger.** A1220 bipolar Hall latch + two SmCo magnets on an acetal disc
replacing the contact-breaker plate: south at LEAD (50° BTDC), north at TRAIL
(0°, TDC). Falling edge = LEAD, rising edge = TRAIL (C-011). Two physical edges
give a prediction-free cranking spark at TDC and a measured lead→trail segment
for low-speed prediction (D-017).

**D-005 Switch.** FGD3040G2-F085V or ISL9V3040 (C-012, C-013). FGBS3040E1
rejected as obsolete (C-015).

**D-006 Coil.** 6 V: primary 1.2–1.8 Ω; 12 V: 2.8–3.5 Ω; single-output
motorcycle coil. Measured in B-402.

**D-007 System voltage.** Stay 6 V by default; 12 V supported by re-entering the
`dwell` table (no hardware change). Negative earth required (D-021b).

**D-008 Enclosure and size.** Die-cast aluminium ~92×38×31 mm; Q1 on the wall
via thermal pad; sealed connectors; controller electronics fit a 50×35 mm
perfboard (S-005). Potting only after B-408/B-409 pass.

**D-009 Licences.** Firmware and tools Apache-2.0; hardware files CERN-OHL-P-2.0;
vendored Unity MIT (vendor/unity/LICENSE.txt).

## Interfaces (frozen)

**D-010 Units.** time µs (Timer1 tick = 1 µs at 8 MHz/8); angle = centidegrees
(cdeg) of **crankshaft**, positive = before TDC; speed = crank rpm; voltage mV.

**D-011 CRC.** CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection,
xorout 0. Check: "123456789" → 0x29B1; "" → 0xFFFF; "A" → 0xB915 (C-020).
`crc16_ccitt()` in `firmware/include/crc16.h`.

**D-012 Configuration image** (`firmware/include/config.h`). Stored at EEPROM
address 0. `crc` covers all bytes before it (`offsetof(config_t, crc)`).

| Field | Default | Valid range / rule |
|---|---|---|
| magic, version | 0xA351, 1 | exact (checked by `config_check_image`) |
| variant | compiled variant | must equal compiled variant |
| cycle_div | 2 | {1, 2} |
| active_map | 0 | {0, 1} |
| crank_exit_cycles | 3 | 1–10 |
| reserved0 | 0 | 0 |
| lead_cdeg | 5000 | 1000–6000 |
| trail_cdeg | 0 | 0–500; and lead − trail ≥ 1000 |
| trim_cdeg | 0 | −500…500 |
| sensor_latency_us | 0 | 0–200 |
| adv_min_cdeg | 0 | ≤ adv_max |
| adv_max_cdeg | 3800 | ≤ min(lead − 500, 4500) |
| magbreak_open_cdeg | 6000 | 2000–12000 |
| maps[m].rpm[i] | 500,800,1200,1800,2400,3000,3600,4500 | rpm[0] ≥ 100, strictly increasing |
| maps[0].adv_cdeg | 0,600,1200,2000,2600,3000,3400,3400 | adv_min ≤ adv ≤ adv_max |
| maps[1].adv_cdeg | 0,200,800,1600,2200,2600,3000,3000 | same |
| crank_exit_rpm / crank_enter_rpm | 600 / 450 | exit 300–1500; enter < exit |
| rev_limit_rpm / rev_resume_rpm | 6000 / 5700 | limit 2000–8000; resume < limit |
| dwell_mv[4] / dwell_us[4] | 4500,5500,6500,7500 / 6000,4500,3500,3000 | mv strictly increasing; us 500–8000 |
| dwell_max_us | 8000 | ≤ 15000 |
| dwell_crank_us | 10000 | ≥ 1000 |
| crank_dwell_max_us | 50000 | dwell_crank ≤ x ≤ 50000 |
| ssd_hold_ms | 120 | 100–500 |
| stall_timeout_ms | 1500 | 500–5000 |
| vbat_min_mv | 4500 | 3500–11000 |

`config_validate` checks in this order and returns the first failure:
RESERVED, VARIANT, CYCLE_DIV, MAP, ANGLES (lead range, trail range, lead−trail,
trim, latency, adv_min ≤ adv_max, adv_max ceiling, magbreak_open), CURVE (both
maps), CRANK (exit range, enter < exit, cycles), REVLIMIT, DWELL, TIMEOUTS, VBAT.
`config_check_image`: MAGIC, VERSION, CRC, then `config_validate`.
`config_load_or_default`: copy the image only if `config_check_image == CFG_OK`,
else load sealed defaults and return false. Default curve rationale: original
full advance ≈ 38–41° crank (C-002, C-006); map 0 peaks 4–7° below that for
first running on modern fuel; map 1 is map 0 minus 4° from 1800 rpm
(poor fuel / hot weather). Raising the curve is gated (G-004).

**D-013 Timing arithmetic** (`firmware/include/timing.h`, exact semantics in the
header comments). Integer only; 64-bit intermediates where needed; rounding
half away from zero where stated; `curve_lookup` uses C truncating division;
`effective_advance` clamps to [adv_min, min(adv_max, lead − 100)] and never
returns a negative value.

**D-014 Serial CLI** (`firmware/include/cli.h`), 38400 8N1, one command per line.
* Tokens separated by one or more **spaces** (tab is not a separator); leading
  and trailing spaces ignored; trailing CR/LF ignored; commands and keys are
  lower-case; line longer than 63 chars (after CR/LF removal) → TOOLONG.
* Numbers: optional `-` then ≥ 1 decimal digit, nothing else (`+`, hex,
  letters → SYNTAX); value outside the field's C type range (uint8 0–255,
  uint16 0–65535, int16 −32768–32767), including arbitrarily long digit strings
  → RANGE.
* `set <key> <n>` / `get <key>` for keys: map, cyclediv, crankcycles, lead,
  trail, trim, latency, crankexit, crankenter, revlimit, revresume, advmin,
  advmax, dwellmax, dwellcrank, crankdwellmax, ssdhold, stall, vbatmin, magopen.
  `set` does not validate the whole config (so multi-field edits are possible);
  reply `OK`. `get` replies `key=value` (decimal, signed for trim).
* `curve <m 0-1> <i 0-7> <rpm> <adv>`, `dwell <i 0-3> <mv> <us>` → `OK`;
  index out of range → RANGE; wrong argument count → SYNTAX.
* `get curve <m>` → `curve<m> r0:a0 r1:a1 … r7:a7`; `get dwell` → `dwell mv0:us0 … mv3:us3`.
* `save` → validate; invalid → INVALID_CONFIG; valid → `config_seal`, set
  `*save_requested = true`, reply `OK`. `defaults` → `config_defaults`, `OK`.
  `help` → one-line command list, CLI_OK.
* If `rpm > 0`, every mutating command (set, curve, dwell, save, defaults)
  → BUSY, config untouched. `get`/`help` always allowed.
* Unknown command or key → UNKNOWN; empty line → SYNTAX.
* Errors reply `ERR <n>` with n = numeric `cli_status_t`. `working` changes only
  on CLI_OK; `*save_requested` is true only after a successful save. The reply
  is always NUL-terminated within `out_len` (truncated if needed, status unchanged).
* `status` is handled by the runtime (D-018), not by `cli_exec`:
  `rpm=<n> mode=<STOP|CRANK|RUN|LIMP|KILL|HOLD> adv=<cdeg> vbat=<mV> map=<m> resets=<n>`.

**D-015 Hardware binding.** Pin map in `firmware/include/hal.h`. Timer1 clk/8
(1 µs), 32-bit extension via overflow ISR; ICP1 with noise canceller, edge
select toggled after each capture; spark edge produced by OC1A hardware
compare ("clear on match") for zero software jitter; dwell start by OC1B ISR;
1 ms housekeeping tick (Timer2 CTC or Timer1-derived). Required ISRs:
TIMER1_CAPT, TIMER1_COMPA, TIMER1_COMPB, TIMER1_OVF (T-302). ADC6 with the
internal 1.1 V reference: `mv = adc * 1100 * 72700 / (4700 * 1024)` (C-031).
Fuses: lfuse 0x5F, hfuse 0xD1, efuse 0xFD (BOD 2.7 V; some avrdude versions
read unused efuse bits as 0, i.e. 0x05 — equivalent). Bit positions verified
against avr-libc iom328p.h (C-024).

**D-016 Variants.** Compile-time `VARIANT=TCI|MAGBREAK` (`-DDEFAULT_VARIANT_x`).
TCI: PB1 HIGH = coil charging; driven LOW = spark; Hi-Z = soft shutdown.
MAGBREAK: PB1 HIGH = breaker closed (primary shorted); HIGH→LOW = spark;
re-close `magbreak_open_cdeg` after opening; never Hi-Z; boot state LOW (open,
safe while unpowered); kill = hold HIGH (closed) — the magneto cut-out
convention. PB2 reserved for the HF-B harvest isolator (behaviour frozen at G-003).

**D-017 Segment method.** Below/at all speeds in RUN the spark delay after LEAD is
predicted from the previous cycle's measured LEAD→TRAIL time (`segment_delay_us`)
because that segment covers the same compression-stroke region where a single
decelerates; if no valid segment exists, `spark_delay_from_lead_us` on the
lead-to-lead period is used.

**D-018 Runtime behaviour** (implemented in S-015…S-017; tested by T-202…T-222):
1. **Boot:** set CLKPR, init HAL, drive PB1 LOW, load config
   (`config_load_or_default`; if defaults were loaded blink LED twice), working
   copy = active. If PB0 is LOW at boot, ignore the first TRAIL edge.
2. **Glitch filter:** ignore LEAD edges closer than the period of 2×rev_limit.
3. **Speed:** period = LEAD-to-LEAD (valid if < stall timeout); rpm from
   `rpm_from_period_us`. STOPPED → CRANK on the first LEAD. CRANK → RUN after
   `crank_exit_cycles` consecutive cycles ≥ crank_exit_rpm; RUN → CRANK below
   crank_enter_rpm. Map = 1 if PD3 grounded, else `active_map`.
4. **CRANK (and LIMP: vbat < vbat_min in any mode), TCI:** on LEAD, coil ON now
   if no valid segment, else schedule ON at LEAD + max(0, segment − dwell_crank).
   On TRAIL with the coil ON and a LEAD seen this cycle → spark immediately.
   If ON longer than crank_dwell_max without TRAIL → soft shutdown (D-019).
5. **RUN, TCI:** on LEAD compute advance (`effective_advance`), delay (D-017),
   dwell (`dwell_for_vbat`); spark via OC1A at LEAD+delay; if the coil is not
   already ON, ON at spark − dwell (or immediately if that is past). After each
   spark pre-schedule the next ON = predicted next spark − dwell if that falls
   before the next expected LEAD. TRAIL while the coil is ON and the spark has
   not happened → spark now (late backstop). A predicted ON with no LEAD within
   `dwell_max_us` → soft shutdown.
6. **Rev limiter:** evaluated when scheduling a dwell: rpm ≥ rev_limit → no new
   dwell until rpm ≤ rev_resume. A dwell already started always ends in a spark.
7. **Kill (PD2 low ≥ 20 ms):** TCI → soft shutdown, no dwell while active;
   MAGBREAK → hold closed. Release (high ≥ 20 ms) → wait for a fresh LEAD.
8. **Stall:** no edge for `stall_timeout_ms` → STOPPED (period/segment invalid,
   rpm 0); a charging coil is soft-shut first.
9. **MAGBREAK:** as 3–8 with "close" for ON and "open" for spark; on LEAD close
   if open; open at the spark time; re-close after `magbreak_open_cdeg`
   (in CRANK from the current segment: segment × open/(lead − trail); in RUN
   `angle_to_us(open, period)`); rev-limit = stay closed.
10. **EEPROM:** written only when rpm = 0 after `save`, then read back and
    checked with `config_check_image`; on success the new config becomes active
    atomically; on failure reply `ERR 7` and keep the old config.
11. **LED (D13):** mirrors the trigger latch (ON while PB0 low) when stopped —
    a static-timing aid like a points lamp; 2 blinks after boot = defaults
    loaded; fast blink = kill active.

**D-019 Soft shutdown (TCI).** Disconnect OC1A (COM1A = 0), set PB1 as input
without pull-up. Gate discharges through R7 470 kΩ with C7 10 nF over
milliseconds; collector current decays slowly so no spark. Output then held
Hi-Z for `ssd_hold_ms` before it is driven LOW again. Verified by B-406.

**D-020 Simulation contract.** `tests/sim/harness.h`: real ELF on simavr
ATmega328P at 8 MHz (CLKPR ignored by simavr — R-009), blank EEPROM (defaults),
ADC6 driven through the 68k/4k7 divider model, PB0/PD2/PD3 driven, PB1 state
from the pin IRQ plus DDRB. Timing tolerances in the tests are derived from
SC-2 (±0.5° bench).

## Decision rules (engineering forks)

**D-021 On-bike facts (S-002/S-003):**
a. Magneto drive: turn the crank two revolutions and count spindle turns.
   1 turn → `cyclediv 2`, magnets 25° apart on the disc. 2 turns → `cyclediv 1`,
   magnets 50° apart (wasted spark each revolution).
b. Earth polarity: negative earth → proceed. Positive earth → convert (swap
   battery leads, repolarise the dynamo, fit the N-type electronic regulator,
   C-022) as step S-003. If the owner will not convert → HALT, BLOCKED.
c. Rotation direction of the spindle decides which magnet is "first": the S
   magnet must reach the sensor 25° (spindle) before the N magnet.
d. Magneto type: rotating-armature (breaker rotates) or rotating-magnet (breaker
   stationary): in both the disc replaces the breaker plate and the sensor sits
   on the stationary housing; record the type in `hardware/SURVEY.md`.
e. Auto-advance unit on the magneto drive (C-034): lock it at FULL ADVANCE
   position or replace with a solid drive, then set the static timing marks.
f. Cut-out terminal present and reaching the primary (C-019): needed for
   MAGBREAK and for TCI (primary permanently shorted). If absent → fit a
   lead from the breaker centre bolt through an insulated brush (S-006).

**D-022 Battery-less path (gate G-003 after S-020).** At the bench, magneto
driven at controlled speed, with Q2 as the breaker:
(a) a spark jumps a 7 mm three-point gap at 150 spindle rpm;
(b) the break-angle window that still jumps 7 mm is ≥ 30° crank at 500 spindle rpm;
(c) HF-A or HF-B boots the controller from empty within one spindle
    revolution at 150 rpm and sustains ≥ 15 mA at 5 V from 300 rpm.
All three → BL-1 MAGBREAK. (a)+(b) only → present BL-3 and BL-2 at G-003.
(a) fails → BL-3 or BL-2 at G-003.

**D-023 Parts:** out of stock or obsolete → next ranked alternative in BOM.md,
record in DEVIATIONS.md. Never substitute a part that is not in the ranking
without a new decision entry.

**D-024 Test outcomes:** failing frozen test → fix the implementation, never the
test. Believed-wrong test → TEST_CHALLENGE (HANDOFF.md). Bench test failure →
the specific remedy in tests/bench/BENCH.md for that test; if none works →
BLOCKED.

**D-025 On-road outcomes (S-021/S-022):** pinking/knock → switch to map 1
immediately, then lower the affected curve points by 200 cdeg and repeat.
Any kickback → stop, verify TRAIL = TDC with the strobe (B-410), never set
`trail` above 500. Misfire at high rpm → check dwell (B-405) and plug/lead.

**D-026 Deployment (operational) forks.** "Staging" = the bench (S-007/S-008);
"production" = the bike (S-010/S-021).
a. Staging flash or smoke test fails (B-413 part 1–2) → reflash the previous
   image (`evidence/S-018.txt` hashes identify it), re-run B-413; if still failing → BLOCKED.
b. Health check fails after installation (`status` does not reply, `resets` > 0
   at idle, LED 2-blink after a save, or B-410 off by > 1°) → switch SW1 off, roll
   back (OPERATIONS.md "Rollback to magneto" or reflash previous image), HALT.
c. "Monitoring alert" during a ride (misfire, LED fast-blink without kill, rider
   feels timing change) → stop, SW1 off, read `status`; if not explained by a
   decision rule → roll back to magneto and HALT.
d. Dependency install fails (apt package missing) → use the versions in
   ENVIRONMENT.md from the Ubuntu 24.04 archive; if unavailable → BLOCKED.
e. Performance target missed (T-205/T-206 outside ±0.5°) → move the spark edge to
   OC1A hardware compare if not already, remove work from ISRs; never widen tolerances.
f. Security/dependency scan (vendor hash mismatch in verify-freeze) → HALT, BLOCKED.

## Default rule (verbatim)
> Choose the most reversible option that does not expand scope, log it in `DEVIATIONS.md` with rationale, and continue — **unless** it touches frozen tests, security, data integrity, a public interface, research integrity, or (upward) a statement's evidence class, in which case halt and write `BLOCKED.md`.

Additionally for this project: any action that would start the engine or change
ignition timing on the road outside a gated step is treated as touching safety →
halt and write `BLOCKED.md`.
