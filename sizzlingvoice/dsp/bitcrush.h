
/*
Yet another bitcrusher algorithm. But this one has smooth parameter control.

Input is assumed to be between 0 and 1.
Normfreq goes from 0 to 1.0; (freq/samplerate).
Bits goes from 1 to 16.

Output gain is greater than unity when bits < 1.0.

function output = crusher( input, normfreq, bits );
    step = 1/2^(bits);
    phasor = 0;
    last = 0;

    for i = 1:length(input)
       phasor = phasor + normfreq;
       if (phasor >= 1.0)
          phasor = phasor - 1.0;
          last = step * floor( input(i)/step + 0.5 ); %quantize
       end
       output(i) = last; %sample and hold
    end
end
*/

#pragma once

#include <math.h>
#include <assert.h>

class BitCrush
{
public:
    BitCrush(float freq, float sampleRate, float bits)
    {
        assert(sampleRate > 0.0f);

        mPhasor = 0.0f;
        mLast = 0.0f;
        mSampleRate = sampleRate;

        Bits(bits);
        Rate(freq);
    }

    void Bits(float bits)
    {
        assert(bits >= 1.0f && bits <= 16.0f);
        const float powBits = exp2f(bits);
        mStep = 1.0f / powBits;
        mInvStep = powBits;
    }

    void Rate(float freq)
    {
        assert(freq > 0.0f);
        assert(freq <= mSampleRate);
        mNormFreq = freq / mSampleRate;
    }

    float Process(float input)
    {
        float phasor = mPhasor;
        float last = mLast;

        phasor += mNormFreq;
        if (phasor >= 1.0f)
        {
            phasor -= 1.0f;
            last = mStep * floorf((input * mInvStep) + 0.5f);
        }
        mPhasor = phasor;
        mLast = last;
        return last;
    }

private:
    float mStep;
    float mInvStep;
    float mPhasor;
    float mNormFreq;
    float mLast;
    float mSampleRate;
};
