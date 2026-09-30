Spike for C-030. Build: `avr-gcc -mmcu=atmega328p -DF_CPU=8000000UL -Os -o fw.elf fw.c && gcc -O1 -o h h.c -I/usr/include/simavr -I/usr/include/simavr/avr -lsimavr -lelf && ./h`.
Result (output.txt): PB1 goes high 5 us after each PB0 edge (ISR) and is cleared by OC1A exactly 1000 us after the capture.
