# Components and ranked choices (D-002 … D-008)

Ranking criteria, in the order you asked for them to count (A-006):
**applicability** to a small, compact, programmable unit on a kick-start
Ariel 350 single → **availability** (UK distributors, stock, not obsolete) → **cost**.
Cost bands are indicative (A-012): £ < £5, ££ £5–20, £££ £20–60, ££££ > £60.
Check prices and stock at order time (step S-004).

## Ranked choices

### Ignition type / power architecture (D-002)
| Rank | Option | Applicability | Availability | Cost | Verdict |
|---|---|---|---|---|---|
| 1 | **TCI, battery** — inductive coil switched by an ignition IGBT, MCU timing | Best for a bike with a working battery + dynamo; smallest parts count | Excellent | ££ | **Chosen (battery case)** |
| 2 | **MAGBREAK, battery-less** — keep the magneto as generator + HT coil, MCU-timed electronic breaker replaces the points | Keeps original look, inherently battery-less, one-kick heritage; advance limited to the magneto's usable window | Excellent (common parts) | £–££ | **Chosen (battery-less case), subject to spike S-020 + gate G-003** |
| 3 | Alternator in dynamo shell + regulator + capacitor, feeding the TCI | Works without a battery once running; kick-start on a capacitor is marginal (C-016) | Medium (specialist conversion) | ££££ | Battery-less fallback 1 (BL-3) |
| 4 | Self-generating CDI "mag-box" (magnet rotor + source coil + CDI) | Fully battery-less and strong spark, but a custom generator must be designed and machined | Low (custom) | £££ | Battery-less fallback 2 (BL-2) |
| — | Buy a commercial self-generating electronic magneto | Not a custom build; reliability comments mixed (C-018) | Medium | ££££ | Reference only |

### Controller (D-003)
| Rank | Option | Why |
|---|---|---|
| 1 | **Arduino Pro Mini 5 V/16 MHz clone (ATmega328P), run at 8 MHz, no bootloader** | 33×18 mm; 16-bit Timer1 with input capture + hardware compare output (jitter-free spark edge); on-chip EEPROM for curves; 2.7–5.5 V at 8 MHz; boots in ms without a bootloader; huge availability. £ |
| 2 | ATtiny1614/3216 bare chip on a custom PCB | Smaller and cheaper in volume, UPDI programming, but needs a custom board first. £ |
| 3 | RP2040-Zero | Cheap and USB-native, but 3.3 V I/O (marginal IGBT gate drive), higher current, no EEPROM, slower cold boot. £ |
| 4 | STM32 "Blue Pill" | Capable but 3.3 V and clone-quality problems. £ |
| 5 | ESP32-C3 (Bluetooth tuning) | Wireless tuning is attractive but 50–100 mA peaks and ~100 ms+ boot are wrong for battery-less kick starting; adds a radio attack surface. ££ |

### Trigger (D-004)
| Rank | Option | Why |
|---|---|---|
| 1 | **A1220 Hall latch + two SmCo magnets (S = lead, N = trail)** | Works from zero rpm (kick), two independent edges give a fixed-TDC cranking spark with no prediction, −40…150 °C, AEC-Q100, 3–24 V. £ |
| 2 | A1120 unipolar switch + one long arc magnet | Also works; arc magnet harder to source and position. £ |
| 3 | Slotted disc + optical interrupter | Oil/dirt sensitive inside a breaker housing. £ |
| 4 | Variable-reluctance pickup | Output too small at kick speed. £ |

### Output switch (D-005)
| Rank | Option | Why |
|---|---|---|
| 1 | **onsemi FGD3040G2-F085V (DPAK)** | Active, logic-level, 400 V internal clamp, 300 mJ SCIS. £ |
| 1= | onsemi ISL9V3040D3ST (DPAK) / ISL9V3040P3 (TO-220) | Same class, widely stocked; TO-220 easiest to hand-solder. £ |
| — | onsemi FGBS3040E1-F085 smart driver with built-in soft shutdown | **Rejected: obsolete** (C-015). Soft shutdown is done with the gate network instead (D-019). |
| — | BU941 Darlington | Works, but no clamp, higher saturation loss. |

### Coil (D-006)
| Rank | Option | Why |
|---|---|---|
| 1 | **Compact 6 V motorcycle coil, primary 1.2–1.8 Ω** (or 12 V, 2.8–3.5 Ω after a 12 V conversion) | Fits under the tank/seat; single HT output. ££ |
| 2 | Period-style canister coil (6 V) | Looks original, bulkier. ££–£££ |
| 3 | Pencil/coil-on-plug | Clearance and plug angle on the Ariel head unverified; avoid. |

### System voltage (D-007)
Keep 6 V if the dynamo and regulator are healthy (default). A 12 V conversion
(electronic dynamo regulator, 12 V battery, 12 V coil, `dwell` table re-entered)
is supported with no hardware change to the controller.

## Bill of materials — TCI (battery)

| Ref | Qty | Part | Notes | Band |
|---|---|---|---|---|
| A1 | 1 | Arduino Pro Mini 5 V/16 MHz (ATmega328P) | on-board regulator unused (RAW not connected) | £ |
| U1 | 1 | LM2936MP-5.0 (SOT-223) or LM2936Z-5.0 (TO-92) | 5.5–40 V operating, −24 V reverse, −50 V transient, 50 mA (C-025). On a 6 V battery it runs in dropout during kicks: output tracks input (B-402) | £ |
| Q1 | 1 | FGD3040G2-F085V or ISL9V3040D3ST/P3 | ignition IGBT | £ |
| H1 | 1 | Allegro A1220LUA-T | Hall latch, SIP-3 | £ |
| M1, M2 | 2 | SmCo disc magnet Ø5 × 3 mm, axially magnetised | temperature-safe | £ |
| — | 1 | Acetal (Delrin) rod Ø40 mm × 20 mm | magnet carrier disc (S-006) | £ |
| D1 | 1 | SS14 (1 A, 40 V Schottky) | reverse protection | £ |
| D2 | 1 | SMBJ20A TVS | supply clamp | £ |
| R1 | 1 | 10 Ω 1 W | | £ |
| R2, R3 | 1+1 | 68 kΩ, 4.7 kΩ 1 % | supply sense | £ |
| R4, R5 | 1+1 | 4.7 kΩ, 1 kΩ | trigger pull-up / series | £ |
| R6, R7 | 1+1 | 220 Ω, 470 kΩ | gate network | £ |
| R8, R9 | 1+1 | 10 kΩ, 1 kΩ | kill input | £ |
| R10 | 1 | 10 kΩ | map-select pull-up (DEV-001) | £ |
| C1 | 1 | 100 µF 35 V low-ESR electrolytic | | £ |
| C2, C4, C6, C8 | 4 | 100 nF X7R | | £ |
| C3 | 1 | 22 µF 16 V (LDO output, ESR per LM2936 datasheet) | | £ |
| C5 | 1 | 1 nF C0G | trigger filter | £ |
| C7 | 1 | 10 nF C0G/X7R 50 V | soft-shutdown timing | £ |
| L1 | 1 | Ignition coil (see D-006) | | ££ |
| — | 1 | Copper-core HT lead + 5 kΩ suppressor cap (or resistor plug) | EMI (R-007) | £ |
| SW1 | 1 | Main ignition switch ≥ 10 A (existing key/toggle if suitable) | hard stop | £–££ |
| SW2 | 1 | Handlebar momentary N/O stop button | firmware kill | £ |
| SW3 | 1 | Miniature toggle (optional) | map select | £ |
| F1 | 1 | 5 A blade fuse + holder | | £ |
| E1 | 1 | Die-cast aluminium box ~ 92 × 38 × 31 mm (e.g. Hammond 1590A) | Q1 heatsinks to the wall | ££ |
| J1/J4 | 2 | Superseal 1.0 4-way and 3-way pairs | sealed connectors | ££ |
| — | — | 3-core screened cable, 1.0 mm² and 0.5 mm² wire, heatshrink, thermal pad | | ££ |
| Tools | 1+1 | USBasp ISP programmer; FTDI/CH340 5 V TTL serial lead | flashing / tuning | ££ |

Controller electronics ≈ ££; with coil, box and connectors ≈ £££ (A-012).

## Bill of materials — MAGBREAK additions (battery-less)
Controller A1, trigger H1/M1/M2, kill-sense parts and connectors as above, **plus**:
| Ref | Qty | Part | Notes |
|---|---|---|---|
| Q2 | 1 | FGD3040G2-F085V / ISL9V3040 | electronic breaker |
| HF | 1 set | Harvest front end per gate G-003: HF-A (fast 1 kV rectifiers e.g. UF4007, 5.6 V zener, 470 µF reservoir, 5 V LDO) or HF-B (bridge of UF4007, IXTY08N100D2 depletion MOSFET, same reservoir/LDO) | exact values frozen at G-003 |
| SW2 | 1 | Cut-out push button (existing magneto cut-out if fitted) | shorts P to earth |
