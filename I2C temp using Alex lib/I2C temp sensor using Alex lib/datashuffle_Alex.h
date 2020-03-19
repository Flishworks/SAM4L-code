#ifndef	_DATASHUFFLE_ALEX_H_	// Guards against double calling of headers
#define _DATASHUFFLE_ALEX_H_

#include <stddef.h>
#include <stdint.h>

typedef union {
	uint16_t all;
	uint8_t  byte[2];
} u_16;

typedef union {
	uint32_t all;
	uint16_t half[2];
	uint8_t	 byte[4];
} u_32;

typedef union {
	uint64_t all;
	uint32_t word[2];
	uint16_t half[4];
	uint8_t	 byte[8];
} u_64;

typedef union {
	int16_t all;
	int8_t  byte[2];
} s_16;

typedef union {
	int32_t all;
	int16_t half[2];
	int8_t	byte[4];
} s_32;

typedef union {
	int64_t all;
	int32_t word[2];
	int16_t half[4];
	int8_t	byte[8];
} s_64;


// Byte alignment and packing - (also looks at ASF's compiler.h)
//http://stackoverflow.com/questions/11770451/what-is-the-meaning-of-attribute-packed-aligned4

#define COMPILER_ALIGNED(arg)				__attribute__((__aligned__(arg)))
#define COMPILER_PRAGMA(arg)				_Pragma(#arg)
#define COMPILER_TIGHT_PACK()				COMPILER_PRAGMA(pack(1))
#define COMPILER_RESET_PACK()				COMPILER_PRAGMA(pack())



#endif