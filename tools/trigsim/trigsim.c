/* trigsim — bench trigger simulator for the Ariel ignition (plan step S-019).
 * Runs on a SPARE 16 MHz ATmega328P (Pro Mini / Nano), NOT on the ignition board.
 * Output PB0 (D8) emulates the A1220 latch: HIGH, LOW from LEAD to TRAIL, HIGH again.
 * Connect D8 -> ignition PB0 (D8) and GND -> GND. Serial 38400 8N1.
 *   r<rpm>\n   set engine speed, 50..8000 crank rpm (default 3000), e.g. "r3000"
 *   s\n        toggle a slow sweep 250 -> 6000 -> 250 rpm
 *   x\n        stop (hold output HIGH)
 *   w<deg>\n   lead-to-trail window in crank degrees (default 50)
 * One trigger pair per 720 deg of crank (cycle_div = 2). */
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>

#define TICKS_PER_US 2u                     /* Timer1 clk/8 at 16 MHz */
static uint16_t hi, last;

static uint32_t now_us(void)
{
    uint16_t t = TCNT1;
    if (t < last) hi++;
    last = t;
    return (((uint32_t)hi << 16) | t) / TICKS_PER_US;
}

static uint16_t rpm = 3000, window_deg = 50;
static uint8_t running = 1, sweep;

static void poll_serial(void)
{
    static char b[8]; static uint8_t n;
    while (UCSR0A & _BV(RXC0)) {
        char c = (char)UDR0;
        if (c == '\n' || c == '\r') {
            b[n] = 0;
            if (n) {
                if (b[0] == 'r') { long v = atol(b + 1); if (v >= 50 && v <= 8000) { rpm = (uint16_t)v; running = 1; sweep = 0; } }
                else if (b[0] == 's') { sweep = !sweep; running = 1; }
                else if (b[0] == 'x') running = 0;
                else if (b[0] == 'w') { long v = atol(b + 1); if (v >= 10 && v <= 90) window_deg = (uint16_t)v; }
            }
            n = 0;
        } else if (n < sizeof b - 1) b[n++] = c;
    }
}

static void wait_until(uint32_t t)
{
    while ((int32_t)(t - now_us()) > 0) poll_serial();
}

int main(void)
{
    uint32_t t0;
    int16_t s_rpm = 250; int8_t dir = 1;
    DDRB |= _BV(PB0); PORTB |= _BV(PB0);
    TCCR1B = _BV(CS11);
    UBRR0 = 25; UCSR0B = _BV(RXEN0); UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);   /* 38400 @16 MHz */
    t0 = now_us() + 1000;
    for (;;) {
        uint32_t period, win;
        uint16_t r = sweep ? (uint16_t)s_rpm : rpm;
        poll_serial();
        if (!running) { PORTB |= _BV(PB0); t0 = now_us() + 1000; continue; }
        period = 120000000UL / r;                         /* 720 deg in us */
        win = (uint32_t)((uint64_t)period * window_deg / 720u);
        wait_until(t0);            PORTB &= (uint8_t)~_BV(PB0);   /* LEAD  (falling) */
        wait_until(t0 + win);      PORTB |= _BV(PB0);             /* TRAIL (rising)  */
        t0 += period;
        if (sweep) { s_rpm += dir * 50; if (s_rpm >= 6000) dir = -1; if (s_rpm <= 250) dir = 1; }
    }
}
