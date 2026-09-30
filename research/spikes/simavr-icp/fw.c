#include <avr/io.h>
#include <avr/interrupt.h>
ISR(TIMER1_CAPT_vect){
  uint16_t c = ICR1;
  TCCR1B ^= _BV(ICES1);           /* capture next opposite edge */
  PORTB |= _BV(PB1);               /* "dwell" on */
  OCR1A = c + 1000;                /* 1000 ticks = 1 ms @ 8MHz/8 */
  TCCR1A = _BV(COM1A1);            /* clear OC1A on compare = "spark" */
  TIFR1 = _BV(OCF1A);
}
int main(void){
  DDRB |= _BV(PB1);
  TCCR1A = 0; TCCR1B = _BV(CS11) | _BV(ICES1) | _BV(ICNC1);
  TIMSK1 = _BV(ICIE1);
  sei();
  for(;;){}
}
