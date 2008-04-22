#include <avr/eeprom.h>
#include <avr/io.h> 
//#include <avr/interrupt.h> 
//#include <avr/sleep.h>

#define F_CPU 1000000  // ATmega168 default clock speed is 1 MHz
#include <util/delay.h> 

#include "bjhutils.h"


static uint8_t debounce0, debounce1, debouncedState;
// vertical stacked counter based debounce
uint8_t debounce(uint8_t sample)
{
  uint8_t delta, changes;
    
  // Set delta to changes from last sample   // REM: Really?  this is differences from the last *state*...
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

#define POSITION_OFFSET 50

#define BUTTON(reg) REG_PORT(reg, D)
#define BUTTON_SET BIT(PD6)  // TODO: find out if the PD is needed
#define BUTTON_GO BIT(PD7)
#define BUTTONS (BUTTON_SET | BUTTON_GO)

#define LED(reg) REG_PORT(reg, D)
#define LEDS B8(1111)

uint8_t sampleButtons() {
  return ~(bit_read(BUTTON(PIN), BUTTONS));  // invert button state so 1 means "pressed"
}

enum  { RUN_MODE = 0, SET_MODE = 1 };

// Declare and initialize non-volitile storage for the servo positions
uint8_t EEMEM positionAstore = 140;  // 135 should be somewhere around the middle...
uint8_t EEMEM positionBstore = 130;  // ...move a little to either side by default

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
  OCR1A = eeprom_read_byte(&positionAstore) + POSITION_OFFSET; // servo range of 125ish - 250ish
  OCR1B = 185; // servo at center  --  presently unused, could independantly drive another servo
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
  uint8_t *currentPosition = &positionA;
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
      }	else if (bit_read(changes, BUTTON_SET) && bit_read(debouncedState, BUTTON_SET)) {
	// "set" button was pressed
	if (RUN_MODE == mode) {
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
      *currentPosition = ADCH;  // Read ADC and store it in whichever position is current
      bit_assign(LED(PORT), (ADCH >> 4), LEDS);  // Read ADC and divide by 16 to get 4-bit value (I only have 4 LEDs hooked up)
    }

    OCR1A = *currentPosition + POSITION_OFFSET;  // REM: theoreticaly could assign this only where actually necessary, but I see no reason not to simply inforce that the servo *shall* be at currentPosition

    _delay_ms(1);  // OPTIMIZEME: could sleep and have a 1khz interrupt drive the run loop
  }
}
