#include <avr/io.h> 
#include <avr/interrupt.h> 

#define F_CPU 1000000
#include <util/delay.h> 
#include <avr/sleep.h>

#include "bjhutils.h"


#define BUTTON_DDR DDRB
#define BUTTON_PORT PORTB
#define BUTTON_PIN PINB
#define BUTTON_INC BIT(PB1)
#define BUTTON_DEC BIT(PB0)
#define BUTTONS (BUTTON_INC | BUTTON_DEC)

#define LED_PORT PORTC
#define LED_DDR DDRC

#define TIMER_THRESH OCR0A
//#define DEFAULT_TIMER_THRESH 49
//#define TIMER_THRESH_STEP 1

//#define TIMER_THRESH_PORT PORTD
//#define TIMER_THRESH_DDR DDRD

//ISR(TIMER0_COMPA_vect) {
//  LED_PORT = ~LED_PORT;
//}

uint8_t sampleButtons() {
  return ~(BUTTON_PIN & BUTTONS);  // invert button state so 1 means "pressed"
}

static uint8_t debounce0,debounce1,debouncedState;
// vertical stacked counter based debounce
uint8_t debounce(uint8_t sample)
{
  uint8_t delta, changes;
    
  // Set delta to changes from last sample
  delta = sample ^ debouncedState;
    
  // Increment counters
  debounce1 = debounce1 ^ debounce0;
  debounce0  = ~debounce0;
    
  // reset any unchanged bits
  debounce0 &= delta;
  debounce1 &= delta;
    
  // update state & calculate returned change set
  changes = ~(~delta | debounce0 | debounce1);
  debouncedState ^= changes;
    
  return changes;
}

#define ADC_PRESCALE_MASK (BIT(ADPS1) | BIT(ADPS0))  // Prescale clock by 8 -- 1M / 8 = 125k
#define ADC_CHANNEL_MASK (BIT(MUX0))  // ADC1
#define LED(reg) REG_PORT(reg, D)

int main(void) {
  // Set up ADC
  bit_set(ADCSRA, ADC_PRESCALE_MASK);  // Set clock prescale
  bit_set(ADMUX, BIT(REFS0));  // Set reference to AVcc
  bit_set(ADMUX, BIT(ADLAR));  // Set left-alignment for easy 8-bit reading of ADCH
  bit_set(ADMUX, ADC_CHANNEL_MASK); // Enable ADC channel (ADC1, in this case)
  bit_set(DIDR0, BIT(ADC1D));  // Disable digital input buffer on the ADC pin
  bit_set(ADCSRA, BIT(ADATE));  // Auto Trigger Enable  (default auto-trigger is free-running mode)
  bit_set(ADCSRA, BIT(ADEN));  // Enable ADC
  bit_set(ADCSRA, BIT(ADSC));  // Start conversions

  // set up counter 1 (16 bit) to act as a dual channel PWM generator
  // we want OC1A and B to be set on reaching BOTTOM, clear on reaching
  // compare match, use ICR1 as TOP and have a prescale of 8.  -- rewritten to assume 1MHz clock
  TCCR1A = BIT(COM1A1) // set OC1A/B at TOP
    | BIT(COM1B1) // clear OC1A/B when match
    | BIT(WGM11); // mode 14 (fast PWM, clear TCNT1 on match ICR1)
  TCCR1B = BIT(WGM13)
    | BIT(WGM12)
    | BIT(CS11); // timer uses main system clock / 8 
  ICR1 = 2500; // used for TOP, makes for 50 hz PWM
  OCR1A = 185; // servo at center
  OCR1B = 185; // servo at center
  DDRB = BIT(PB1)
    | BIT(PB2);  // have to set up pins as outputs

  LED(DDR) = B8(1111);
  LED(PORT) = B8(0);
  while (1) {
    OCR1A = ADCH + 20;  // Read ADC, offset into the servo range and use that to set the servo pulse-width
    LED(PORT) = (ADCH >> 4);  // Read ADC and divide by 16 to get 4-bit value (I only have 4 LEDs hooked up)
  }
}
