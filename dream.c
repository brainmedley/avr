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

  //while (1) {
    //LED_PORT = ~LED_PORT;
    //_delay_ms(100);
    //sleep_mode();
  //}


  uint8_t count = 0;
  uint8_t debounce_counter = 0;
  uint8_t debounce_threshold = DEBOUNCE_BASE_THRESHOLD;
  uint8_t button_state, button_old_state = 0;

  while (1) {
    uint8_t button_new_state = ~(BUTTON_PIN & BUTTONS);  // invert button state so 1 means "pressed"

    if (button_new_state == button_old_state) {  // buttons are in the same state as the last poll...
      if (button_state != button_new_state) {    // ...but different than the last "processed" input
	debounce_counter++;
      }
    } else {
      debounce_counter = 0;
    }

    if (button_state != button_new_state && debounce_threshold < debounce_counter) {
      debounce_threshold = DEBOUNCE_BASE_THRESHOLD;  // reset debounce_threshold for the next input cycle

      if ((button_new_state & BUTTON_INC) && (button_new_state & BUTTON_DEC)) {
	// both buttons were pressed
	TIMER_THRESH = DEFAULT_TIMER_THRESH;
	debounce_threshold = DEBOUNCE_BOTH_THRESHOLD;  // extra debounce to give both buttons a chance to be released
      }	else if (button_new_state & BUTTON_INC) {
	// increment button was pressed
	TIMER_THRESH -= TIMER_THRESH_STEP;  // reducing the timer threshold makes it fire faster
      }	else if (button_new_state & BUTTON_DEC) {
	// decrement button was pressed
	TIMER_THRESH += TIMER_THRESH_STEP;
      }
      button_state = button_new_state;  // new state has been processed and is now the status quo
    }

    //LED_PORT = count;  // invert count since the LEDs are being sink-driven
    TIMER_THRESH_PORT = TIMER_THRESH;
    button_old_state = button_new_state;
    _delay_ms(1);
  }
}
