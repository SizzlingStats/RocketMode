
#pragma once

#include <assert.h>

#define FORCEINLINE __forceinline

class QAngle
{
public:
    float x, y, z;
    void Init( void )
    {
        x = y = z = 0.0f;
    }
    void Init( float _x, float _y, float _z )
    {
        x = _x;
        y = _y;
        z = _z;
    }
};

class Vector
{
public:
    float x, y, z;
    Vector()
    {
    }
    Vector(float X, float Y, float Z)
    {
        x = X; y = Y; z = Z;
    }
    void Init( void )
    {
        x = y = z = 0.0f;
    }
    void Init( float _x, float _y, float _z )
    {
        x = _x;
        y = _y;
        z = _z;
    }
    float& operator[](int i)
    {
        assert( (i >= 0) && (i < 3) );
        return ((float*)this)[i];
    }
    float operator[](int i) const
    {
        assert( (i >= 0) && (i < 3) );
        return ((float*)this)[i];
    }
    FORCEINLINE float LengthSqr(void) const
    {
        //CHECK_VALID(*this);
        return (x * x + y * y + z * z);
    }
    FORCEINLINE float DistToSqr(const Vector& vOther) const
    {
        Vector delta;
        delta.x = x - vOther.x;
        delta.y = y - vOther.y;
        delta.z = z - vOther.z;
        return delta.LengthSqr();
    }
};
