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

#define DEBOUNCE_BASE_THRESHOLD 5  // 5 ms debounce
#define DEBOUNCE_BOTH_THRESHOLD 20

#define TIMER_THRESH OCR0A
#define DEFAULT_TIMER_THRESH 49
#define TIMER_THRESH_STEP 1

#define TIMER_THRESH_PORT PORTD
#define TIMER_THRESH_DDR DDRD

ISR(TIMER0_COMPA_vect) {
  LED_PORT = ~LED_PORT;
}

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


int main(void) {
  //GIMSK = 0;
  
  bit_clear(BUTTON_DDR, BUTTONS);
  bit_set(BUTTON_PORT, BUTTONS);

  LED_DDR = B8(1);

  // set up an interrupt that goes off at 20Hz - by default
  TCCR0A = BIT(WGM01);
  TCCR0B = B8(101); // 1M/1024 = ~977
  // REM: prescaler reset? PSR10 = 1
  TIMER_THRESH = DEFAULT_TIMER_THRESH; // 977 / 49 = ~20
  TIMSK0 = BIT(OCIE0A); // turn on the interrupt
  sei(); // turn on interrupts

  TIMER_THRESH_DDR = B8(11111111);
  TIMER_THRESH_PORT = TIMER_THRESH;
  
  LED_PORT = 0;

  uint8_t count = 0;
  uint8_t wasBoth = 0;  // ENHANCEME: working out how to do this more like a real state machine would rock

  while (1) {
    uint8_t changes = debounce(~(BUTTON_PIN & BUTTONS));  // invert button state so 1 means "pressed"

    if (changes) {
      if (bit_read(debouncedState, BUTTON_INC) && bit_read(debouncedState, BUTTON_DEC)) {
	// both buttons were pressed
	TIMER_THRESH = DEFAULT_TIMER_THRESH;
	wasBoth = 1;
      } else if ( wasBoth && ! bit_read(debouncedState, BUTTONS)) {
	// both buttons were released
	wasBoth = 0;
      }	else if ( ! wasBoth && bit_read(changes, BUTTON_INC) && bit_read(debouncedState, BUTTON_INC)) {
	// increment button was pressed
	TIMER_THRESH -= TIMER_THRESH_STEP;  // reducing the timer threshold makes it fire faster
      }	else if ( ! wasBoth && bit_read(changes, BUTTON_DEC) && bit_read(debouncedState, BUTTON_DEC)) {
	// decrement button was pressed
	TIMER_THRESH += TIMER_THRESH_STEP;
      }
    }

    //LED_PORT = count;  // invert count since the LEDs are being sink-driven
    TIMER_THRESH_PORT = TIMER_THRESH;
    _delay_ms(1);
  }
}
