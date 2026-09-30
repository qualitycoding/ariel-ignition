# Risk register (after pre-mortem round 2)

| ID | Description | Lens | Severity | Likelihood | Root cause | Traces to | Mitigation |
|---|---|---|---|---|---|---|---|
| R-001 | Default curve too advanced/retarded for this engine | invalid assumption | Medium | Medium | timing spec only from Tier 4 | C-002, A-011, D-012 | S-002 step 4, B-410, G-004, map 1, D-025 |
| R-002 | Magneto drive not half speed | invalid assumption | Low | Low | C-003 below bar | C-003 | D-021a, B-403 |
| R-003 | MAGBREAK cannot spark or self-power at kick speed | invalid assumption | Medium | Medium | C-004 below bar; untested topology | C-004, D-016 | S-020, D-022, G-003 branches |
| R-004 | Capacitor battery-less (BL-3) will not kick-start | invalid assumption | Medium | Medium | C-016 | C-016 | only via G-003 re-plan; limp mode T-209 |
| R-005 | No cut-out terminal access to the primary on this magneto | invalid assumption | Medium | Low | C-019 | C-019, D-021f | insulated brush lead (D-021f) |
| R-006 | Bike is positive earth and owner will not convert | operations | Medium | Low | C-022 | D-021b | documented BLOCKED halt |
| R-007 | EMI resets controller while riding | technical/ops | Medium | Low | HT near signal wiring | D-015, WIRING §4 | filter, screen, star ground, resistor cap, `resets` monitor, B-408, D-026c |
| R-008 | A frozen simulation test proves invalid | misinterpretation | Medium | Low | harness written before implementation | T-202…T-222 | spikes C-030/C-031, reference model, TEST_CHALLENGE |
| R-009 | simavr does not model the CKDIV8 start phase, so boot time is optimistic | technical | Low | Medium | simulator limit | D-020 | B-411, B-413 step 1 |
| R-010 | Reviews not independent (single agent) | integrity | Medium | Medium | no sub-agents | A-013 | owner review at G-002; physical tests |
| R-011 | Kickback from an early spark while cranking | technical | Medium | Low | firmware or mounting error | C-005, D-018 | physical TDC edge, T-202…T-204, B-403/B-404, G-002, D-025 |
| R-012 | Soft shutdown still produces a spark | technical | Medium | Low | gate network values | D-019 | B-406 + remedy |
| R-013 | Chosen part becomes obsolete | supply chain | Low | Medium | market | C-015 | D-023 ranked alternatives |
| R-014 | Power loss during `save` loses the tune | operations | Medium | Low | single EEPROM image | D-018.10 | CRC → defaults + LED alert; tune logged in evidence |
| R-015 | Malicious/garbled serial input changes timing | security | Low | Low | serial CLI | TM-1/TM-2 | physical access only, BUSY rule, T-013/T-015/T-016 |
| R-016 | Coil/IGBT overheating | performance | Low | Low | long dwell | D-018 | dwell caps, stall off, B-409 |
| R-017 | Brown-out during a kick on a weak 6 V battery | technical | Medium | Medium | LM2936 5.5 V minimum; sag | C-025 | 8 MHz clock, BOD 2.7 V, B-402/B-405, battery test S-003 |
| R-018 | HUMAN steps marked done without measurements | integrity | Medium | Low | agent executes plan | Rule 9 | evidence files as "Done when"; HANDOFF |
| R-019 | Toolchain drift changes test results | supply chain | Low | Low | apt updates | ENVIRONMENT.md | pinned versions, vendored Unity |
| R-020 | BODLEVEL mapping wrong | invalid assumption | Low | Low | C-024 partial | C-024 | B-402 steps 5, 7 |
| R-021 | Default rev limit unsuitable | ops | Low | Low | A-010 | A-010 | owner sets `revlimit` |
| R-022 | Magnet loosens or gap drifts | ops | Low | Low | vibration/heat | D-004 | SmCo, epoxy, maintenance check |

Totals: Critical 0, High 0, Medium 13, Low 9.
