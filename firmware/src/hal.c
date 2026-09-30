/* Hardware init and time base (D-015). AVR only. */
#include <avr/wdt.h>
#include "hal.h"

/* 32-bit microsecond clock from the 16-bit Timer1.
 * A wrap is detected by comparing each TCNT1 reading with the previous one, so
 * nothing depends on the overflow flag surviving while interrupts are disabled
 * (a wrap can fall inside a long ISR). The 1 ms tick and the overflow ISR both
 * read the clock, so two wraps can never pass unseen. Callers: interrupts off. */
static uint16_t t_hi, t_last;

static uint32_t clock_update(void)
{
    uint16_t t = TCNT1;
    if (t < t_last) t_hi++;
    t_last = t;
    return ((uint32_t)t_hi << 16) | t;
}

ISR(TIMER1_OVF_vect) { (void)clock_update(); }

/* Extend a 16-bit value taken from Timer1 within the last 65 ms (e.g. ICR1). */
uint32_t hal_extend_us(uint16_t t16)
{
    uint32_t n = clock_update();
    return n - (uint16_t)((uint16_t)n - t16);
}

uint32_t hal_now_us(void)
{
    uint8_t s = SREG;
    uint32_t v;
    cli();
    v = clock_update();
    SREG = s;
    return v;
}

/* 1100 mV * (68k + 4.7k) / 4.7k / 1024 = 16.6161 mV per LSB = 1088953 / 65536 */
uint16_t hal_vbat_from_adc(uint16_t adc)
{
    return (uint16_t)(((uint32_t)adc * 1088953UL) >> 16);
}

void hal_init(void)
{
    /* Fuses leave CKDIV8 programmed: 16 MHz / 8 = 2 MHz. Divide by 2 -> 8 MHz. */
    CLKPR = _BV(CLKPCE);
    CLKPR = 1;

    /* Pins: PB1 (output stage) and PB2 driven LOW, LED output, trigger input. */
    PORTB = 0;
    DDRB  = OUT_BIT | _BV(PB2) | _BV(PB5);
    PORTD = _BV(PD0);                                /* RX pull-up only; kill (R8) and map (R10) have external pull-ups */

    /* Timer1: normal mode, clk/8, noise canceller, falling edge first. */
    TCCR1A = 0;
    TCCR1B = _BV(CS11) | _BV(ICNC1);
    TIMSK1 = _BV(ICIE1) | _BV(TOIE1);
    hal_ic_follow_pin();

    /* Timer0: CTC, 8 MHz / 64 / 125 = 1 kHz. */
    TCCR0A = _BV(WGM01);
    OCR0A  = 124;
    TCCR0B = _BV(CS01) | _BV(CS00);
    TIMSK0 = _BV(OCIE0A);

    /* ADC6, 1.1 V reference, clk/64 (125 kHz). Two blocking conversions so
     * the first reading is valid before the first trigger edge. */
    ADMUX  = _BV(REFS1) | _BV(REFS0) | 6;
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);
    for (uint8_t i = 0; i < 2; i++) {
        ADCSRA |= _BV(ADSC);
        while (ADCSRA & _BV(ADSC)) { }
    }

    /* UART 38400 8N1, RX interrupt. */
    UBRR0  = (uint16_t)(F_CPU / (16UL * 38400UL) - 1UL);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

    wdt_enable(WDTO_250MS);       /* an ISR-driven tick keeps it alive; a hang resets to Hi-Z */
}
