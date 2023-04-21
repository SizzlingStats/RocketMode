
#pragma once

#include "autotalent/ladspa.h"

enum AT_Ports
{
    AT_CONCERT_A = 0,
    AT_FIXED_PITCH_SEMITONES = 1,
    AT_FIXED_PITCH_DEGREE = 2,
    AT_A = 3,
    AT_Bb = 4,
    AT_B = 5,
    AT_C = 6,
    AT_Db = 7,
    AT_D = 8,
    AT_Eb = 9,
    AT_E = 10,
    AT_F = 11,
    AT_Gb = 12,
    AT_G = 13,
    AT_Ab = 14,
    AT_CORRECTION_STRENGTH = 15,
    AT_CORRECTION_SMOOTHNESS = 16,
    AT_PITCH_SHIFT_NOTES = 17,
    AT_OUTPUT_SCALE_ROTATE = 18,
    AT_LFO_DEGREE = 19,
    AT_LFO_RATE = 20,
    AT_LFO_SHAPE = 21,
    AT_LFO_SYMMETRY = 22,
    AT_LFO_QUANTIZATION = 23,
    AT_FORMANT_CORRECTION = 24,
    AT_FORMANT_WARP = 25,
    AT_MIX = 26,
    AT_PITCH_OUTPUT = 27,
    AT_CONFIDENCE_OUTPUT = 28,
    AT_INPUT1 = 29,
    AT_OUTPUT1 = 30,
    AT_LATENCY_OUTPUT = 31,
    AT_NUM_PORTS
};

//                                   A, Bb,  B,  C, Db,  D, Eb,  E,  F, Gb,  G, Ab
static float sCMajor[12]        = {  1, -1,  1,  1, -1,  1, -1,  1,  1, -1,  1, -1 };
static float sBFlatMinor[12]    = { -1,  1, -1,  1,  1, -1,  1, -1,  1,  1, -1,  1 };

// autotalent exports
extern "C"
{
    extern void init();
    extern void fini();
}

class AutoTalent
{
public:
    static void GlobalInit() { init(); }
    static void GlobalShutdown() { fini(); }

    AutoTalent()
    {
        mPortSettings[AT_CONCERT_A] = 440.0f;
        mPortSettings[AT_FIXED_PITCH_SEMITONES] = 0.0f;
        mPortSettings[AT_FIXED_PITCH_DEGREE] = 0.0f;
        mPortSettings[AT_A] = 1.0f;
        mPortSettings[AT_Bb] = 1.0f;
        mPortSettings[AT_B] = 1.0f;
        mPortSettings[AT_C] = 1.0f;
        mPortSettings[AT_Db] = 1.0f;
        mPortSettings[AT_D] = 1.0f;
        mPortSettings[AT_Eb] = 1.0f;
        mPortSettings[AT_E] = 1.0f;
        mPortSettings[AT_F] = 1.0f;
        mPortSettings[AT_Gb] = 1.0f;
        mPortSettings[AT_G] = 1.0f;
        mPortSettings[AT_Ab] = 1.0f;
        mPortSettings[AT_CORRECTION_STRENGTH] = 1.0f;
        mPortSettings[AT_CORRECTION_SMOOTHNESS] = 0.0f;
        mPortSettings[AT_PITCH_SHIFT_NOTES] = 0.0f;
        mPortSettings[AT_OUTPUT_SCALE_ROTATE] = 0.0f;
        mPortSettings[AT_LFO_DEGREE] = 0.0f;
        mPortSettings[AT_LFO_RATE] = 5.0f;
        mPortSettings[AT_LFO_SHAPE] = 1.0f;
        mPortSettings[AT_LFO_SYMMETRY] = 1.0f;
        mPortSettings[AT_LFO_QUANTIZATION] = 0.0f;
        mPortSettings[AT_FORMANT_CORRECTION] = 0.0f;
        mPortSettings[AT_FORMANT_WARP] = 0.0f;
        mPortSettings[AT_MIX] = 1.0f;
        mPortSettings[AT_PITCH_OUTPUT] = 0.0f;
        mPortSettings[AT_CONFIDENCE_OUTPUT] = 0.0f;
        mPortSettings[AT_INPUT1] = 0.0f;
        mPortSettings[AT_OUTPUT1] = 0.0f;
        mPortSettings[AT_LATENCY_OUTPUT] = 0.0f;
    }

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

        for (int i = 0; i < AT_NUM_PORTS; ++i)
        {
            mDescriptor->connect_port(mHandle, i, &mPortSettings[i]);
        }
        
        SetKey(sBFlatMinor);
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

    float mPortSettings[AT_NUM_PORTS];
};
