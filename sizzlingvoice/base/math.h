
#pragma once

#define FORCENOINLINE _declspec(noinline)
#define FORCEINLINE __forceinline

namespace Math
{
    float Cos(float x);
    float Sin(float x);
    void SinCos(float x, float* outSin, float* outCos);
    float Sqrt(float x);

    template<typename T>
    FORCEINLINE T Min(T x, T y)
    {
        return x < y ? x : y;
    }

    template<typename T>
    FORCEINLINE T Max(T x, T y)
    {
        return x > y ? x : y;
    }

    template<typename T>
    FORCEINLINE T Clamp(T x, T min, T max)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
}
