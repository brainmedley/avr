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

#include <stdint.h>
#include "debounce.h"
//////////


/////  Debouncer  //////
//typedef struct {
//    uint8_t debounce0;
//    uint8_t debounce1;
//    uint8_t debouncedState;
//    // REM: maybe add a 'poll' function pointer here so 'debounce' would poll-convert-and-debounce all in one shot
//} debounceState;


// vertical stacked counter based debounce
uint8_t debounce(uint8_t sample, debounceState *debouncer) {
    uint8_t delta, changes;
    
    // Set delta to changes from last stable state
    delta = sample ^ debouncer -> debouncedState;
    
    // Increment counters
    debouncer -> debounce1 = (debouncer -> debounce1) ^ (debouncer -> debounce0);
    debouncer -> debounce0  = ~(debouncer -> debounce0);
    
    // reset any unchanged bits
    debouncer -> debounce0 &= delta;
    debouncer -> debounce1 &= delta;
    
    // update state & calculate returned change set
    changes = ~(~delta | (debouncer -> debounce0) | (debouncer -> debounce1));
    debouncer -> debouncedState ^= changes;
    
    return changes;
}
//////////
