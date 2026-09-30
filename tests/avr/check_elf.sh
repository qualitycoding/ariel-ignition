#!/bin/sh
# FROZEN — DO NOT MODIFY. Hash recorded in tests/FROZEN_MANIFEST.sha256 (Rule 2E).
# T-301 (performance/resources) + T-302 (structure). Enforces D-015, SC-7.
# Usage: tests/avr/check_elf.sh firmware/build/<VARIANT>/ariel_ign.elf
set -eu
ELF="$1"; fail=0
[ -f "$ELF" ] || { echo "T-301 FAIL: $ELF missing"; exit 1; }
set -- $(avr-size -A "$ELF" | awk '/^\.text/{t=$2}/^\.data/{d=$2}/^\.bss/{b=$2}/^\.noinit/{n=$2}END{print t+0, d+0, b+0, n+0}')
FLASH=$(( $1 + $2 )); RAM=$(( $2 + $3 + $4 ))
echo "flash=$FLASH ram=$RAM"
[ "$FLASH" -le 30720 ] || { echo "T-301 FAIL: flash $FLASH > 30720"; fail=1; }
[ "$RAM" -le 1536 ]    || { echo "T-301 FAIL: static RAM $RAM > 1536"; fail=1; }
# T-302: required interrupt handlers present (D-015): ICP1, OC1A, OC1B, Timer1 overflow.
for v in __vector_10 __vector_11 __vector_12 __vector_13; do
  avr-nm "$ELF" | grep -q " T $v$" || { echo "T-302 FAIL: handler $v missing"; fail=1; }
done
[ $fail -eq 0 ] && echo "T-301/T-302 PASS"
exit $fail
