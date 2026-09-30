# Question tree (software profile — Engineering branch)

Each leaf names what it informs. Status: ✔ answered (claim), ◐ answered below bar (risk), → deferred to a plan step with a decision rule.

- Q1 The engine and its original ignition
  - Q1.1 Bore/stroke of the NH 350? → C-001 ✔ (informs spike C-006, D-012)
  - Q1.2 Original full-advance timing? → C-002 ◐, C-006 ✔ (D-012 defaults, G-004); measured in S-002
  - Q1.3 Magneto drive speed and lobes? → C-003 ◐ → S-002 + D-021a
  - Q1.4 Auto-advance fitted? → C-034 → S-002 + D-021e
  - Q1.5 How is a Lucas magneto stopped / primary reached? → C-019 ◐, C-021 (D-016, D-021f)
- Q2 Trigger
  - Q2.1 Which sensor works from zero speed, hot, in a breaker housing? → C-010, C-011 ✔ (D-004)
  - Q2.2 Sensor latency relevance? → C-010/C-011 25 µs → `latency` parameter (D-012, B-410)
- Q3 Output stage
  - Q3.1 Available logic-level ignition IGBTs? → C-012, C-013 ✔ (D-005)
  - Q3.2 Smart driver with soft shutdown? → C-015 ✔ obsolete → D-019 gate network
- Q4 Controller
  - Q4.1 Speed grade at low voltage? → C-023 ✔ (D-003)
  - Q4.2 Fuses / brown-out? → C-024 ◐ → B-402 step 5/7
  - Q4.3 Regulator surviving automotive supply? → C-025 ✔ (WIRING.md)
- Q5 Battery-less options
  - Q5.1 Capacitor + electronic ignition viability? → C-016 ◐ (D-002 ranking, T-209 limp mode)
  - Q5.2 Alternator hazards without battery? → C-017 (BL-3 only)
  - Q5.3 Self-generating commercial units? → C-018 (reference)
  - Q5.4 Electronic breaker in the retained magneto (MAGBREAK) — spark window, harvest? → C-004 ◐ → S-020 + D-022 + G-003
- Q6 Safety of starting and stopping
  - Q6.1 Kickback avoidance → C-005 ◐ (D-018 fixed-TDC crank spark, T-202…T-204)
  - Q6.2 Stop methods → C-019, C-021 (SW1/SW2/stall, T-208, T-210, T-222)
- Q7 Electrics
  - Q7.1 Earth polarity / conversion → C-022 ◐ → S-003 + D-021b
- Q8 Test tooling
  - Q8.1 Can firmware be tested in the loop without hardware? → C-030, C-031 ✔ (D-020)
  - Q8.2 Test framework provenance → C-032 ✔
