# Firmware build (AVR). Pinned toolchain: plan/ENVIRONMENT.md
MCU      ?= atmega328p
F_CPU    ?= 8000000UL
VARIANT  ?= TCI            # TCI | MAGBREAK  (compile-time default variant, D-016)
CC        = avr-gcc
OBJCOPY   = avr-objcopy
CFLAGS    = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DDEFAULT_VARIANT_$(strip $(VARIANT)) \
            -Os -std=c11 -Wall -Wextra -Werror -Iinclude -ffunction-sections -fdata-sections
LDFLAGS   = -Wl,--gc-sections
SRC       = $(wildcard src/*.c)
BUILD     = build/$(strip $(VARIANT))

all: $(BUILD)/ariel_ign.hex
$(BUILD)/ariel_ign.elf: $(SRC) $(wildcard include/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
	avr-size $@
$(BUILD)/ariel_ign.hex: $(BUILD)/ariel_ign.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@
# Flash via ISP (USBasp); no bootloader (D-015: fast boot for battery-less start).

flash: $(BUILD)/ariel_ign.hex
	avrdude -p m328p -c usbasp -U flash:w:$<:i
clean:
	rm -rf build
.PHONY: all flash clean
# One-time fuse programming (D-015): lfuse 0x5F (ext. crystal, BOD-enabled
# fast start, CKDIV8 ON -> 2 MHz until firmware sets CLKPR /2 = 8 MHz),
# hfuse 0xD1 (no bootloader, EESAVE on), efuse 0xFD (BOD 2.7 V).
fuses:
	avrdude -p m328p -c usbasp -U lfuse:w:0x5F:m -U hfuse:w:0xD1:m -U efuse:w:0xFD:m
.PHONY: fuses
