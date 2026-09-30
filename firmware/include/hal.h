/* hal.h — hardware abstraction (AVR only; not compiled for host).
 * Pin map, timer usage and fuses frozen by D-015:
 *   PB0 / ICP1 (D8)  trigger input, A1220 latch output, 4k7 pull-up to +5 V.
 *                    Falling edge = LEAD (south magnet), rising = TRAIL (north).
 *   PB1 / OC1A (D9)  output stage. TCI: IGBT gate via 220R; HIGH = coil on,
 *                    driven LOW = spark, input (Hi-Z) = soft shutdown through
 *                    the 470k/10n gate network. MAGBREAK: HIGH = breaker closed.
 *   PB2 (D10)        MAGBREAK harvest-isolate control (reserved, see D-016).
 *   PB5 (D13)        status LED (on-board).
 *   PD0/PD1          UART 38400 8N1 (tuning lead).
 *   PD2 / INT0 (D2)  kill input, active low, internal + external pull-up.
 *   PD3 (D3)         map select, grounded = map 1.
 *   ADC6 (A6)        supply sense, 68k/4k7 divider, internal 1.1 V reference.
 *   Timer1           clk/8 = 1 us ticks at 8 MHz; ICP1 noise canceller on;
 *                    extended to 32 bits with the overflow interrupt. */
#ifndef ARIEL_HAL_H
#define ARIEL_HAL_H
#include "common.h"
void     hal_init(void);          /* clocks (CLKPR /2), Timer1, ICP, OC1A, ADC, UART, pins */
uint32_t hal_now_us(void);        /* 32-bit extended Timer1 time             */
uint16_t hal_vbat_mv(void);       /* last ADC6 sample: adc * 1100 * 72700 / (4700 * 1024) */
bool     hal_kill_active(void);   /* debounced kill input (PD2 low >= 20 ms) */
uint8_t  hal_map_select(void);    /* PD3: open = 0, grounded = 1             */
#endif
