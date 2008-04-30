/*
Copyright (c) 2008 Benjamin Holt

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
 */

#define F_CPU 1000000  // ATmega168 default clock speed is 1 MHz
#include <util/delay.h> 
#include <avr/eeprom.h>
#include <avr/io.h> 

#include "bit_helpers.h"


static uint8_t debounce0, debounce1, debouncedState;
// vertical stacked counter based debounce
uint8_t debounce(uint8_t sample)
{
  uint8_t delta, changes;
  
  // Set delta to changes from last stable state
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


#define ADC_PRESCALE_MASK (BIT(ADPS1) | BIT(ADPS0))  // Prescale clock by 8 -- 1M / 8 = 125k (probably overkill for this project, but what the hey)
#define ADC_CHANNEL_MASK (BIT(MUX0))  // ADC1

#define BUTTON(reg) (reg##D)
#define BUTTON_SET BIT(PD6)  // TODO: find out if the PD is needed
#define BUTTON_GO BIT(PD7)
#define BUTTONS (BUTTON_SET | BUTTON_GO)

#define LED(reg) (reg##D)
#define LEDS B8(1111)

uint8_t sampleButtons() {
  return ~(bit_read(BUTTON(PIN), BUTTONS));  // invert button state so 1 means "pressed"
}

enum  { RUN_MODE = 0, SET_MODE = 1 };

// Declare and initialize non-volitile storage for the servo positions
uint8_t EEMEM positionAstore = 140;  // 135 should be somewhere around the middle...
uint8_t EEMEM positionBstore = 130;  // ...move a little to either side by default

void setServo(uint8_t position) {
  OCR1A = position + 50;  // fudge-factor to bump the ~0-255 range of the potentiometer up to straddle the reasonable range of timings for the servo PWM (servo range runs 125ish - 250ish)
}


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
  // compare match, use ICR1 as TOP and have a prescale of 8.
  TCCR1A = BIT(COM1A1) // set OC1A/B at TOP
    | BIT(COM1B1) // clear OC1A/B when match
    | BIT(WGM11); // mode 14 (fast PWM, clear TCNT1 on match ICR1)
  TCCR1B = BIT(WGM13)
    | BIT(WGM12)
    | BIT(CS11); // timer uses main system clock / 8 
  ICR1 = 2500; // used for TOP, makes for 50 hz PWM
  setServo(eeprom_read_byte(&positionAstore));
  OCR1B = 0; // B is presently unused, could independantly drive another servo
  DDRB = BIT(PB1)
    | BIT(PB2);  // have to set up pins as outputs

  // Set up LED display
  bit_set(LED(DDR), LEDS);  // Set LED pins to outputs
  bit_clear(LED(PORT), LEDS);  // Initialize to 0 (off)

  // Set up button inputs
  bit_clear(BUTTON(DDR), BUTTONS);  // Set button pins to inputs
  bit_set(BUTTON(PORT), BUTTONS);  // Enable pull-up resistors


  uint8_t mode = RUN_MODE;
  uint8_t positionA = eeprom_read_byte(&positionAstore);
  uint8_t positionB = eeprom_read_byte(&positionBstore);
  uint8_t *currentPosition = &positionA;  // always start with positionA
  while (1) {
    uint8_t changes = debounce(sampleButtons());  // invert button state so 1 means "pressed"

    if (changes) {
      if (bit_read(changes, BUTTON_GO) && bit_read(debouncedState, BUTTON_GO)) {  // REM: yes, this begs for a state machine...
        // "go" button was pressed
        if (&positionA == currentPosition) {
          currentPosition = &positionB;
        } else {
          currentPosition = &positionA;
        }
        setServo(*currentPosition);
      }	else if (bit_read(changes, BUTTON_SET) && bit_read(debouncedState, BUTTON_SET)) {
        // "set" button was pressed
        if (RUN_MODE == mode) {
          currentPosition = &positionA;  // always program positionA first          
          mode = SET_MODE;
        } else {
          eeprom_write_byte(&positionAstore, positionA);  // Stash the A & B positions
          eeprom_write_byte(&positionBstore, positionB);
          bit_clear(LED(PORT), LEDS);  // Turn off LEDs in run mode
          mode = RUN_MODE;
        }
      }
    }
    
    if (SET_MODE == mode) {
      // OPTIMIZEME: when not in SET_MODE, turn off adc and maybe even voltage divider
      bit_assign(LED(PORT), (ADCH >> 4), LEDS);  // Read ADC and divide by 16 to get 4-bit value (I only have 4 LEDs hooked up)
      *currentPosition = ADCH;  // Read ADC and store it in whichever position is current
      setServo(*currentPosition);
    }

    _delay_ms(1);  // OPTIMIZEME: could sleep and have a 1khz interrupt drive the run loop
  }
}
