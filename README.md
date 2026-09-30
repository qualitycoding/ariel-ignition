# Ariel 350 electronic ignition

A small, programmable electronic ignition for a magneto-fired Ariel NH (Red Hunter)
350 single. It keeps the magneto body in place, so the magdyno still drives the
dynamo and the original parts can go back on in under 30 minutes.

> **Status: plan and frozen tests only.** The firmware functions are stubs; the
> step-by-step implementation plan is in `plan/PLAN.md` and starts with `HANDOFF.md`.
> Nothing has been fitted to a motorcycle yet.

## How it works
* A plastic disc with two small magnets replaces the contact-breaker plate on the
  magneto spindle. A Hall-effect latch in the breaker housing sees the south magnet
  at **50° before TDC (LEAD)** and the north magnet at **TDC (TRAIL)**.
* **Kicking over:** the coil charges at LEAD and fires exactly at TDC on the TRAIL
  edge. No prediction is used, so a slow or failed kick cannot fire early and
  cannot kick back. If the kick stalls before TDC, the coil is shut down slowly so
  that no spark occurs.
* **Running:** above 600 rpm the controller times the spark from the LEAD edge
  using a programmable advance curve. There are two maps and a rev limiter.
* **Stopping:** a main switch (hard stop), a handlebar kill button, and plain
  stalling all stop it cleanly. On the battery-less variant, the kill button
  shorts the magneto primary, just like the original cut-out.

## Variants
| | TCI (battery) | MAGBREAK (battery-less) |
|---|---|---|
| Spark source | separate ignition coil | the original magneto armature |
| Switch | ignition IGBT in the coil circuit | ignition IGBT replacing the points |
| Status | fully specified | gated on a bench spike (S-020, gate G-003) |

## Wiring and parts
* Diagrams: [`hardware/wiring-tci.svg`](hardware/wiring-tci.svg),
  [`hardware/wiring-magbreak.svg`](hardware/wiring-magbreak.svg)
* Netlists (authoritative): [`hardware/WIRING.md`](hardware/WIRING.md)
* Ranked component choices and BOM: [`hardware/BOM.md`](hardware/BOM.md)

Key parts: Arduino Pro Mini 5 V (ATmega328P, run at 8 MHz, no bootloader),
Allegro A1220 Hall latch plus 2 SmCo magnets, onsemi FGD3040G2-F085V / ISL9V3040
ignition IGBT, TI LM2936-5.0 regulator, 6 V (or 12 V) coil, and a small die-cast
aluminium box.

## Tuning over serial (38400 baud, engine stopped)
```
get curve 0            -> curve0 500:0 800:600 1200:1200 ... (centidegrees BTDC)
curve 0 5 3000 3100    -> set point 5 of map 0 to 31.00 deg at 3000 rpm
set revlimit 5800
save
status                 -> rpm=0 mode=STOP adv=0 vbat=6310 map=0 resets=0
```
The controller refuses changes while the engine is turning. The full grammar is
in `plan/DECISIONS.md` D-014.

## Build and test
```
sudo apt-get install -y build-essential gcc-avr avr-libc binutils-avr avrdude simavr libsimavr-dev libelf-dev
make test                                   # unit + simavr firmware-in-the-loop + ELF checks
make firmware                               # both variants
make -C firmware fuses                      # once, via USBasp
make -C firmware VARIANT=TCI flash
```
The simulation tests run the actual AVR firmware on a simulated ATmega328P. They
drive the trigger, kill, map and battery-voltage inputs, then measure spark
angles, dwell, soft shutdown and rev-limit behaviour.

## Safety
The coil makes more than 20 kV. Bench-test with a 7 mm spark gap
(`tests/bench/BENCH.md`) before fitting. The first start and any advance above
36° are human gates (`plan/GATES.md`). Keep the original breaker parts as a
rollback kit (`plan/OPERATIONS.md`).

## Licences
Firmware and tools: Apache-2.0. Hardware files: CERN-OHL-P-2.0 (licence texts
are added in step S-001). Unity test framework: MIT (`vendor/unity/LICENSE.txt`).
