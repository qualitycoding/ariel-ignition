# Wiring and circuit specification (frozen by D-015, D-016, D-019)

Two variants share one controller board, one trigger and one firmware source.
The variant is chosen at build time (`VARIANT=TCI` or `VARIANT=MAGBREAK`).

| | **TCI — battery** | **MAGBREAK — battery-less** |
|---|---|---|
| Energy source | 6 V (or 12 V) battery, charged by the existing dynamo | the retained Lucas magneto itself |
| HT source | external ignition coil | magneto armature (original HT pick-up and lead) |
| Switch element | ignition IGBT in series with the coil primary | ignition IGBT across the magneto primary (replaces the points) |
| Controller supply | battery via LM2936-5.0 LDO | harvested from the magneto primary (topology decided at gate G-003) |
| Engine stop | main switch SW1 (hard) + kill button SW2 (firmware) | cut-out button SW2 shorts the primary (hard, the original method) |
| Status | fully specified | electrical front end is gated by spike S-020 + gate G-003 |

Diagrams: `wiring-tci.svg`, `wiring-magbreak.svg` (block wiring). The netlists below
are the authoritative connection lists; the KiCad schematic produced in S-009
must match them net-for-net (check B-401).

## 1. Common: trigger (both variants)

The contact-breaker plate on the magneto spindle is replaced by an acetal
(Delrin) **magnet carrier disc** clamped by the original centre bolt. Two SmCo
disc magnets (Ø5 × 3 mm) sit in the disc rim: **south face outward at the lead
position**, **north face outward at the trail position**. An Allegro **A1220LUA-T**
bipolar Hall latch sits in a bracket on the breaker housing, 1.0–1.5 mm from the
magnet faces (air gap set in S-006, verified by B-403).

The magneto normally turns at half engine speed (C-003), so crank angles on the
disc are halved: lead 50° BTDC and trail 0° (TDC) crank = **25° of spindle
between the magnets**. If S-002 finds an engine-speed drive, set `cyclediv 1`
and space the magnets 50° apart (D-021 rule).

Latch behaviour (C-011): south pole turns the open-drain output ON (pulls low),
north pole turns it OFF. So: **falling edge = LEAD**, **rising edge = TRAIL**.

| Net | From | To | Notes |
|---|---|---|---|
| +5V_S | board +5V | A1220 pin 1 (VCC) | via screened 3-core cable, brown |
| TRIG | A1220 pin 3 (OUT) | R5 1k → PB0 (D8) | white; R4 4k7 pull-up PB0-side to +5V; C5 1 nF PB0→GND |
| GND_S | board GND | A1220 pin 2 (GND) | blue; screen bonded to GND at board end only |
| — | C6 100 nF | across A1220 VCC–GND at the sensor | |

## 2. TCI (battery) netlist

```
BATTERY+ ──SW1 (main switch)──F1 5A── IGN+
IGN+ ── L1 coil primary (+) ; L1 primary (−) ── Q1 collector ; Q1 emitter ── PGND (star) ── frame
L1 HT ── copper-core HT lead ── 5 kΩ suppressor cap ── spark plug
IGN+ ── D1 (SS14) ── R1 10Ω 1W ── VIN_F ; VIN_F ── C1 100µF/35V + C2 100nF ── GND
VIN_F ── D2 SMBJ20A (TVS) ── GND
VIN_F ── U1 LM2936MP-5.0 IN ; U1 OUT ── +5V ── C3 22µF + 100nF ── GND ; +5V ── Pro Mini VCC pin
IGN+ (before D1) ── R2 68k ── VSENSE ── R3 4k7 ── GND ; VSENSE ── C4 100nF ── GND ; VSENSE ── A6
PB1 (D9) ── R6 220Ω ── GATE ; GATE ── R7 470k ── PGND ; GATE ── C7 10nF ── PGND ; GATE ── Q1 gate
PD2 (D2) ── R9 1k ── KILL ; KILL ── R8 10k ── +5V ; KILL ── C8 100nF ── GND ; KILL ── SW2 (N/O push) ── GND
PD3 (D3) ── SW3 (map toggle, optional) ── GND
PD0/PD1/GND ── J2 tuning connector (FTDI 5 V TTL: GND, RX←TX, TX→RX)
Pro Mini ISP pads (MOSI D11, MISO D12, SCK D13, RST, VCC, GND) ── J3 6-pin ISP (flashing only)
GND (signal) joins PGND at ONE point: Q1 emitter pad.
```

Component notes (details and ranked alternatives in `BOM.md`):
* **Q1** logic-level ignition IGBT with internal ~400 V clamp: onsemi
  **FGD3040G2-F085V** (DPAK) or **ISL9V3040D3ST** (C-013, C-014). Mounted on the
  enclosure wall through a thermal pad.
* **Soft shutdown (D-019):** firmware aborts a charge by switching PB1 to an
  input. The gate then discharges only through R7 (470 k) with C7 (10 nF) plus
  the gate's own charge, so the collector current ramps down over milliseconds
  instead of microseconds and the secondary voltage stays far below spark
  breakdown. Verified on the bench by B-406 (no spark across a 7 mm gap).
* **L1** coil: 6 V system → 6 V coil, primary 1.2–1.8 Ω; 12 V system → 12 V
  coil, primary 2.8–3.5 Ω (no ballast). Peak current 4–5 A. Measure R in B-402.
* **SW1** is the hard stop: it removes power from the coil and the controller.
* **SW2** is the handlebar stop button (firmware kill, 20 ms debounce,
  soft-shutdown of any charging coil, no spark while held).
* Stopping by stalling (closing the throttle, using the valve lifter, or
  stalling in gear) still works: the engine stops, the trigger stops, the
  firmware soft-shuts the coil within `dwellmax`/`crankdwellmax` and goes to
  STOPPED after `stall` ms. Leave SW1 off when parked.

## 3. MAGBREAK (battery-less) netlist — gated

The magneto keeps its armature, condenser, slip ring, HT pick-up and HT lead.
The points are removed (the carrier disc of §1 replaces the breaker plate).
The **primary live end (P)** is reached, stationary, through the magneto's own
**cut-out terminal and brush** on the breaker cover (C-019, to be confirmed on
this magneto in S-002).

```
P (cut-out terminal) ── Q2 switch element ── earth (magneto body / frame)
P ── SW2 cut-out push button (N/O) ── earth      ← hard kill: shorts the primary, exactly as original
P ── harvest front end (HF-A or HF-B, chosen at gate G-003) ── VRES ── U2 LDO 5 V ── +5V ── Pro Mini VCC
PB1 (D9) ── Q2 gate network (as §2: R6/R7/C7)
PB2 (D10) ── harvest isolate control (HF-B only)
Trigger, kill sense, tuning and ISP connectors exactly as §1/§2 (kill sense PD2 wired to SW2 via 10k so the MCU also sees it)
```

Candidate harvest front ends (resolved by spike S-020, decision rule D-022):
* **HF-A** — rectify the *non-firing* flux reversal of the armature (the second
  reversal per spindle revolution that a single-cylinder magneto wastes) into a
  zener-clamped reservoir capacitor; Q2 held closed through the firing reversal.
* **HF-B** — full bridge on P with Q2 across the bridge DC side and a
  depletion-mode isolation MOSFET (e.g. IXTY08N100D2) in the reservoir path,
  opened by PB2 at the firing instant.
* **Fallback** — if neither boots the controller within the first kick at
  150 spindle rpm with ≥ 10 kV available spark, gate G-003 chooses BL-3
  (alternator + capacitor + TCI) or BL-2 (self-generating CDI mag-box).

## 4. Connectors and harness

| Conn | Pins | Type | Signals |
|---|---|---|---|
| J1 power/coil | 4 | Superseal 1.0 or Deutsch DT | IGN+, COIL−(Q1 C), PGND, KILL |
| J4 trigger | 3 | Superseal 1.0 | +5V_S, TRIG, GND_S (screen to GND_S) |
| J2 tuning | 3 | 0.1" header inside a sealed gland, or a DT 3-way | GND, TX, RX |
| J3 ISP | 6 | 0.1" 2×3 header, internal only | Flashing only |

Wire: 1.0 mm² (16 A) for IGN+/COIL−/PGND; 0.5 mm² signals; screened 3-core for the trigger.
Keep the HT lead at least 50 mm from the trigger cable and the controller.
