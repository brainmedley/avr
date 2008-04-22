// Temporary corral for handy stuff, to be refactored as needed

#ifndef _BJH_UTILS_H_
#define _BJH_UTILS_H_ 1

/* From <http://www.thescripts.com/forum/thread216333.html> */
/* Binary constant generator macro
By Tom Torfs - donated to the public domain
*/

/* All macro's evaluate to compile-time constants */

/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0x11111111, always fits in unsigned long
*/
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0)	\
  +((x&0x000000F0LU)?2:0)\
  +((x&0x00000F00LU)?4:0)\
  +((x&0x0000F000LU)?8:0)\
  +((x&0x000F0000LU)?16:0)\
  +((x&0x00F00000LU)?32:0)\
  +((x&0x0F000000LU)?64:0)\
  +((x&0xF0000000LU)?128:0)

// ENHANCEME: a nybble version could be nice, too

/* for up to 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* for up to 16-bit binary constants, MSB first */
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8)\
			+ B8(dlsb))

/* for up to 32-bit binary constants, MSB first */
#define B32(dmsb,db2,db3,dlsb) (((unsigned long)B8(dmsb)<<24) \
				+ ((unsigned long)B8(db2)<<16) \
				+ ((unsigned long)B8(db3)<<8) \
				+ B8(dlsb))

 /* Sample usage:
B8(01010101) = 85
B16(10101010,01010101) = 43605
B32(10000000,11111111,10101010,01010101) = 2164238933
 */

#include <inttypes.h>
#define bit_set_8(var, mask)   ((var) |= (uint8_t)(mask))
#define bit_clear_8(var, mask)   ((var) &= (uint8_t)~(mask))
#define bit_toggle_8(var, mask)   ((var) ^= (uint8_t)(mask))
#define bit_read_8(var, mask)   ((var) & (uint8_t)(mask))
#define bit_assign_8(var, val, mask)   ((var) = (((var)&~(uint8_t)(mask))|((val)&(uint8_t)(mask))))

#define bit_set_16(var, mask)   ((var) |= (uint16_t)(mask))
#define bit_clear_16(var, mask)   ((var) &= (uint16_t)~(mask))
#define bit_toggle_16(var, mask)   ((var) ^= (uint16_t)(mask))
#define bit_read_16(var, mask)   ((var) & (uint16_t)(mask))
#define bit_assign_16(var, val, mask)   ((var) = (((var)&~(uint16_t)(mask))|((val)&(uint16_t)(mask))))

 // 32 bit versions here

 // Shorter named versions for the common operation.
#define bit_set(var, mask)   bit_set_8(var, mask)
#define bit_clear(var, mask) bit_clear_8(var, mask)
#define bit_toggle(var, mask) bit_toggle_8(var, mask)
#define bit_read(var, mask) bit_read_8(var, mask)
#define bit_assign(var, val, mask) bit_assign_8(var, val, mask)

#define BIT(x)   (1 << (x))
#define LONGBIT(x)   ((uint32_t)1 << (x))


// Constructs things like PORTA or DDRA; to be used in definitions like LED(reg) REG_PORT(reg, A)  // ENHANCEME: it'd be nice to make this safer...
#define REG_PORT(r, p) r##p

#endif // _BJH_UTILS_H_
