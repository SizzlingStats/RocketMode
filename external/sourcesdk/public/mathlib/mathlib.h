
#pragma once

struct matrix3x4_t
{
    float m_flMatVal[3][4];
};

inline float DegToRad(float deg)
{
    return deg * (3.14159f / 180.f);
}
