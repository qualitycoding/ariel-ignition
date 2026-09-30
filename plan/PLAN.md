STATUS: READY — 24 steps (S-001…S-024), 4 gates (G-002, G-003, G-004, G-006), 0 open High risks (see premortem/RISK_REGISTER.md).
Generation branch: gen-20260930T084855Z-ariel-electronic-ignition

# Implementation plan — Ariel 350 electronic ignition

Read HANDOFF.md first. Tracks: **F** firmware (agent executes), **H** hardware
and bike work (a human builder executes the physical actions; the agent records
results and runs the checks). Order: S-001 → F track (S-011…S-018, S-019) →
H track (S-002…S-010) → G-002 → S-021 → G-004 → S-022. MAGBREAK track
(S-020 → G-003 → S-023) can run after S-018 in parallel with the H track.
Steps marked **HUMAN** contain physical actions; the agent must not mark them
complete without the recorded measurements named in "Evidence produced".

### S-001 Baseline, licences and red run
- Tier: Haiku
- Profile: software
- Depends on: none
- Inputs: this branch
- Actions:
1. `make verify-freeze` — every line must print `OK`.
2. Add `LICENSE` (Apache-2.0 full text) at repo root and `hardware/LICENSE` (CERN-OHL-P-2.0 full text) from the official sources (apache.org, ohwr.org).
3. Run `make test`; save the output to `evidence/S-001-red.txt`.
4. Commit: `git add -A && git commit -m "S-001: licences and red baseline"`; update `.checkpoints/state.json`.
- Outputs: licence files; red baseline log with 31 Unity failures and T-301/T-302 failures.
- Evidence produced: `evidence/S-001-red.txt`
- Done when: none (baseline); `make verify-freeze` OK.
- Checkpoint: `.checkpoints/state.json` step S-001 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: verify-freeze mismatch → HALT, BLOCKED (tests altered). Rollback: `git revert` the commit.
- Gate: none
- Relevant decisions/claims: D-009, C-032

### S-011 CRC-16 implementation
- Tier: Haiku
- Profile: software
- Depends on: S-001
- Inputs: `firmware/include/crc16.h`, D-011
- Actions:
1. Implement `crc16_ccitt()` in `firmware/src/crc16.c` (bitwise, poly 0x1021, init 0xFFFF; no table, to save flash).
2. `make test-unit` and check the T-001 lines.
3. Commit: `git add -A && git commit -m "S-011: CRC-16/CCITT-FALSE"`; update `.checkpoints/state.json`.
- Outputs: T-001 passes.
- Evidence produced: `evidence/S-011.txt` (test output)
- Done when: T-001
- Checkpoint: step S-011 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: fix implementation; never edit tests (D-024). Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-011, C-020

### S-012 Configuration defaults, validation, image loading
- Tier: Sonnet
- Profile: software
- Depends on: S-011
- Inputs: `config.h`, D-012
- Actions:
1. Implement `config_defaults`, `config_validate` (exact order in D-012), `config_seal`, `config_check_image`, `config_load_or_default` in `firmware/src/config.c`. Default `variant` = compiled variant (`DEFAULT_VARIANT_TCI` → 0, `DEFAULT_VARIANT_MAGBREAK` → 1).
2. `make test-unit`.
3. Commit: `git add -A && git commit -m "S-012: configuration defaults, validation and image loading"`; update `.checkpoints/state.json`.
- Outputs: T-002, T-003, T-004 pass.
- Evidence produced: `evidence/S-012.txt`
- Done when: T-001…T-004
- Checkpoint: step S-012 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-024. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-012, C-002, C-006

### S-013 Timing arithmetic
- Tier: Sonnet
- Profile: software
- Depends on: S-012
- Inputs: `timing.h`, D-013, D-017
- Actions:
1. Implement every function in `firmware/src/timing.c` exactly per the header comments (integer maths, 64-bit intermediates for products that can exceed 2^31).
2. `make test-unit`.
3. Commit: `git add -A && git commit -m "S-013: timing arithmetic"`; update `.checkpoints/state.json`.
- Outputs: T-005…T-010 pass.
- Evidence produced: `evidence/S-013.txt`
- Done when: T-001…T-010
- Checkpoint: step S-013 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-024. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-013, D-017, C-006

### S-014 Serial tuning interpreter
- Tier: Sonnet
- Profile: software
- Depends on: S-013
- Inputs: `cli.h`, D-014
- Actions:
1. Implement `cli_exec` in `firmware/src/cli.c`: work on a copy of `*working`, commit the copy only on CLI_OK; bounded formatting (`snprintf` on host, a small formatter on AVR — no `printf` float code).
2. `make test-unit`.
3. Commit: `git add -A && git commit -m "S-014: serial tuning interpreter"`; update `.checkpoints/state.json`.
- Outputs: T-011…T-016 pass.
- Evidence produced: `evidence/S-014.txt`
- Done when: T-001…T-016
- Checkpoint: step S-014 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-024. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-014

### S-015 Hardware abstraction layer
- Tier: Sonnet
- Profile: software
- Depends on: S-014
- Inputs: `hal.h`, D-015
- Actions:
1. Create `firmware/src/hal.c` (AVR only; exclude from the host build): CLKPR /2 as the first statement of `hal_init`; Timer1 clk/8, ICP1 + noise canceller, OVF 32-bit extension; OC1A/OC1B; 1 ms tick; ADC6 1.1 V ref free-running or tick-triggered; UART 38400 with a 64-byte RX line buffer; PD2/PD3 pull-ups; kill debounce 20 ms; LED.
2. `make firmware` — both variants compile with `-Werror`.
3. Commit: `git add -A && git commit -m "S-015: hardware abstraction layer"`; update `.checkpoints/state.json`.
- Outputs: HAL compiles; `hal_now_us()` monotonic across overflow.
- Evidence produced: `evidence/S-015-size.txt` (`avr-size` both variants)
- Done when: T-001…T-016 (unchanged)
- Checkpoint: step S-015 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: compile errors → fix; D-024. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-015, C-023, C-024, C-025

### S-016 TCI runtime state machine and main loop
- Tier: Opus
- Profile: software
- Depends on: S-015
- Inputs: D-016, D-018, D-019, `tests/sim/test_sim_tci.c`
- Actions:
1. Create `firmware/src/runtime.c` (+ private header) implementing D-018 items 1–8 and 10–11 for TCI, all timing decisions inside the ISRs listed in D-015; no floating point; no blocking calls in ISRs.
2. Replace the `main.c` stub: init, EEPROM load, main loop = serial line handling (`status` + `cli_exec`), save handling (D-018.10), LED.
3. `make test-sim test-elf` (TCI part) until T-202…T-211, T-301, T-302 pass.
4. Commit: `git add -A && git commit -m "S-016: TCI runtime"`; update `.checkpoints/state.json`.
- Outputs: TCI firmware behaves per D-018 in simulation.
- Evidence produced: `evidence/S-016.txt`
- Done when: T-001…T-016, T-202…T-211, T-301, T-302
- Checkpoint: step S-016 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-024; if a sim test appears physically inconsistent with D-018 → TEST_CHALLENGE. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-016, D-018, D-019, C-030, C-031

### S-017 MAGBREAK variant runtime
- Tier: Sonnet
- Profile: software
- Depends on: S-016
- Inputs: D-016, D-018 item 9
- Actions:
1. Add the MAGBREAK output behaviour (compile-time branches on `DEFAULT_VARIANT_MAGBREAK`); PB2 configured as output LOW and otherwise unused until G-003.
2. `make test`.
3. Commit: `git add -A && git commit -m "S-017: MAGBREAK variant"`; update `.checkpoints/state.json`.
- Outputs: both variants pass.
- Evidence produced: `evidence/S-017.txt`
- Done when: all automated tests (T-001…T-016, T-202…T-211, T-220…T-222, T-301, T-302)
- Checkpoint: step S-017 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-024. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-016, D-018

### S-018 Firmware release candidate
- Tier: Haiku
- Profile: software
- Depends on: S-017
- Inputs: —
- Actions:
1. `make clean test` three times in a row; all runs identical and green.
2. `make verify-freeze`.
3. Record `avr-size` and the SHA-256 of both `.hex` files in `evidence/S-018.txt`.
4. Commit: `git add -A && git commit -m "S-018: firmware release candidate"`; update `.checkpoints/state.json`.
- Outputs: release-candidate firmware images.
- Evidence produced: `evidence/S-018.txt`
- Done when: all automated tests
- Checkpoint: step S-018 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: a flaky test → D-024 (find the nondeterminism in the implementation). Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-024

### S-019 Bench trigger simulator
- Tier: Sonnet
- Profile: software
- Depends on: S-018
- Inputs: D-020 edge model
- Actions:
1. Create `tools/trigsim/trigsim.ino` for a second Arduino (Uno/Nano): outputs the LEAD/TRAIL waveform of `sim_cycles()` on one pin (open-drain emulation), rpm set over serial (`rpm 3000`), plus a "kick" profile (one cycle at 250 rpm).
2. Build it with `arduino-cli compile --fqbn arduino:avr:nano` (install arduino-cli per ENVIRONMENT.md if missing).
3. Commit: `git add -A && git commit -m "S-019: bench trigger simulator"`; update `.checkpoints/state.json`.
- Outputs: bench trigger simulator.
- Evidence produced: `evidence/S-019.txt` (compile log). Functional proof is B-404.
- Done when: all automated tests (unchanged); compiles cleanly.
- Checkpoint: step S-019 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: arduino-cli unavailable → write it as avr-gcc C with the same behaviour. Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-020

### S-002 Bike survey (HUMAN)
- Tier: Sonnet
- Profile: software, software.deploys
- Depends on: S-001
- Inputs: the motorcycle, D-021
- Actions:
1. HUMAN: read the magneto/magdyno plate; photograph the breaker end with the cover off.
2. HUMAN: fit a degree disc; find TDC (compression) with the piston-stop method; mark TDC and 50° BTDC on the disc.
3. HUMAN: count spindle turns for two crank turns (D-021a); note spindle rotation direction seen from the breaker end (D-021c).
4. HUMAN: measure the original timing with the points: angle at which the points open at full advance (record; compare with C-002).
5. HUMAN: note earth polarity, battery voltage, regulator type, presence of an auto-advance unit, presence of the cut-out terminal (D-021b, e, f).
6. Agent: write `hardware/SURVEY.md` with these values and the D-021 decisions they trigger.
7. Commit: `git add -A && git commit -m "S-002: bike survey"`; update `.checkpoints/state.json`.
- Outputs: `hardware/SURVEY.md`
- Evidence produced: `hardware/SURVEY.md`, photos under `evidence/photos/`
- Done when: none
- Checkpoint: step S-002 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: a measurement impossible → record why; apply the default rule. Rollback: none needed (no change to the bike).
- Gate: none
- Relevant decisions/claims: D-021, C-001, C-002, C-003, C-019, C-022, C-034

### S-003 Electrics preparation (HUMAN)
- Tier: Haiku
- Profile: software.deploys
- Depends on: S-002
- Inputs: SURVEY.md, D-007, D-021b
- Actions:
1. HUMAN: if positive earth, convert to negative earth (battery leads, repolarise dynamo, N-type electronic regulator) and check the charging voltage 7.0–7.5 V (6 V) or 13.8–14.6 V (12 V) at 2000 rpm.
2. HUMAN: load-test the battery.
3. Agent: record values in `hardware/SURVEY.md`.
4. Commit: `git add -A && git commit -m "S-003: electrics prepared"`; update `.checkpoints/state.json`.
- Outputs: healthy negative-earth supply.
- Evidence produced: SURVEY.md "Electrics" section
- Done when: none
- Checkpoint: step S-003 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: charging out of range → fix the charging system first (outside this project), then resume. Rollback: reverse the polarity change (documented steps in OPERATIONS.md).
- Gate: none
- Relevant decisions/claims: D-007, D-021, C-022

### S-004 Parts ordering (HUMAN)
- Tier: Haiku
- Profile: software.deploys
- Depends on: S-002
- Inputs: hardware/BOM.md, D-023
- Actions:
1. Agent: check stock for every BOM line at two UK distributors (e.g. Farnell, RS, Mouser UK, DigiKey UK); write `hardware/ORDER.md` with the chosen part numbers.
2. HUMAN: order.
3. Commit: `git add -A && git commit -m "S-004: parts ordered"`; update `.checkpoints/state.json`.
- Outputs: parts on hand.
- Evidence produced: `hardware/ORDER.md`
- Done when: none
- Checkpoint: step S-004 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-023. Rollback: n/a.
- Gate: none
- Relevant decisions/claims: D-023, C-012, C-013, C-015

### S-005 Controller build (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-004
- Inputs: hardware/WIRING.md §2
- Actions:
1. HUMAN: build the controller on perfboard exactly per the netlist; star ground at Q1 emitter.
2. HUMAN: B-401 (netlist continuity) and B-402 (power-up) from tests/bench/BENCH.md.
3. Commit: `git add -A && git commit -m "S-005: controller built"`; update `.checkpoints/state.json`.
- Outputs: working controller hardware (not yet flashed).
- Evidence produced: `evidence/B-401.md`, `evidence/B-402.md`
- Done when: B-401, B-402
- Checkpoint: step S-005 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: per BENCH.md remedies. Rollback: n/a.
- Gate: none
- Relevant decisions/claims: D-008, D-015, C-025

### S-006 Trigger disc and sensor (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-002, S-004
- Inputs: WIRING.md §1, SURVEY.md
- Actions:
1. HUMAN: remove the contact-breaker plate (keep all original parts bagged and labelled — rollback kit).
2. HUMAN: turn an acetal disc to fit the spindle taper/centre bolt; drill two Ø5 mm pockets at the D-021a spacing; press in the SmCo magnets (S out = lead, N out = trail); retain with high-temperature epoxy.
3. HUMAN: make an aluminium bracket for the A1220 on the housing; set 1.0–1.5 mm gap.
4. HUMAN: B-403.
5. Commit: `git add -A && git commit -m "S-006: trigger fitted"`; update `.checkpoints/state.json`.
- Outputs: trigger assembly.
- Evidence produced: `evidence/B-403.md`, photos
- Done when: B-403
- Checkpoint: step S-006 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: edges not within ±1° → re-position the sensor bracket; magnets reversed → flip. Rollback: refit the original breaker plate.
- Gate: none
- Relevant decisions/claims: D-004, D-021, C-010, C-011

### S-007 Fuses and flashing = staging deploy (HUMAN)
- Tier: Haiku
- Profile: software.deploys
- Depends on: S-005, S-018
- Inputs: firmware images from S-018
- Actions:
1. HUMAN: connect the USBasp to J3; agent/human run `make -C firmware fuses` then `make -C firmware VARIANT=TCI flash`.
2. Connect the tuning lead; `status` must reply; `get lead` → `lead=5000`.
3. Enter survey values: e.g. `set cyclediv 1` if D-021a says so; `save`.
4. Commit: `git add -A && git commit -m "S-007: controller flashed"`; update `.checkpoints/state.json`.
- Outputs: flashed controller.
- Evidence produced: `evidence/S-007.txt` (avrdude log, status reply)
- Done when: B-413 steps 1–2 pass
- Checkpoint: step S-007 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: avrdude cannot see the chip → check ISP wiring, slow the ISP clock (`-B 32`). Rollback: reflash previous image.
- Gate: none
- Relevant decisions/claims: D-003, D-015, C-024

### S-008 Bench validation (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-007, S-019
- Inputs: tests/bench/BENCH.md
- Actions:
1. HUMAN + agent: run B-404 … B-409 on the bench with the trigger simulator, a bench PSU and the real coil + a 7 mm spark gap.
2. Commit: `git add -A && git commit -m "S-008: bench validation"`; update `.checkpoints/state.json`.
- Outputs: all bench tests pass.
- Evidence produced: `evidence/B-404.md` … `evidence/B-409.md`
- Done when: B-404, B-405, B-406, B-407, B-408, B-409
- Checkpoint: step S-008 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: per BENCH.md remedies; otherwise BLOCKED. Rollback: n/a.
- Gate: none
- Relevant decisions/claims: D-019, D-024

### S-009 KiCad schematic (optional)
- Tier: Sonnet
- Profile: software
- Depends on: S-005
- Inputs: WIRING.md
- Actions:
1. Draw the KiCad 8 schematic `hardware/kicad/ariel-ign.kicad_sch` matching the netlists; run `kicad-cli sch erc`.
2. Export the netlist and compare net-by-net with WIRING.md (script or manual table in evidence).
3. Commit: `git add -A && git commit -m "S-009: KiCad schematic"`; update `.checkpoints/state.json`.
- Outputs: schematic (optional deliverable).
- Evidence produced: `evidence/S-009-erc.txt`
- Done when: ERC clean; B-401 comparison
- Checkpoint: step S-009 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: KiCad unavailable → skip; WIRING.md stays authoritative (default rule). Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-015

### S-010 Installation on the bike (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-003, S-006, S-008
- Inputs: plan/OPERATIONS.md "Installation"
- Actions:
1. HUMAN: install per OPERATIONS.md: controller box, coil, harness, SW1/SW2; short the magneto primary to earth at the cut-out terminal; disconnect the magneto HT pick-up and earth it.
2. HUMAN: static timing check with the LED (OPERATIONS.md).
3. Agent: prepare the G-002 request (GATES.md) with evidence links.
4. Commit: `git add -A && git commit -m "S-010: installed on bike"`; update `.checkpoints/state.json`.
- Outputs: installed, not yet started.
- Evidence produced: `evidence/S-010.md`
- Done when: static timing check within ±1°
- Checkpoint: step S-010 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: static check off → adjust the sensor bracket (S-006). Rollback: OPERATIONS.md "Rollback to magneto".
- Gate: G-002
- Relevant decisions/claims: D-021, D-026

### S-021 First start and strobe check = production deploy (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-010, G-002 approved
- Inputs: OPERATIONS.md "First start"
- Actions:
1. HUMAN: start with the decompressor procedure; B-410 (strobe) at idle and 3000 rpm; adjust `trim`/`latency` per B-410.
2. HUMAN: 10 km ride on map 0 below 4000 rpm; note any pinking.
3. Commit: `git add -A && git commit -m "S-021: first start and strobe check"`; update `.checkpoints/state.json`.
- Outputs: running engine, verified timing.
- Evidence produced: `evidence/B-410.md`
- Done when: B-410
- Checkpoint: step S-021 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-025. Rollback: OPERATIONS.md rollback.
- Gate: none
- Relevant decisions/claims: D-025, D-026, C-005

### S-022 Road tuning (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: S-021
- Inputs: D-025
- Actions:
1. HUMAN: raise map 0 points in 100 cdeg steps from 2400 rpm up, one step per ride, listening for pinking under load in top gear; stop at the first sign and back off 200 cdeg.
2. Agent: record each change (`get curve 0`) in `evidence/S-022.md`.
3. Commit: `git add -A && git commit -m "S-022: road tuning"`; update `.checkpoints/state.json`.
- Outputs: tuned map 0.
- Evidence produced: `evidence/S-022.md`
- Done when: B-410 re-run after the final change
- Checkpoint: step S-022 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: D-025. Rollback: `defaults` or re-enter the previous curve from evidence.
- Gate: G-004 (before any value above 3600 cdeg)
- Relevant decisions/claims: D-025, C-002, C-006

### S-020 MAGBREAK feasibility spike (HUMAN)
- Tier: Opus
- Profile: software
- Depends on: S-018, S-019
- Inputs: D-016, D-022, a spare or the bike's magneto on the bench
- Actions:
1. HUMAN: drive the magneto spindle from a variable-speed drill/lathe with a tachometer; breaker removed; Q2 (ignition IGBT with the D-015 gate network) across P–earth, switched by the flashed MAGBREAK controller fed from a bench supply, trigger from the real disc.
2. Measure: firing-reversal primary current polarity and waveform (current probe or 0.1 Ω shunt), spark on a 7 mm three-point gap vs break angle (sweep `lead`/curve) at 150/500/1500/3000 spindle rpm.
3. Build HF-A on a breadboard; measure boot time from empty and available current at 150/300 rpm; if HF-A fails, build HF-B and repeat.
4. Agent: write `research/spikes/magbreak/REPORT.md` with the D-022 criteria table.
5. Commit: `git add -A && git commit -m "S-020: MAGBREAK feasibility spike"`; update `.checkpoints/state.json`.
- Outputs: D-022 (a)(b)(c) results.
- Evidence produced: `research/spikes/magbreak/REPORT.md`
- Done when: none (spike); criteria reported
- Checkpoint: step S-020 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: unsafe HV behaviour → stop, report as (a) failed. Rollback: refit the magneto's breaker.
- Gate: G-003
- Relevant decisions/claims: D-016, D-022, C-004, C-016, C-018, C-019

### S-023 MAGBREAK build and first start (HUMAN)
- Tier: Sonnet
- Profile: software.deploys
- Depends on: G-003 approved (BL-1)
- Inputs: G-003 decision (frozen HF values), WIRING.md §3
- Actions:
1. HUMAN: build the MAGBREAK controller with the selected harvest front end; B-401/B-402 adapted (no battery: bench-drive the magneto).
2. HUMAN: B-411 (battery-less first-kick boot) on the bench; install; G-006; first start; B-410.
3. Commit: `git add -A && git commit -m "S-023: MAGBREAK build and first start"`; update `.checkpoints/state.json`.
- Outputs: battery-less ignition running.
- Evidence produced: `evidence/B-411.md`, `evidence/B-410-magbreak.md`
- Done when: B-411, B-410
- Checkpoint: step S-023 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: B-411 fails → return to G-003 with the data. Rollback: refit breaker plate and points (OPERATIONS.md).
- Gate: G-006 (before first start)
- Relevant decisions/claims: D-016, D-022, D-026

### S-024 Final documentation and rollback drill
- Tier: Haiku
- Profile: software, software.deploys
- Depends on: S-022 (and S-023 if run)
- Inputs: evidence/
- Actions:
1. Update README "Status" with measured values (final curve, timing error, current draw); B-412 rollback drill result.
2. `make verify-freeze && make test`.
3. Commit: `git add -A && git commit -m "S-024: final documentation"`; update `.checkpoints/state.json`.
- Outputs: final documentation.
- Evidence produced: `evidence/B-412.md`
- Done when: all automated tests; B-412; B-413 steps 3–4
- Checkpoint: step S-024 done; resume = re-run the step (all steps are idempotent: they rewrite their outputs; HUMAN steps resume from the first measurement not yet in the evidence file)
- On failure: — Rollback: `git revert`.
- Gate: none
- Relevant decisions/claims: D-026
