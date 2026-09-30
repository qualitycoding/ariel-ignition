# Human gates (Rule 10, Phase 3.3)

At a gate the implementer halts, writes `GATE-<id>.md` (evidence bundle +
questions below, copied verbatim) and waits for the owner's written response.
Only the listed responses are valid; each maps to a plan branch.

G-001 `[math, computational]` — **N/A** (profiles not active).

## G-002 — First engine start / public release (irreversible, safety)
* **Trigger:** end of S-010 (before S-021). Also before any public release or
  tag of the firmware (not planned; if the owner asks for one, raise G-002 first).
* **Evidence bundle:** `evidence/S-018.txt` (all automated tests green, image
  hashes); `evidence/B-401.md`…`B-409.md`, `B-413.md` steps 1–2; `evidence/S-010.md`
  static timing; photo of the bagged original breaker parts (rollback kit);
  `hardware/SURVEY.md`.
* **Questions:** (1) Do you accept the bench evidence? (2) Is the rollback kit
  complete? (3) Start on map 0 or map 1?
* **Allowed responses → branch:** `proceed` → S-021 on map 0;
  `proceed-map1` → set `map 1`, `save`, then S-021; `proceed-with-rescope: <text>`
  → record in DEVIATIONS.md, apply, re-raise G-002; `stop` → OPERATIONS.md rollback, HALT.

## G-003 — Battery-less path selection
* **Trigger:** end of S-020.
* **Evidence bundle:** `research/spikes/magbreak/REPORT.md` with the D-022 (a)(b)(c)
  table, scope captures, measured harvest current and boot times.
* **Questions:** (1) Which battery-less path? (2) If BL-1: which harvest front end
  (HF-A or HF-B) with the component values in the report?
* **Allowed responses → branch:** `BL-1 HF-A` or `BL-1 HF-B` → freeze values in
  WIRING.md §3 as a new D-### (tests unaffected), then S-023;
  `BL-3` → new plan section required: HALT and re-run the protocol for the
  alternator conversion (out of scope of this plan); `BL-2` → same (re-plan);
  `stop` → battery-less work ends; battery TCI continues.

## G-004 — Advance above 36°
* **Trigger:** during S-022, before entering any curve value or `advmax` > 3600 cdeg.
* **Evidence bundle:** B-410 at the current setting; `evidence/S-022.md` ride log
  with fuel grade, ambient temperature and pinking observations.
* **Questions:** (1) Maximum advance allowed (cdeg, ≤ 4500)? (2) Fuel grade in use?
* **Allowed responses → branch:** `proceed: <max cdeg>` → continue S-022 up to that
  value; `stop` → keep the current curve, go to S-024.

## G-006 — First start on the MAGBREAK variant (irreversible, safety)
* **Trigger:** in S-023, after B-411 and installation, before the first kick.
* **Evidence bundle:** `evidence/B-411.md`, MAGBREAK bench evidence, T-220…T-222
  green, rollback kit (points, cam, condenser as found).
* **Questions:** (1) Accept the battery-less bench evidence? (2) Rollback kit complete?
* **Allowed responses → branch:** `proceed` → first start + B-410;
  `stop` → refit points (OPERATIONS.md), HALT MAGBREAK track.
