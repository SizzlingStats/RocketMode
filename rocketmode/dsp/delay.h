
#pragma once

#include "base/math.h"
#include <stdint.h>

class Delay
{
public:
    Delay()
    {
    }

    ~Delay()
    {
        delete[] mBuffer;
    }

    void Init(float delayTime, float feedback, float sampleRate)
    {
        mFeedback = feedback;

        const int samples = static_cast<int>(delayTime * sampleRate);
        mDelayLengthSamples = samples;

        delete[] mBuffer;
        float* buffer = new float[2 * samples]();

        mBuffer = buffer;
        mWrite = buffer;
        mRead = buffer + samples;
        mBufferEnd = buffer + samples + samples;
    }

    void Process(float* samples, int numSamples)
    {
        float feedback = mFeedback;
        float* read = mRead;
        float* write = mWrite;
        float* end = mBufferEnd;
        for (int i = 0; i < numSamples; ++i)
        {
            float output = samples[i] + (*read++ * feedback);
            samples[i] = output;
            *write++ = output;

            if (read >= end)
            {
                read = mBuffer;
            }
            if (write >= end)
            {
                write = mBuffer;
            }
        }
        mRead = read;
        mWrite = write;
    }

private:
    float* mBuffer = nullptr;
    float* mBufferEnd = nullptr;

    float* mWrite = nullptr;
    float* mRead = nullptr;

    float mFeedback = 0.0f;
    int mDelayLengthSamples = 0;
};
