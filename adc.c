#include <avr/io.h> 
//#include <avr/interrupt.h> 

#define F_CPU 1000000  // Default for ATmega168
//#include <util/delay.h> 
//#include <avr/sleep.h>

#include "bjhutils.h"

#define LED(reg) REG_PORT(reg, D)

// RF: might be worth pulling a lot of this ADC code out into a library and defining all the variations for prescale, etc.
//#define ADC_PRESCALE_MASK (BIT(ADPS2) | BIT(ADPS1) | BIT(ADPS0))  // Prescale clock by 128 // FIXME: this is wrong for my clock rate
#define ADC_PRESCALE_MASK (BIT(ADPS1) | BIT(ADPS0))  // Prescale clock by 8 -- 1M / 8 = 125k


int main(void) {
  LED(DDR) = B8(1111);
  LED(PORT) = B8(0);

  // Set up ADC
  bit_set(ADCSRA, ADC_PRESCALE_MASK);  // Set clock prescale
  bit_set(ADMUX, BIT(REFS0));  // Set reference to AVCC
  bit_set(ADMUX, BIT(ADLAR));  // Set left-alignment for easy 8-bit reading of ADCH
  // MUX setting for other than ADC0?
  //bit_set(ADCSRA, BIT(ADFR));  // Set ADC to free-running mode  -- FIXME: dunno where this comes from datasheet says ADCSRB ADTS[2:0] = 0 for free-running
  bit_clear(ADCSRB, (BIT(ADTS2) | BIT(ADTS1) | BIT(ADTS0)));
  bit_set(ADCSRA, BIT(ADEN));  // Enable ADC
  bit_set(ADCSRA, BIT(ADSC));  // Start conversions

  while (1) {
    LED(PORT) = (ADCH >> 4);  // Read ADC and divide by 16 to get 4-bit value (I only have 4 LEDs hooked up)
    // REM: could totally delay away some ms here...
  }
}
