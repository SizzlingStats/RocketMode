
#pragma once

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#define SINGLE_INHERITANCE __single_inheritance
#else
#define SINGLE_INHERITANCE
#define FORCEINLINE inline __attribute__((always_inline))
#endif
