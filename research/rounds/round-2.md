# Round 2 — Synthesis, depth and empirical (2026-09-30)
R3 gaps: timing spec is given as piston travel, not degrees; need Tier 1 part data for
the trigger latch and a soft-shutdown driver; MCU low-voltage speed grade; regulator.
R4 depth: A1220 datasheet (C-011); FGBS3040E1 smart driver found (C-015) — the maker
lists it Obsolete → design changed to a gate-network soft shutdown (D-019);
FGD3040G2-F085V confirmed active (C-013); LM2936 (C-025); ATmega328P speed grade (C-023).
R6 spikes: timing-reference (C-006: 1/2 in = 40.4–41.6°, 7/16 in = 37.6–38.7°, rod
length barely matters); crc-check (C-020); simavr-icp (C-030); simavr-adc (C-031);
Unity byte-compare (C-032); avr-libc fuse bits (C-024, positions only).
Confidence changes: C-006, C-020, C-030, C-031, C-032 → verified.
