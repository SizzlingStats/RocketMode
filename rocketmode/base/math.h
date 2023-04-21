
#pragma once

#include "sourcesdk/public/tier0/platform.h"

namespace Math
{
    float Cos(float x);
    float Sin(float x);
    void SinCos(float x, float* outSin, float* outCos);
    float Sqrt(float x);
    float Exp(float x);
    float Log(float x);

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

    FORCEINLINE float Lerp(float a, float b, float t)
    {
        return (a * (1.0f - t)) + (b * t);
    }

    FORCEINLINE float ExpDecay2(float dt, float halflife = 1.0f)
    {
        constexpr float logQuarter = -1.38629436f;
        return Exp(logQuarter / halflife * dt);
    }

    FORCEINLINE float ExpDecay(float dt, float halflife = 1.0f)
    {
        constexpr float logHalf = -0.69314718f;
        return Exp(logHalf / halflife * dt);
    }

    // a + (b-a)*(1 - pow(smooth, dt)),
    float ExpSmooth(float a, float b, float smooth, float dt);
}
