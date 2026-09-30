# Deviations from the plan (implementation phase)

## DEV-001 — external pull-up on PD3 (map select); internal pull-ups on PD2/PD3 not used
- **Plan:** PD3 map switch to GND with the ATmega internal pull-up (D-015 pin map).
- **Found:** test T-211 grounds PD3 at t=0, before firmware init. simavr's pull-up model forces the pin
  high when firmware enables the internal pull-up, discarding the earlier external low (spike: /tmp spike,
  reproduced with a 10-line firmware). The frozen test and harness are unchanged.
- **Change:** PD3 gets an external 10 k pull-up R10 to +5 V (like R8 on the kill line); firmware enables no
  internal pull-up on PD2/PD3 (PD0/RX keeps its). Pin assignment is unchanged; only the pull-up source.
- **Why acceptable:** external pull-ups are at least as robust on a noisy ignition harness; +1 resistor.
- **Reversible:** re-enable PORTD bit 3 in hal.c if the simulator is fixed. Hardware change only.
- **Residual:** motivated by a simulator artefact; the design is nonetheless sound on real hardware.

## DEV-002 — wrap-safe clock instead of overflow-counter clock
- **Found:** with the original design (overflow ISR increments a high word), a Timer1 wrap that falls inside
  the long capture ISR is lost when TIFR1 is written; the clock is then 65.5 ms low (T-221 failed).
- **Change:** wraps are detected by comparing successive TCNT1 reads (1 ms tick + overflow ISR guarantee
  reads); captured values are reconstructed as `now - age`. No plan interface affected.
