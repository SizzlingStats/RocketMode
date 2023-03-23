
#pragma once

struct matrix3x4_t
{
    float m_flMatVal[3][4];
};

inline float DegToRad(float deg)
{
    return deg * (3.14159f / 180.f);
}

using byte = char;

struct cplane_t
{
    Vector	normal;
    float	dist;
    byte	type;			// for fast side tests
    byte	signbits;		// signx + (signy<<1) + (signz<<1)
    byte	pad[2];

    cplane_t() {}

private:
    // No copy constructors allowed if we're in optimal mode
    cplane_t(const cplane_t& vOther);
};
