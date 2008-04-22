#include <stdio.h>
#include "bjhutils.h"

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


void binstring(char *buffer, int len, int num) {
  buffer[len - 1] = (char)0;  // null-terminate
  int end = len - 2;
  for (int i = 0 ; i <= end ; i++) {
    if (num & 1) {
      buffer[end - i] = '1';
    } else {
      buffer[end - i] = '0';
    }
    num >>= 1;
  }
}


int main(void) {
  uint8_t foo = B8(10101010);
  char fooBuf[9];
  bit_assign(foo, B8(01011100), B8(00001111));
  binstring(fooBuf, 9, foo);
  printf("%s\n", fooBuf);
  return 0;

  char debounce0Buf[9];
  char debounce1Buf[9];
  char debouncedStateBuf[9];
  char changesBuf[9];
  uint8_t inputs[] = {
    B8(001),
    B8(101),
    B8(101),
    B8(101),
    B8(101),
    B8(101),
    B8(100),
    B8(100),
    B8(100),
    B8(100),
    B8(100),
    0xFF  // sentinel value to terminate
  };
  
  for (int i = 0 ; inputs[i] != 0xFF ; i++) {  // Loop until sentinel value
    uint8_t changes = debounce(inputs[i]);
    binstring(debounce0Buf, 9, debounce0);
    binstring(debounce1Buf, 9, debounce1);
    binstring(debouncedStateBuf, 9, debouncedState);
    binstring(changesBuf, 9, changes);
    
    printf("debounce1:\t%s\ndebounce0:\t%s\n\ndebouncedState:\t%s\nchanges:\t%s\n------------------------\n", debounce1Buf, debounce0Buf, debouncedStateBuf, changesBuf);
  }
  return 0;
}
