/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>
#include <util/delay.h>

#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "bjhlib/bit_helpers.h"
#include "bjhlib/debounce.h"
#include "bjhlib/time.h"

#include "usbdrv.h"
#include "oddebug.h"
//////////


/////  USB  //////
// We use a simplifed keyboard report descriptor which does not support the boot protocol. We don't allow setting status LEDs and we only allow one simultaneous key press (except modifiers). We can therefore use short 2 byte input reports. The report descriptor has been created with usb.org's "HID Descriptor Tool" which can be downloaded from http://www.usb.org/developers/hidpage/ Redundant entries (such as LOGICAL_MINIMUM and USAGE_PAGE) have been omitted for the second INPUT item.
PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = { // USB report descriptor
    0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,  // USAGE (Keyboard)
    0xa1, 0x01,  // COLLECTION (Application)
    0x05, 0x07,  //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,  //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,  //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,  //   LOGICAL_MINIMUM (0)
    0x25, 0x01,  //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,  //   REPORT_SIZE (1)
    0x95, 0x08,  //   REPORT_COUNT (8)
    0x81, 0x02,  //   INPUT (Data,Var,Abs)
    0x95, 0x01,  //   REPORT_COUNT (1)
    0x75, 0x08,  //   REPORT_SIZE (8)
    0x25, 0x65,  //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,  //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,  //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,  //   INPUT (Data,Ary,Abs)
    0xc0         // END_COLLECTION
};


// Keyboard usage values, see usb.org's HID-usage-tables document, chapter 10 Keyboard/Keypad Page for more codes.
#define MOD_CONTROL_LEFT    (1<<0)
#define MOD_SHIFT_LEFT      (1<<1)
#define MOD_ALT_LEFT        (1<<2)
#define MOD_GUI_LEFT        (1<<3)
#define MOD_CONTROL_RIGHT   (1<<4)
#define MOD_SHIFT_RIGHT     (1<<5)
#define MOD_ALT_RIGHT       (1<<6)
#define MOD_GUI_RIGHT       (1<<7)

#define KEY_1       30
#define KEY_2       31
#define KEY_3       32
#define KEY_4       33
#define KEY_5       34
#define KEY_6       35
#define KEY_7       36
#define KEY_8       37
#define KEY_9       38
#define KEY_0       39
#define KEY_RETURN  40
#define KEY_COMMA   54
#define KEY_PERIOD  55


static uchar valueBuffer[16];
static uchar *nextDigit;
static void bufferValue(unsigned int value) {
    uchar digit;
    
    nextDigit = &valueBuffer[sizeof(valueBuffer)];
    *--nextDigit = 0xff;  // terminate with 0xff
    *--nextDigit = 0;
    *--nextDigit = KEY_RETURN;
    do {
        digit = value % 10;
        value /= 10;
        *--nextDigit = 0;
        if (digit == 0) {
            *--nextDigit = KEY_0;
        } else {
            *--nextDigit = KEY_1 - 1 + digit;
        }
    } while (value != 0);
}


static uchar reportBuffer[2];  // buffer for HID reports
static uchar idleRate;  // in 4 ms units
static void buildReport(void) {
    uchar key = 0;
    
    if (nextDigit != NULL) {
        key = *nextDigit;
    }
    reportBuffer[0] = 0;  // no modifiers
    reportBuffer[1] = key;
}


uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;
    
    usbMsgPtr = reportBuffer;
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {  // class request type
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {  // wValue: ReportType (highbyte), ReportID (lowbyte)
            // we only have one report type, so don't look at wValue
            buildReport();
            return sizeof(reportBuffer);
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idleRate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
        }
    } else {
        // no vendor specific requests implemented
    }
    return 0;
}


void usbPollAndSend() {
    usbPoll();
    if (usbInterruptIsReady() && nextDigit != NULL){  // we can send another key
        buildReport();
        usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
        if (*++nextDigit == 0xff) {   /// this was terminator character
            nextDigit = NULL;
        }
    }
}
//////////


/////  Debugger  //////
#define DEBUGGER(reg) (reg##B)
#define DEBUGGER_LED_MASK B8(1111)
void mutter(uint8_t value) {
    bit_assign(DEBUGGER(PORT), value, DEBUGGER_LED_MASK);
}

#define DEBUGGER_BUTTON_PIN BIT(4)
void setupDebugger() {
    bit_set(DEBUGGER(DDR), DEBUGGER_LED_MASK);  // Set LED pins to outputs
    bit_clear(DEBUGGER(PORT), DEBUGGER_LED_MASK);  // Initialize to 0 (off)

    bit_clear(DEBUGGER(DDR), DEBUGGER_BUTTON_PIN);  // Set button pin to input
    bit_set(DEBUGGER(PORT), DEBUGGER_BUTTON_PIN);  // Enable pull-up resistor
    
    mutter(0);
}
//////////


/////  ADC  //////
// FIXME: pull this out into a library somewhere, add methods for reading the full 10-bits and multiplexing the channels
#define ADC_PRESCALE_8 (BIT(ADPS1) | BIT(ADPS0))  // Prescale clock by 8 -- 16M / 8 = 2M (way overkill for this project, gotta look up a more reasonable prescale)
#define _ADC_BUFFER_DISABLE_MASK(c) (BIT(ADC##c##D))
void setupAdcChannel(uint8_t channel) {
    bit_set(ADCSRA, ADC_PRESCALE_8);  // Set clock prescale
    bit_set(ADMUX, BIT(REFS0));  // Set reference to AVcc  FIXME: change to one of the fixed values so we can work out mV?
    bit_set(ADMUX, BIT(ADLAR));  // Set left-alignment for easy 8-bit reading of ADCH
    bit_assign(ADMUX, channel, B8(111)); // Enable ADC channel
    //bit_set(DIDR0, _ADC_BUFFER_DISABLE_MASK(channel));  // Disable digital input buffer on the ADC pin  REM: not required and the macros don't expand right when extracted to a function
    bit_set(ADCSRA, BIT(ADATE));  // Auto Trigger Enable  (default auto-trigger is free-running mode)
    bit_set(ADCSRA, BIT(ADEN));  // Enable ADC
    bit_set(ADCSRA, BIT(ADSC));  // Start conversions
}
//////////


/////  Business Logic  //////
void setupTimer() {
    TCCR0A = 0;
    bit_set(TCCR0A, BIT(WGM01));  // Clear-Timer-on-Compare
    TCCR0B = 0;
    OCR0A = 250;  // 16MHz / 64 / 250 = 1kHz
    bit_set(TCCR0B, BIT(CS01)|BIT(CS00));  //  prescale clk / 64
    //bit_set(TIMSK0, BIT(OCIE0A));  // Enable interrupt
}


#if 0
#define ADC_CHANNEL 2
#define PROBE_SWITCH_PIN BIT(0)
#else
#define ADC_CHANNEL 3
#define PROBE_SWITCH_PIN BIT(1)
#endif

void setupAdc() {
    setupAdcChannel(ADC_CHANNEL);
    _delay_ms(1);  // HACK ALERT: stall long enough for the ADC to get up and running
}


#define PROBE(reg) (reg##C)
void setupProbe() {
    // Set up button inputs
    bit_clear(PROBE(DDR), PROBE_SWITCH_PIN);  // Set button pins to inputs
    bit_set(PROBE(PORT), PROBE_SWITCH_PIN);  // Enable pull-up resistors
}


uint8_t readTemp() {
    uint8_t value = ADCH;
//    mutter(value >> 4);
    // TODO: offset by calibration factor
    return value;
}

//#define PROBE_SWITCH_BIT BIT(0)
//#define DEBUGGER_BUTTON_BIT BIT(7)
uint8_t inputPoll() {
    uint8_t result = ~bit_read(DEBUGGER(PIN), DEBUGGER_BUTTON_PIN);  // Invert so 1 means "pressed"
    return result;
}
//////////


/////  Main  //////
void setup() {
    odDebugInit();
    usbDeviceDisconnect();
    for (int i = 0 ; i < 20 ; i++) {  // 300 ms disconnect
        _delay_ms(15);
        wdt_reset();
    }
    usbDeviceConnect();
    
    setupTimer();
    setupDebugger();
    setupAdc(ADC_CHANNEL);
    
    usbInit();
}


//static debounceState debouncer = { 0, 0, 0 };
static uint8_t isRecording = 0;
static uint8_t currentTemp = 0;
void run(timeChanges tchanges, time *t, debounceState *debouncer) {  // FIXME: better name
    // Every ms:
    uint8_t changes = debounce(inputPoll(), debouncer);
    if (bit_read(changes, DEBUGGER_BUTTON_PIN) && bit_read(debouncer -> debouncedState, DEBUGGER_BUTTON_PIN)) {
        isRecording = isRecording ? 0 : 1;
    }
    
    
    if (tchanges.quarters && ! bit_read(t -> quarters, B8(11))) {
        // Every second
        currentTemp = readTemp();
        mutter(currentTemp >> 4);
    }

    if (currentTemp && isRecording && nextDigit == NULL) {
        bufferValue(currentTemp);
        currentTemp = 0;
    }
}


int main(void) {
    wdt_enable(WDTO_1S);  // WDT can sometimes persist across resets, re-init early to be sure of the state
    wdt_reset();
    
    setup();
    wdt_reset();
    sei();    

    time timer = { 0, 0, 0 };
    debounceState debouncer = { 0, 0, 0 };
    while (1) {  // main event loop
        // every-run-loop stuff
        usbPollAndSend();

        if (bit_read(TIFR0, BIT(OCF0A))) {  // check the Compare Flag
            bit_set(TIFR0, BIT(OCF0A));  // "clear" the compare flag by writing to it
            timeChanges tchanges = updateTime(&timer);
            
            run(tchanges, &timer, &debouncer);
        }
        
        wdt_reset();
    }    
    return 0;
}
//////////
