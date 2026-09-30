<!-- FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E). -->
# Bench and on-bike acceptance tests (B-401 … B-412)

Physical tests executed by the builder; the agent records results in
`evidence/B-4xx.md` using the table in each test. Pass criteria are exact.
Safety: the coil secondary exceeds 20 kV. Keep hands clear, use insulated
pliers, never run the coil with the HT lead unconnected (use the 7 mm gap).

**Rig:** bench PSU 0–15 V/5 A with current limit; the controller; the real
coil; a three-point spark gap set to 7 mm; the trigger simulator (S-019) or the
real disc on the drive; 2-channel scope (≥ 20 MHz) with a 10:1 probe;
USB-serial tuning lead.

## B-401 Netlist continuity (SC-7)
Power off. Beep out every net in WIRING.md §2 (or §3). **Pass:** every net
continuous; no continuity between IGN+, +5V, GND and GATE pairs (> 1 MΩ except
where a resistor joins them, within ±5 % of its value).
Remedy: rework the joint.

## B-402 Power-up (SC-7)
Q1 collector disconnected. PSU current limit 100 mA.
1. 6.3 V in: +5V rail 4.85–5.15 V; supply current ≤ 30 mA.
2. 4.5 V in: MCU runs (`status` replies) — dropout behaviour.
3. 15.0 V in: +5V rail 4.85–5.15 V.
4. Reverse polarity −6.3 V for 5 s: current ≤ 5 mA; board works afterwards.
5. With ISP: fuses read lfuse 0x5F, hfuse 0xD1, efuse 0xFD (or 0x05).
6. Measure coil primary resistance: within D-006 range.
7. Lower the supply slowly from 5.0 V at the +5V rail (feed VCC directly): the MCU resets (LED/`status` stop) between 2.5 and 2.9 V (BOD 2.7 V, C-024).
**Pass:** all seven. Remedy: check D1/U1 orientation and U1 output capacitor.

## B-403 Trigger angles (SC-2)
Disc and sensor fitted; controller powered; LED mirrors the latch. Turn the
engine forward by hand on the degree disc. **Pass:** LED turns ON at 50° ±1°
BTDC and OFF at 0° ±1° (TDC) of the compression stroke, and does not change on
the exhaust-stroke TDC. Remedy: move the bracket; check magnet polarity.

## B-404 Timing accuracy on the bench (SC-2)
Trigger simulator at 300, 1000, 3000, 5500 rpm; scope ch1 = trigger, ch2 = Q1
collector (10:1, spark = collector flyback). **Pass:** measured spark angle =
curve value ±0.5° at every speed (angle = time from spark to simulated TDC ×
6 × rpm / 1e6 degrees); cranking (300 rpm) spark at TDC ±0.5°.
Remedy: set `latency` to the measured constant delay; recheck.

## B-405 Spark energy (SC-1)
7 mm gap. **Pass:** continuous sparking at 300 and 5500 rpm with PSU at 6.3 V,
and at 300 rpm with PSU at 4.8 V (kick sag) — no missed spark in 100 cycles
(count on scope). Remedy: raise `dwellcrank` / dwell table within limits;
coil alternative (D-006 rank 2).

## B-406 Soft shutdown (SC-5)
Simulator: one LEAD edge only (kick that stalls). **Pass:** collector voltage
never exceeds 60 V during shutdown (scope), no spark at the 7 mm gap, over 20
repetitions; also with `kill` pressed during a dwell. Remedy: increase C7 to
22 nF (D-019), recheck.

## B-407 Stop functions (SC-4)
At 1500 rpm simulated: (a) press SW2 → no spark within 60 ms, sparks resume
after release; (b) switch SW1 off → controller and coil unpowered within 1 s;
(c) stop the simulator mid-cycle → coil current zero within 60 ms (current
clamp) and no spark. **Pass:** all three.

## B-408 EMI (SC-7)
Controller 30 cm from the sparking gap, trigger cable 1 m, 5500 rpm for 30 min.
**Pass:** `status` shows `resets=0`; B-404 still passes at 3000 rpm afterwards.
Remedy: resistor cap/plug, ferrite on the trigger cable at the box, screen
bonding check.

## B-409 Thermal (SC-7)
Box closed, 25 °C ambient, 5900 rpm simulated (just below the rev limit) with 7.5 V
supply for 30 min. **Pass:** box wall next to Q1 ≤ 70 °C; coil body ≤ 80 °C.

## B-410 Strobe check on the bike (SC-2)
Engine running, strobe on the degree disc. **Pass:** idle (≤ 1000 rpm) and
3000 rpm spark = `get curve` value ±1°. Remedy: static error → `trim`; error
growing with rpm → `latency` (µs = error° × 1e6 / (6 × rpm)).

## B-411 Battery-less first kick (MAGBREAK, SC-1)
Magneto on the bench drive from rest, controller fully discharged (10 min
unpowered). One "kick" profile (0 → 250 spindle rpm in 0.5 s). **Pass:** spark
at the 7 mm gap within the first spindle revolution in 9 of 10 attempts.

## B-412 Rollback drill (SC-8)
Time the OPERATIONS.md "Rollback to magneto" procedure with the original
parts. **Pass:** ≤ 30 min, engine starts on the magneto.

## B-413 Deployment smoke, health check, alert and firmware rollback (software.deploys)
1. Smoke (staging): flash the S-018 image by ISP; within 2 s of power-up
   `status` replies with `mode=STOP resets=0`.
2. Health: `get lead` → `lead=5000` (or the saved value); LED mirrors the latch.
3. Alert: erase the EEPROM (`avrdude -p m328p -c usbasp -e` — EESAVE keeps it, so
   instead write 0xFF: `avrdude -p m328p -c usbasp -U eeprom:w:0xff:m`); power-cycle:
   LED gives 2 blinks and `status` still replies (defaults loaded, D-018.1).
4. Rollback: flash the previous image, then the current one again; both boot per step 1.
**Pass:** all four. Remedy: D-026a.
