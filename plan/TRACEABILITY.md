# Traceability: success criteria → tests → steps

| SC | Success criterion (from the request + intake answers) | Automated tests | Bench / bike tests | Steps |
|---|---|---|---|---|
| SC-1 | Starts on the first or second kick (with decompressor), battery and battery-less | T-202, T-220 | B-405, B-411, S-021 | S-016, S-017, S-021, S-023 |
| SC-2 | Spark timing within ±1° crank on the bike (±0.5° bench) from cranking to 5500 rpm | T-005…T-010, T-205, T-206, T-221 | B-403, B-404, B-410 | S-013, S-016, S-021 |
| SC-3 | Programmable: two maps, curve and parameters editable over serial, stored in EEPROM, safe defaults on corruption | T-002…T-004, T-011…T-016, T-211 | S-007 | S-012, S-014, S-016 |
| SC-4 | Can be switched off: hard stop (SW1 / magneto cut-out), kill button, stalling stops it cleanly | T-208, T-210, T-222 | B-407 | S-016, S-017 |
| SC-5 | Kick-safe: fixed-TDC cranking spark, no spark on an aborted kick, limp mode at low voltage | T-202, T-203, T-204, T-209, T-210 | B-406 | S-016 |
| SC-6 | Rev limiter; no tuning while the engine turns | T-207, T-013 | — | S-014, S-016 |
| SC-7 | Small, compact, robust: fits the alloy box, resource budget, EMI and thermal proof | T-301, T-302 | B-401, B-402, B-408, B-409 | S-005, S-015, S-018 |
| SC-8 | Reversible: rollback to the original magneto in ≤ 30 min | — | B-412 | S-006, S-024 |

Test catalogue: T-001 CRC; T-002…T-004 config (T-004 operational: corrupt
storage); T-005…T-010 timing; T-011…T-014 CLI; T-015/T-016 security (malformed
input, fuzz); T-202…T-211 TCI firmware-in-the-loop (T-205/T-206 performance
timing); T-220…T-222 MAGBREAK; T-301 resources (performance), T-302 structure;
B-401…B-412 physical acceptance. Threat model: TM-1 serial port with physical
access only (no radio, no network); TM-2 malformed or hostile serial input →
T-015, T-016, and the BUSY rule (T-013) prevents mid-ride changes.
