
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "vector.h"

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8 = uint8_t;
using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8 = int8_t;
using uint = unsigned int;
using byte = char;

#if defined(_M_IX86)
#define __i386__ 1
#endif

#define IsPC() true
#ifdef _WIN64
    #define PLATFORM_WINDOWS_PC64 1
#endif

#define Assert(x) assert(x)
#define AssertMsg(x, ...) assert(x)
#define AssertMsg2(x, ...) assert(x)
#define AssertFatalMsg(x, ...) assert(x)

// OVERALL Coordinate Size Limits used in COMMON.C MSG_*BitCoord() Routines (and someday the HUD)
#define	COORD_INTEGER_BITS			14
#define COORD_FRACTIONAL_BITS		5
#define COORD_DENOMINATOR			(1<<(COORD_FRACTIONAL_BITS))
#define COORD_RESOLUTION			(1.0/(COORD_DENOMINATOR))

// Special threshold for networking multiplayer origins
#define COORD_INTEGER_BITS_MP		11
#define COORD_FRACTIONAL_BITS_MP_LOWPRECISION 3
#define COORD_DENOMINATOR_LOWPRECISION			(1<<(COORD_FRACTIONAL_BITS_MP_LOWPRECISION))
#define COORD_RESOLUTION_LOWPRECISION			(1.0/(COORD_DENOMINATOR_LOWPRECISION))

#define NORMAL_FRACTIONAL_BITS		11
#define NORMAL_DENOMINATOR			( (1<<(NORMAL_FRACTIONAL_BITS)) - 1 )
#define NORMAL_RESOLUTION			(1.0/(NORMAL_DENOMINATOR))

template <typename T>
inline T DWordSwapC( T dw )
{
    uint32 temp;
    temp  =   *((uint32 *)&dw)               >> 24;
    temp |= ((*((uint32 *)&dw) & 0x00FF0000) >> 8);
    temp |= ((*((uint32 *)&dw) & 0x0000FF00) << 8);
    temp |= ((*((uint32 *)&dw) & 0x000000FF) << 24);
    return *((T*)&temp);
}

#if defined( _MSC_VER ) && !defined( PLATFORM_WINDOWS_PC64 )
    #define DWordSwap  DWordSwapAsm
    #pragma warning(push)
    #pragma warning (disable:4035) // no return value
    template <typename T>
    inline T DWordSwapAsm( T dw )
    {
        __asm
        {
            mov eax, dw
            bswap eax
        }
    }
    #pragma warning(pop)
#else
    #define DWordSwap DWordSwapC
#endif

inline unsigned long LoadLittleDWord(const unsigned long *base, unsigned int dwordIndex)
{
    return base[dwordIndex];
}

inline void StoreLittleDWord(unsigned long *base, unsigned int dwordIndex, unsigned long dword)
{
    base[dwordIndex] = dword;
}

inline void LittleFloat(float* pOut, float* pIn)
{
    *pOut = *pIn;
}

inline long BigLong(long val)
{
    return DWordSwap(val);
}

#define BITS_PER_INT 32

inline int GetBitForBitnum( int bitNum ) 
{ 
    static int bitsForBitnum[] = 
    {
        ( 1 << 0 ),
        ( 1 << 1 ),
        ( 1 << 2 ),
        ( 1 << 3 ),
        ( 1 << 4 ),
        ( 1 << 5 ),
        ( 1 << 6 ),
        ( 1 << 7 ),
        ( 1 << 8 ),
        ( 1 << 9 ),
        ( 1 << 10 ),
        ( 1 << 11 ),
        ( 1 << 12 ),
        ( 1 << 13 ),
        ( 1 << 14 ),
        ( 1 << 15 ),
        ( 1 << 16 ),
        ( 1 << 17 ),
        ( 1 << 18 ),
        ( 1 << 19 ),
        ( 1 << 20 ),
        ( 1 << 21 ),
        ( 1 << 22 ),
        ( 1 << 23 ),
        ( 1 << 24 ),
        ( 1 << 25 ),
        ( 1 << 26 ),
        ( 1 << 27 ),
        ( 1 << 28 ),
        ( 1 << 29 ),
        ( 1 << 30 ),
        ( 1 << 31 ),
    };
    return bitsForBitnum[ (bitNum) & (BITS_PER_INT-1) ]; 
}
