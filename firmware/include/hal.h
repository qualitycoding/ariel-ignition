/* hal.h — hardware abstraction (AVR only; not compiled for host).
 * Pin map, timer usage and fuses frozen by D-015:
 *   PB0 / ICP1 (D8)  trigger input, A1220 latch output, 4k7 pull-up to +5 V.
 *                    Falling edge = LEAD (south magnet), rising = TRAIL (north).
 *                    Level LOW between LEAD and TRAIL (the latch holds its state).
 *   PB1 / OC1A (D9)  output stage. TCI: IGBT gate via 220R; HIGH = coil on,
 *                    driven LOW = spark, input (Hi-Z) = soft shutdown through
 *                    the 470k/10n gate network. MAGBREAK: HIGH = breaker closed.
 *   PB2 (D10)        MAGBREAK harvest-isolate control (reserved, held LOW).
 *   PB5 (D13)        status LED.
 *   PD0/PD1          UART 38400 8N1 (tuning lead).
 *   PD2 (D2)         kill input, active low, external 10k pull-up R8.
 *   PD3 (D3)         map select, grounded = map 1, external 10k pull-up R10
 *                    (no internal pull-up: see DEVIATIONS.md DEV-001).
 *   ADC6 (A6)        supply sense, 68k/4k7 divider, internal 1.1 V reference.
 *   Timer1           clk/8 = 1 us ticks at 8 MHz; ICP1 noise canceller on;
 *                    extended to 32 bits with the overflow interrupt.
 *                    OC1A compare = hardware spark edge; OC1B compare = software
 *                    event timer. Timer0 = 1 kHz housekeeping tick.
 *
 * Output-pin rules (they prevent accidental sparks):
 *   * While OC1A is connected (COM1A != 0) the pin follows the OC1A latch, not
 *     PORTB; PORTB1 is kept at 1 during that time.
 *   * To leave the connected state, first write PORTB, then disconnect.
 *   * To reach Hi-Z: clear DDRB1, immediately clear PORTB1 (no pull-up), then
 *     disconnect. */
#ifndef ARIEL_HAL_H
#define ARIEL_HAL_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include "common.h"

#define OUT_BIT   _BV(PB1)
#define COM1A_MSK (_BV(COM1A1) | _BV(COM1A0))

void     hal_init(void);
uint32_t hal_now_us(void);                 /* main or ISR context           */
uint32_t hal_extend_us(uint16_t t16);      /* ISR only: extend a value just captured/read */
uint16_t hal_vbat_from_adc(uint16_t adc);  /* adc * 1100 * 72700 / (4700 * 1024) mV       */

/* ---- trigger input ---------------------------------------------------- */
static inline bool hal_trig_low(void) { return !(PINB & _BV(PB0)); }
/* Choose the capture edge that follows the current pin level, and clear a
 * spurious flag caused by changing the edge select. */
static inline void hal_ic_follow_pin(void)
{
    if (PINB & _BV(PB0)) TCCR1B &= (uint8_t)~_BV(ICES1); else TCCR1B |= _BV(ICES1);
    TIFR1 = _BV(ICF1);
}

/* ---- output stage ----------------------------------------------------- */
static inline void hal_out_high(void)       /* coil on / breaker closed */
{
    TCCR1A &= (uint8_t)~COM1A_MSK; PORTB |= OUT_BIT;
}
static inline void hal_out_low(void)        /* spark by software / breaker open */
{
    PORTB &= (uint8_t)~OUT_BIT; TCCR1A &= (uint8_t)~COM1A_MSK; TIMSK1 &= (uint8_t)~_BV(OCIE1A);
}
static inline void hal_out_hiz(void)        /* TCI soft shutdown (D-019) */
{
    DDRB &= (uint8_t)~OUT_BIT; PORTB &= (uint8_t)~OUT_BIT;
    TCCR1A &= (uint8_t)~COM1A_MSK; TIMSK1 &= (uint8_t)~_BV(OCIE1A);
}
static inline void hal_out_drive_low(void)  /* after a soft shutdown */
{
    PORTB &= (uint8_t)~OUT_BIT; DDRB |= OUT_BIT;
}
/* Arm the hardware spark edge at 16-bit time t16. The pin MUST already be
 * high. Clear-on-match is connected through a forced set so that the OC1A
 * latch equals the pin (real silicon: the latch takes over the pin). */
static inline void hal_spark_arm(uint16_t t16)
{
    TIMSK1 &= (uint8_t)~_BV(OCIE1A);
    TCCR1A &= (uint8_t)~COM1A_MSK;
    OCR1A = t16;
    TIFR1 = _BV(OCF1A);
    TCCR1A |= COM1A_MSK;                    /* set on match ...          */
    TCCR1C = _BV(FOC1A);                    /* ... force: latch := 1     */
    TCCR1A &= (uint8_t)~_BV(COM1A0);        /* clear on match            */
    TIMSK1 |= _BV(OCIE1A);
}
static inline void hal_spark_cancel(void)   /* leave the pin HIGH */
{
    TIMSK1 &= (uint8_t)~_BV(OCIE1A); TCCR1A &= (uint8_t)~COM1A_MSK;
}
static inline void hal_spark_done(void)     /* after the hardware edge */
{
    PORTB &= (uint8_t)~OUT_BIT; TCCR1A &= (uint8_t)~COM1A_MSK; TIMSK1 &= (uint8_t)~_BV(OCIE1A);
}

/* ---- software event timer (OC1B, no pin) ------------------------------ */
static inline void hal_ev_set(uint16_t t16) { OCR1B = t16; TIFR1 = _BV(OCF1B); TIMSK1 |= _BV(OCIE1B); }
static inline void hal_ev_off(void)         { TIMSK1 &= (uint8_t)~_BV(OCIE1B); }
static inline uint16_t hal_tcnt(void)       { return TCNT1; }

/* ---- inputs ----------------------------------------------------------- */
static inline bool    hal_kill_raw(void)    { return !(PIND & _BV(PD2)); }
static inline uint8_t hal_map_pin(void)     { return (PIND & _BV(PD3)) ? 0u : 1u; }
static inline void    hal_led(bool on)      { if (on) PORTB |= _BV(PB5); else PORTB &= (uint8_t)~_BV(PB5); }
#endif
