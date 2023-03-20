
#pragma once

#ifdef _WIN32
#ifdef SDK_COMPAT
#define OFFSET_INDEX 0
#else
#define OFFSET_INDEX 1
#endif
#else // linux
#define OFFSET_INDEX 2
#endif

#define NUM_CONFIGS 3

namespace HookOffsets
{
    inline constexpr int SetOwnerEntityOffsets[NUM_CONFIGS] = { 17, 18, 19 };
    inline constexpr int SetOwnerEntity = SetOwnerEntityOffsets[OFFSET_INDEX];

    inline constexpr int SpawnOffsets[NUM_CONFIGS] = { 22, 24, 25 };
    inline constexpr int Spawn = SpawnOffsets[OFFSET_INDEX];

    inline constexpr int PlayerRunCommandOffsets[NUM_CONFIGS] = { 421, 430, 431 };
    inline constexpr int PlayerRunCommand = PlayerRunCommandOffsets[OFFSET_INDEX];

    inline constexpr int IsProximityHearingClientOffsets[NUM_CONFIGS] = { 38, 38, 39 };
    inline constexpr int IsProximityHearingClient = IsProximityHearingClientOffsets[OFFSET_INDEX];
    
    inline constexpr int ProcessVoiceDataOffsets[NUM_CONFIGS] = { 3, 3, 4 };
    inline constexpr int ProcessVoiceData = ProcessVoiceDataOffsets[OFFSET_INDEX];

    inline constexpr int CreateVMOffsets[NUM_CONFIGS] = { -1, 5, 6 };
    inline constexpr int CreateVM = CreateVMOffsets[OFFSET_INDEX];

    inline constexpr int DestroyVMOffsets[NUM_CONFIGS] = { -1, 6, 7 };
    inline constexpr int DestroyVM = DestroyVMOffsets[OFFSET_INDEX];
}

#undef OFFSET_INDEX
#undef NUM_CONFIGS