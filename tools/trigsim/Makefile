# Builds for a 16 MHz ATmega328P (Pro Mini / Nano). Flash: avrdude -p m328p -c arduino -P <port> -b 57600 -U flash:w:trigsim.hex
trigsim.hex: trigsim.c
	avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -std=gnu11 -Wall -Wextra -Werror -o trigsim.elf trigsim.c
	avr-objcopy -O ihex trigsim.elf $@
	avr-size trigsim.elf
clean:
	rm -f trigsim.elf trigsim.hex
