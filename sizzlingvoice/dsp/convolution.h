
#pragma once

#include <string.h>
#include <stdint.h>
#include <assert.h>

// https://ccrma.stanford.edu/~jos/sasp/Convolving_Long_Signals.html
class Convolution
{
public:
    ~Convolution()
    {
        delete[] mKernel;
    }

    void Init(const float* kernel, int kernelSamples, int frameSizeMax)
    {
        mFrameSizeMax = frameSizeMax;
        mKernelSamples = kernelSamples;

        delete[] mKernel;
        const int outputSamples = frameSizeMax + kernelSamples - 1;
        float* fKernel = new float[kernelSamples + outputSamples]();
        memcpy(fKernel, kernel, kernelSamples * sizeof(float));
        mKernel = fKernel;
    }

    void Process(float* samples, int numSamples)
    {
        assert(numSamples <= mFrameSizeMax);
        if (numSamples > mFrameSizeMax)
        {
            return;
        }

        float* kernel = mKernel;
        const int kernelSamples = mKernelSamples;
        float* out = kernel + kernelSamples;
        const int leftoverSamples = kernelSamples - 1;
        const int outputSamples = numSamples + leftoverSamples;

        // convolve
        for (int i = 0; i < outputSamples; ++i)
        {
            int startk = i >= numSamples ? i - numSamples + 1 : 0;
            int endk = i < kernelSamples ? i : kernelSamples - 1;
            for (int k = startk; k <= endk; ++k)
            {
                out[i] += samples[i - k] * kernel[k];
            }
        }

        // output
        for (int i = 0; i < numSamples; ++i)
        {
            samples[i] = out[i];
        }

        // move leftover samples to front and fill rest with 0s
        {
            int i = 0;
            for (int j = numSamples; j < outputSamples; ++j, ++i)
            {
                out[i] = out[j];
            }
            for (; i < outputSamples; ++i)
            {
                out[i] = 0.0f;
            }
        }
    }

private:
    float* mKernel = nullptr;
    int mKernelSamples = 0;
    int mFrameSizeMax = 0;
};
