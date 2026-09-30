# Assumptions (Rule 8: every inference recorded)

| ID | Assumption | Basis | Consequence if wrong | Checked by |
|---|---|---|---|---|
| A-001 | The bike is an Ariel NH (Red Hunter) 350 OHV single with a Lucas magneto/magdyno driven off the timing chest | user: "Ariel 350cc, magneto-fired"; C-001, C-033 | trigger mounting differs → D-021d | S-002 |
| A-002 | All intake defaults the user did not change were accepted (profiles, controller class, TCI, rankings order, USB-serial tuning, kick-safe start, alloy box, UK suppliers, rollback, licences) | user reply "all other items take their defaults" | — | — |
| A-003 | Both battery and battery-less electrics must be planned (user answer) | user reply | — | — |
| A-004 | The unit must be switchable off; the bike currently stops by stalling. Plan provides hard stop + kill button + stall behaviour | user reply | — | B-407 |
| A-005 | A manual valve lifter (decompressor) is used when kicking | user reply | crank speed lower → crank_dwell_max may need raising | B-405, S-021 |
| A-006 | Ranking order = applicability > availability > cost | intake default accepted | table order changes | — |
| A-007 | 6 V, negative earth after S-003 (polarity varies by bike, C-022) | default; D-021b | conversion step needed | S-002/S-003 |
| A-008 | No GitHub token reached the planner (the pasted value was the repo URL), so delivery is a git bundle for the owner to push | observed in the intake reply | — | Phase 5 |
| A-009 | The magneto is at half engine speed with one lobe (C-003); engine-speed drive handled by D-021a | C-003 (Tier 3/4) | cyclediv change only | S-002 |
| A-010 | Default rev limit 6000 rpm is conservative for a 1950s 350 single | no manufacturer figure found | owner adjusts `revlimit` | S-022 |
| A-011 | Original full-advance ≈ 38–41° crank (from 7/16–1/2 in BTDC) | C-002 (Tier 4 ×2), C-006 spike | curve shape changes; G-004 protects | S-002 step 4 |
| A-012 | Cost bands (£…££££) are indicative; no live prices were verified | planning-time | budget differs | S-004 |
| A-013 | Single-agent session: "independent" reviewers (cold read, pre-mortem red team) were simulated in the same context; recorded as tier substitution in state.json | environment has no sub-agents | reviews less independent (R-010) | owner review |
| A-014 | The builder can solder through-hole/DPAK parts and use a lathe or has access to one for the acetal disc | typical classic-bike owner | a machinist is needed | S-006 |
