# HANDOFF — Ariel 350 electronic ignition

**Purpose.** Build a compact, programmable electronic ignition for a magneto-fired
Ariel NH 350 single, in two variants: **TCI** (battery) and **MAGBREAK**
(battery-less, retained magneto, gated). The branch contains the frozen tests,
interface headers with stubs, the wiring specification and the step plan. You
implement the firmware and guide the physical build; you never change frozen tests.

**Active profiles:** software, software.deploys (plan/PROFILE.md).

**Reading order:** this file → README.md → plan/PROFILE.md → plan/DECISIONS.md
→ plan/PLAN.md → plan/GATES.md → hardware/WIRING.md → hardware/BOM.md →
tests/bench/BENCH.md → plan/OPERATIONS.md → premortem/RISK_REGISTER.md.

**Environment setup** (plan/ENVIRONMENT.md):
```
sudo apt-get install -y build-essential gcc-avr avr-libc binutils-avr avrdude simavr libsimavr-dev libelf-dev python3
```
**Run the frozen suite:** `make test` (unit T-001…T-016, simulation T-202…T-222,
ELF checks T-301/T-302). Before implementation: 31 Unity failures plus T-302 failures.
**Verify the freeze:** `make verify-freeze` (= `sha256sum -c tests/FROZEN_MANIFEST.sha256`).

**Steps at a glance:** S-001 baseline → S-011…S-014 pure C modules → S-015 HAL →
S-016 TCI runtime → S-017 MAGBREAK → S-018 release candidate → S-019 trigger
simulator → S-002…S-008 survey, parts, build, trigger, flash, bench → S-009 KiCad
(optional) → S-010 install → **G-002** → S-021 first start → S-022 tuning (**G-004**
above 36°) → S-024 docs. MAGBREAK: S-020 spike → **G-003** → S-023 (**G-006**).

**HUMAN steps.** Steps marked HUMAN need physical work by the owner. You give the
literal instructions, then record the owner's measurements in `evidence/<ID>.md`.
A HUMAN step is done only when its evidence file holds every value its "Done when"
names. Never invent or estimate a measurement.

**Evidence.** Create `evidence/` at the repo root; one file per step or bench test
as named in PLAN.md; commit it with the step.

**Human gates.** At G-002, G-003, G-004, G-006 halt, write `GATE-<id>.md` using
plan/GATES.md (evidence bundle, questions verbatim), and wait for one of the
allowed responses.

**Halt / deviation protocol.**
* Unanticipated situation → the default rule in plan/DECISIONS.md; log in `DEVIATIONS.md`.
* Anything touching frozen tests, security, data integrity, a public interface,
  research integrity or engine-start safety → halt, write `BLOCKED.md`.
* A frozen test you believe is wrong → halt, write `TEST_CHALLENGE.md` (item ID,
  evidence, proposed fix); the planning protocol is then re-run from Phase 0.

**Integrity rule (Rule 9, verbatim):**
> **Integrity** `[All]`: No step may fabricate, cherry-pick without disclosure, or manually alter data, test results, benchmarks, or figures. In addition:
> * `[computational, publication]` Every reported number is generated from committed results, not transcribed by hand.
> * `[publication]` Generative-AI images are never used as data figures. AI assistance is disclosed according to the venue's policy, as recorded in `plan/ASSUMPTIONS.md`.

**Operations:** installation, first start, monitoring (`status`, LED), tuning,
stopping and rollback to the magneto: plan/OPERATIONS.md.
