
#include "math.h"

#include "vectorclass/vectormath_trig.h"

namespace Math
{
    float Cos(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = cos(vec);
        return vec.extract(0);
    }

    float Sin(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = sin(vec);
        return vec.extract(0);
    }

    void SinCos(float x, float* outSin, float* outCos)
    {
        Vec4f vecSin, vecCos;
        vecSin.load_partial(1, &x);
        vecSin = sincos(&vecCos, vecSin);
        *outSin = vecSin.extract(0);
        *outCos = vecCos.extract(0);
    }

    float Sqrt(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = sqrt(vec);
        return vec.extract(0);
    }
}
