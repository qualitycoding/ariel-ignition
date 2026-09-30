# Pre-mortem round 1 (2026-09-30)
Reviewer: same agent in a deliberately adversarial pass (no fresh-context agent
available — tier substitution, A-013, R-010).

## Incident report (software.deploys scenario)
*Six months after installation the Ariel is back on its magneto.* Sequence
reconstructed: (1) the default curve came from a forum-quoted figure; on a hot day
the engine pinked under load in top gear (R-001). (2) The owner lowered the
curve; weeks later the engine cut out twice in town — the controller had reset:
the trigger cable ran beside the HT lead and a solid-core lead had been fitted (R-007).
(3) During a failed kick the engine stopped just before TDC with the coil charged;
on stall timeout the firmware switched the coil off hard, the plug sparked and the
kick-start kicked back (R-011, R-012). (4) The battery-less variant was never
finished: the harvest circuit could not start the controller at kick speed and the
plan had no path beyond "MAGBREAK" (R-003). (5) The implementer found that one
simulation test could not pass because the harness treated the initial pin state
as a transition (R-008).

## Lens findings → register
Technical correctness: R-011, R-012, R-017. Dependency drift: R-013, R-019.
Invalid research assumptions (every single-source/inferred claim): R-001 (C-002),
R-002 (C-003), R-003 (C-004), R-004 (C-016), R-005 (C-019), R-006 (C-022),
R-020 (C-024), R-011 (C-005). Implementer misinterpretation: R-018, R-008.
Integrity: R-018. Scale/performance: R-016 (thermal), T-205/T-206 timing.
Security: R-015. Operations: R-007 (monitoring gap), R-014 (config loss), R-010.

## Critical/High found: R-011 (Critical), R-001, R-003, R-007, R-008, R-012 (High)
Hardening applied in this round (plan changed, tests unchanged):
* R-011: crank spark fixed by a physical TDC edge (no prediction) — D-004/D-018;
  added T-203/T-204/T-210 behaviour; B-403 at 300 rpm; G-002 before first start.
* R-012: soft shutdown specified (D-019) with bench proof B-406 and a remedy
  (C7 22 nF); rev limiter never aborts a started dwell (no soft-off spam, T-207).
* R-001: conservative defaults (34° peak, 4–7° under the original), measurement of
  the original timing in S-002, strobe B-410, advance > 36° only after G-004, map 1.
* R-003: D-022 measurable criteria, spike S-020, gate G-003 with BL-3/BL-2 branches
  (re-plan) — battery case unaffected.
* R-007: resistor cap/plug + copper lead mandated, screened trigger with 1k/1nF
  filter, single-point ground, BOD, `resets` counter in `status` (monitoring),
  B-408 EMI soak, D-026c response.
* R-008: harness mechanics proven by spikes (C-030, C-031); initial pin state is
  recorded without creating an event (harness `sample()` only logs transitions
  from a known state; initial state HIZ equals reset DDR); expected values derived
  from the reference model; red run shows every test failing on assertions only.
