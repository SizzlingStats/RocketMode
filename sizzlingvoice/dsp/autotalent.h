
#pragma once

#include "autotalent/ladspa.h"

#define AT_TUNE 0
#define AT_FIXED 1
#define AT_PULL 2
#define AT_A 3
#define AT_Bb 4
#define AT_B 5
#define AT_C 6
#define AT_Db 7
#define AT_D 8
#define AT_Eb 9
#define AT_E 10
#define AT_F 11
#define AT_Gb 12
#define AT_G 13
#define AT_Ab 14
#define AT_AMOUNT 15
#define AT_SMOOTH 16
#define AT_SHIFT 17
#define AT_SCWARP 18
#define AT_LFOAMP 19
#define AT_LFORATE 20
#define AT_LFOSHAPE 21
#define AT_LFOSYMM 22
#define AT_LFOQUANT 23
#define AT_FCORR 24
#define AT_FWARP 25
#define AT_MIX 26
#define AT_PITCH 27
#define AT_CONF 28
#define AT_INPUT1  29
#define AT_OUTPUT1 30
#define AT_LATENCY 31

//                                   A, Bb,  B,  C, Db,  D, Eb,  E,  F, Gb,  G, Ab
static float sCMajor[12]        = {  1, -1,  1,  1, -1,  1, -1,  1,  1, -1,  1, -1 };
static float sBFlatMinor[12]    = { -1,  1, -1,  1,  1, -1,  1, -1,  1,  1, -1,  1 };

// autotalent exports
extern "C"
{
    extern void _init();
    extern void _fini();
}

class AutoTalent
{
public:
    static void GlobalInit() { _init(); }
    static void GlobalShutdown() { _fini(); }

    void SetKey(float(&key)[12])
    {
        for (int i = AT_A; i <= AT_Ab; ++i)
        {
            mDescriptor->connect_port(mHandle, i, &key[i - AT_A]);
        }
    }

    void InitInstance()
    {
        mDescriptor = ladspa_descriptor(0);
        mHandle = mDescriptor->instantiate(mDescriptor, 22050);

        mDescriptor->connect_port(mHandle, AT_TUNE, &AHz);
        mDescriptor->connect_port(mHandle, AT_FIXED, &fixedPitchSemitones);
        mDescriptor->connect_port(mHandle, AT_PULL, &fixedPitchDegree);
        
        SetKey(sCMajor);

        mDescriptor->connect_port(mHandle, AT_AMOUNT, &correctionStrength);
        mDescriptor->connect_port(mHandle, AT_SMOOTH, &correctionSmoothness);
        mDescriptor->connect_port(mHandle, AT_SHIFT, &pitchShift);
        mDescriptor->connect_port(mHandle, AT_SCWARP, &outputScaleRotation);
        mDescriptor->connect_port(mHandle, AT_LFOAMP, &lfoDepth);
        mDescriptor->connect_port(mHandle, AT_LFORATE, &lfoRate);
        mDescriptor->connect_port(mHandle, AT_LFOSHAPE, &lfoShape);
        mDescriptor->connect_port(mHandle, AT_LFOSYMM, &lfoSymmetry);
        mDescriptor->connect_port(mHandle, AT_LFOQUANT, &lfoQuant);
        mDescriptor->connect_port(mHandle, AT_FCORR, &formantCorrection);
        mDescriptor->connect_port(mHandle, AT_FWARP, &formantWarp);
        mDescriptor->connect_port(mHandle, AT_MIX, &mix);

        mDescriptor->connect_port(mHandle, AT_PITCH, &pitch);
        mDescriptor->connect_port(mHandle, AT_CONF, &confidence);
        mDescriptor->connect_port(mHandle, AT_INPUT1, nullptr);
        mDescriptor->connect_port(mHandle, AT_OUTPUT1, nullptr);
        mDescriptor->connect_port(mHandle, AT_LATENCY, &latency);
    }

    void ProcessBuffer(float* samples, int numSamples)
    {
        mDescriptor->connect_port(mHandle, AT_INPUT1, samples);
        mDescriptor->connect_port(mHandle, AT_OUTPUT1, samples);
        mDescriptor->run(mHandle, numSamples);
    }

    void DestroyInstance()
    {
        mDescriptor->cleanup(mHandle);
    }
private:
    const LADSPA_Descriptor* mDescriptor = nullptr;
    LADSPA_Handle mHandle = nullptr;

    float fOne = 1.0f;
    float fZero = 0.0f;
    float fNegOne = -1.0f;

    float AHz = 440.0f;
    float fixedPitchSemitones = 0.0f;
    float fixedPitchDegree = 0.0f;
    float correctionStrength = 1.0f;
    float correctionSmoothness = 0.0f;
    float pitchShift = 0.0f;
    float outputScaleRotation = 0.0f;
    float lfoDepth = 0.0f;
    float lfoRate = 5.0f;
    float lfoShape = 1.0f;
    float lfoSymmetry = 1.0f;
    float lfoQuant = 0.0f;
    float formantCorrection = 0.0f;
    float formantWarp = 0.0f;
    float mix = 1.0f;

    float pitch = 0.0f;
    float confidence = 0.0f;
    float latency = 0.0f;
};
