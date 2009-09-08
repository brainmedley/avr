/*
Copyright (c) 2009 Benjamin Holt

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
//////////


/////  Debouncer  //////
#ifndef _DEBOUNCE_H_
#define _DEBOUNCE_H_ 1
typedef struct {
    uint8_t debounce0;
    uint8_t debounce1;
    uint8_t debouncedState;
    // REM: maybe add a 'poll' function pointer here so 'debounce' would poll-convert-and-debounce all in one shot
} debounceState;

// Updates debouncer based on sample and returns the byte marking which bits in the final debouncedState changed
uint8_t debounce(uint8_t sample, debounceState *debouncer);
#endif // _DEBOUNCE_H_
//////////
