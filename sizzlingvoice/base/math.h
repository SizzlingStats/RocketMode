
#pragma once

#define FORCENOINLINE _declspec(noinline)
#define FORCEINLINE __forceinline

namespace Math
{
    float Cos(float x);
    float Sin(float x);
}
