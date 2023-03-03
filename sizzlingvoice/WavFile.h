
#pragma once

#include <stdint.h>

class WavFile
{
public:
    enum SampleFormat : uint16_t
    {
        WAVE_FORMAT_NONE = 0,
        WAVE_FORMAT_PCM = 1,
        WAVE_FORMAT_IEEE_FLOAT = 3
    };

    ~WavFile();
    bool Load(const char* file);
    const char* Samples() const { return mSampleData; }
    uint32_t NumSamples() const { return mNumSamples; }
    SampleFormat Format() const { return mFormat; }
    uint16_t NumChannels() const { return mNumChannels; }
    uint32_t SampleRate() const { return mSampleRate; }

private:
    char* mSampleData = nullptr;
    uint32_t mNumSamples = 0;
    SampleFormat mFormat = WAVE_FORMAT_NONE;
    uint16_t mNumChannels = 0;
    uint32_t mSampleRate = 0;
};
