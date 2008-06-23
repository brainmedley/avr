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
#include <avr/io.h> 
#include <avr/interrupt.h> 

#include "bit_helpers.h"


/////  Segment Layout /////
#define SEGcenter BIT(0)
#define SEGtop BIT(1)
#define SEGupright BIT(2)
#define SEGlowright BIT(3)
#define SEGbottom BIT(4)
#define SEGlowleft BIT(5)
#define SEGupleft BIT(6)
//////////


/////  Digit Font  /////
// FIXME: should this be an array?
uint8_t DigitFont[] = {
  (SEGtop | SEGupright | SEGlowright | SEGbottom | SEGlowleft | SEGupleft),  // 0
  (SEGupright | SEGlowright),  // 1
  (SEGtop | SEGupright | SEGcenter | SEGlowleft | SEGbottom),  // 2
  (SEGtop | SEGupright | SEGcenter | SEGlowright | SEGbottom),  // 3
  (SEGupleft | SEGcenter | SEGupright | SEGlowright),  // 4
  (SEGtop | SEGupleft | SEGcenter | SEGlowright | SEGbottom),  // 5
  (SEGtop | SEGupleft | SEGlowleft | SEGbottom | SEGlowright | SEGcenter),  // 6
  (SEGtop | SEGupright | SEGlowright),  // 7
  (SEGcenter | SEGtop | SEGupright | SEGlowright | SEGbottom | SEGlowleft | SEGupleft),  // 8
  (SEGcenter | SEGtop | SEGupright | SEGlowright | SEGbottom | SEGupleft),  // 9
};
//////////


#define INC_MOD(v, m) if (++(v) > (m) - 1) (v) = 0


#define LED(reg) (reg##D)
#define LEDS B8(1111)

#define SEGMENT(reg) (reg##D)
#define SEGMENTS B8(1111111)

#define DIGIT(reg) (reg##C)
#define DIGITS (BIT(PC0) | BIT(PC1) | BIT(PC2) | BIT(PC3))


uint8_t display[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
uint8_t displayHead = 0;
uint8_t displaySize = 10;

void advance() {
  static int counter = 0;
  if (counter > 300) {
    counter = 0;
    INC_MOD(displayHead, displaySize);
  } else {
    counter++;
  }
}

#define DISPLAY_LEN 4

ISR (TIMER0_OVF_vect) { 
  static uint8_t currentDigit = 0;  // Keeps track of the digit to be shown
  static uint8_t currentSeg = 0;

  uint8_t segs = DigitFont[display[(displayHead + currentDigit) % displaySize]];  // retrieve and format the value
  //uint8_t segs = DigitFont[displayHead];  // retrieve and format the value

  bit_assign(DIGIT(PORT), ~BIT(currentDigit), DIGITS);  // Select pin for current digit

  bit_assign(SEGMENT(PORT), bit_read(segs, BIT(currentSeg)), SEGMENTS);  // Push the bits
  if (++currentSeg > 6) {
    currentSeg = 0;
    //if (++currentDigit > DISPLAY_LEN - 1) currentDigit = 0;  // Advance currentdigit, with pseudo-modulus
    INC_MOD(currentDigit, DISPLAY_LEN);
    advance();
  }
}


int main(void) {
  // Set up LED display
  bit_set(SEGMENT(DDR), SEGMENTS);  // Set pins to outputs
  bit_clear(SEGMENT(PORT), SEGMENTS);  // Initialize to 0 (off)
  bit_set(DIGIT(DDR), DIGITS);  // Set pins to outputs
  bit_set(DIGIT(PORT), DIGITS);  // Initialize to 1 (off)

  // TCCR0B = BIT(CS00)  | BIT(CS01);  // Prescale / 64 -- kinda blinky
  // TCCR0B = BIT(CS01);  // Prescale / 8  -- blinky with segment mux
  TCCR0B = BIT(CS00);  // no prescale
  TIMSK0 |= BIT(TOIE0);  // Allow timer 0 overflow interrupts
  TCNT0 = 0xFF;

  sei();  // Enable interrupts

  //while (1) {
  //  INC_MOD(displayHead, displaySize);
    //display[0]++;
  //  _delay_ms(100);  // OPTIMIZEME: could sleep and have a 1khz interrupt drive the run loop
  //}
}
