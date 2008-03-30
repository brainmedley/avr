// Simple ADC example, targets ATmega168

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
#define ADC_CHANNEL_MASK (BIT(MUX0))  // ADC1

int main(void) {
  LED(DDR) = B8(1111);
  LED(PORT) = B8(0);

  // Set up ADC
  bit_set(ADCSRA, ADC_PRESCALE_MASK);  // Set clock prescale
  bit_set(ADMUX, BIT(REFS0));  // Set reference to AVcc
  bit_set(ADMUX, BIT(ADLAR));  // Set left-alignment for easy 8-bit reading of ADCH
  bit_set(ADMUX, ADC_CHANNEL_MASK); // Enable ADC channel (ADC1, in this case)
  bit_set(DIDR0, BIT(ADC1D));  // Disable digital input buffer on the ADC pin
  bit_set(ADCSRA, BIT(ADATE));  // Auto Trigger Enable  (default auto-trigger is free-running mode)
  bit_set(ADCSRA, BIT(ADEN));  // Enable ADC
  bit_set(ADCSRA, BIT(ADSC));  // Start conversions

  while (1) {
    LED(PORT) = (ADCH >> 4);  // Read ADC and divide by 16 to get 4-bit value (I only have 4 LEDs hooked up)
    // REM: could totally delay away some ms here...
  }
}
