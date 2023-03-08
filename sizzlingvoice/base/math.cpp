
#include "math.h"

#include "vectorclass/vectormath_trig.h"

namespace Math
{
    FORCENOINLINE float Cos(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = cos(vec);
        return vec.extract(0);
    }

    FORCENOINLINE float Sin(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = sin(vec);
        return vec.extract(0);
    }

    FORCENOINLINE float Sqrt(float x)
    {
        Vec4f vec;
        vec.load_partial(1, &x);
        vec = sqrt(vec);
        return vec.extract(0);
    }
}
