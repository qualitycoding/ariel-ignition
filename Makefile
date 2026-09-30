# Top-level build & test entry points (plan/ENVIRONMENT.md pins every tool).
#   make test        -> all frozen automated tests (unit + sim + elf checks)
#   make test-unit   -> host unit tests T-001..T-016
#   make test-sim    -> simavr firmware-in-the-loop tests T-201..T-222
#   make test-elf    -> resource/structure checks T-301/T-302
#   make firmware    -> both variants (TCI, MAGBREAK)
#   make verify-freeze -> sha256sum -c tests/FROZEN_MANIFEST.sha256
HOSTCC   ?= gcc
HOSTFLAGS = -std=c11 -Wall -Wextra -Werror -O1 -DHOST_BUILD -DDEFAULT_VARIANT_TCI \
            -Ifirmware/include -Ivendor/unity
CORE      = firmware/src/crc16.c firmware/src/config.c firmware/src/timing.c firmware/src/cli.c
UNITY     = vendor/unity/unity.c
SIMINC    = -I/usr/include/simavr -I/usr/include/simavr/avr
OUT       = build/host
UNIT      = test_crc16 test_config test_timing test_cli

.PHONY: all test test-unit test-sim test-elf firmware verify-freeze clean
all: firmware
firmware:
	$(MAKE) -C firmware VARIANT=TCI
	$(MAKE) -C firmware VARIANT=MAGBREAK

$(OUT)/%: tests/unit/%.c $(CORE) $(UNITY) $(wildcard firmware/include/*.h)
	@mkdir -p $(OUT)
	$(HOSTCC) $(HOSTFLAGS) -o $@ $< $(CORE) $(UNITY)

test-unit: $(addprefix $(OUT)/,$(UNIT))
	@rc=0; for t in $(UNIT); do echo "== $$t"; ./$(OUT)/$$t || rc=1; done; exit $$rc

$(OUT)/sim_tci: tests/sim/test_sim_tci.c tests/sim/harness.c tests/sim/harness.h $(UNITY)
	@mkdir -p $(OUT)
	$(HOSTCC) -std=gnu11 -O1 -Wall -DUNITY_INCLUDE_DOUBLE -Ivendor/unity $(SIMINC) -o $@ tests/sim/test_sim_tci.c tests/sim/harness.c $(UNITY) -lsimavr -lelf
$(OUT)/sim_mag: tests/sim/test_sim_magbreak.c tests/sim/harness.c tests/sim/harness.h $(UNITY)
	@mkdir -p $(OUT)
	$(HOSTCC) -std=gnu11 -O1 -Wall -DUNITY_INCLUDE_DOUBLE -Ivendor/unity $(SIMINC) -o $@ tests/sim/test_sim_magbreak.c tests/sim/harness.c $(UNITY) -lsimavr -lelf

test-sim: firmware $(OUT)/sim_tci $(OUT)/sim_mag
	@rc=0; ./$(OUT)/sim_tci || rc=1; ./$(OUT)/sim_mag || rc=1; exit $$rc

test-elf: firmware
	@rc=0; for v in TCI MAGBREAK; do tests/avr/check_elf.sh firmware/build/$$v/ariel_ign.elf || rc=1; done; exit $$rc

test:
	@rc=0; $(MAKE) --no-print-directory test-unit || rc=1; \
	 $(MAKE) --no-print-directory test-sim || rc=1; \
	 $(MAKE) --no-print-directory test-elf || rc=1; exit $$rc

verify-freeze:
	sha256sum -c tests/FROZEN_MANIFEST.sha256

clean:
	rm -rf build firmware/build
