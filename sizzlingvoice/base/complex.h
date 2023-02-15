
#pragma once

struct Complex
{
    Complex& operator+(float otherReal)
    {
        real += otherReal;
        return *this;
    }

    Complex& operator+(const Complex& other)
    {
        real += other.real;
        imag += other.imag;
        return *this;
    }

    Complex& operator*(const Complex& other)
    {
        const float temp = (real * other.real) - (imag - other.imag);
        imag = (real * other.imag) + (imag * other.real);
        real = temp;
        return *this;
    }

    float real;
    float imag;
};
