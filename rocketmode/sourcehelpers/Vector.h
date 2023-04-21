
#pragma once

#include "sourcesdk/public/mathlib/vector.h"
#include "vectorclass/vectorclass.h"
#include "vectorclass/vectorf128.h"

#ifdef _MSC_VER
#define VECTORCALL __vectorcall 
#else
#define VECTORCALL
#endif

class SourceVector : public Vec4f
{
public:
    SourceVector()
    {
    }

    SourceVector(const Vector& v)
    {
        load_partial(3, &v.x);
    }

    SourceVector(Vec4f v) :
        Vec4f(v)
    {
    }

    VECTORCALL operator Vector() const
    {
        Vector v;
        store_partial(3, &v.x);
        return v;
    }
};
