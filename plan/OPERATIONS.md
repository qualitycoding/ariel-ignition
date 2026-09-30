# Operations runbook (software.deploys)

## Installation (S-010), TCI
1. Battery disconnected. Keep every removed original part bagged and labelled (rollback kit).
2. Magneto: breaker plate replaced by the magnet disc (S-006). Connect the magneto
   cut-out terminal permanently to earth (primary shorted → the magneto can no
   longer spark). Remove the HT lead from the magneto pick-up and earth the pick-up
   with a short wire.
3. Mount the controller box on rubber, away from the HT lead (≥ 50 mm), Q1 wall
   facing airflow. Mount the coil; copper-core HT lead with a 5 kΩ cap to the plug.
4. Wire per WIRING.md §2 and §4; SW1 in the main feed; SW2 on the handlebar.
5. Static timing: SW1 on, engine in top gear, rotate the rear wheel forward slowly.
   The controller LED must come on at 50° ±1° BTDC and go off at TDC ±1° on the
   degree disc (compression stroke). Record in evidence/S-010.md.

## First start (S-021, after G-002)
1. Fuel on, `status` shows `mode=STOP`. Tickle the carb as usual.
2. Ease the engine onto compression, pull the valve lifter, move just past, release,
   let the kick-start return, and give a firm swing. The spark happens at TDC.
3. Idle 2 minutes; strobe check (B-410); stop with SW2 and check it stops at once.

## Monitoring
* `status` over the tuning lead: rpm, mode, advance, supply mV, map, reset count.
* LED: static timing aid when stopped; 2 blinks at power-up = defaults loaded
  (EEPROM invalid — re-enter your settings); fast blink = kill active.
* A non-zero `resets` after a ride means supply dips or EMI: run B-408.

## Tuning
Engine stopped (`rpm=0`), tuning lead connected: `get curve 0`, then
`curve 0 <i> <rpm> <cdeg>`, `save`. Changes above 36° need G-004.
Example: raise 3000 rpm to 31°: `curve 0 5 3000 3100`, `save`.

## Stopping
Normal: SW2 (kill) or SW1 (main switch). Stalling also stops it cleanly. Always
turn SW1 off when parked (the controller draws ~20 mA).

## Rollback to magneto (target ≤ 30 min, B-412)
1. SW1 off, battery disconnected.
2. Remove the magnet disc and sensor bracket; refit the original breaker plate,
   points and cam (or auto-advance unit as found in S-002).
3. Remove the earth wire from the cut-out terminal; refit the HT pick-up and HT lead.
4. Set the points gap and time the magneto to the S-002 recorded figure.
5. Unplug the controller at J1/J4 (harness can stay in place).
6. Positive-earth reversal (only if S-003 converted and the owner wants it back):
   swap battery leads, repolarise the dynamo, refit the P-type regulator.

## Maintenance
Every 5000 km: check sensor air gap (1.0–1.5 mm), magnet security, connector
seals; re-run B-410.
