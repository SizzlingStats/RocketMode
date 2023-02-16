
#pragma once

#define FORCENOINLINE _declspec(noinline)
#define FORCEINLINE __forceinline

namespace Math
{
    float Cos(float x);
    float Sin(float x);

    FORCEINLINE float Min(float x, float y)
    {
        return x < y ? x : y;
    }

    FORCEINLINE float Max(float x, float y)
    {
        return x > y ? x : y;
    }

    FORCEINLINE float Clamp(float x, float min, float max)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
}
