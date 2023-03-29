
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
    // CBaseEntity
    inline constexpr int SetOwnerEntityOffsets[NUM_CONFIGS] = { 17, 18, 19 };
    inline constexpr int SetOwnerEntity = SetOwnerEntityOffsets[OFFSET_INDEX];

    inline constexpr int SpawnOffsets[NUM_CONFIGS] = { 22, 24, 25 };
    inline constexpr int Spawn = SpawnOffsets[OFFSET_INDEX];

    inline constexpr int ChangeTeamOffsets[NUM_CONFIGS] = { 91, 96, 97 };
    inline constexpr int ChangeTeam = ChangeTeamOffsets[OFFSET_INDEX];

    inline constexpr int StartTouchOffsets[NUM_CONFIGS] = { 98, 103, 104 };
    inline constexpr int StartTouch = StartTouchOffsets[OFFSET_INDEX];

    // CBasePlayer
    inline constexpr int GetNextObserverSearchStartPointOffsets[NUM_CONFIGS] = { 381, 390, 391 };
    inline constexpr int GetNextObserverSearchStartPoint = GetNextObserverSearchStartPointOffsets[OFFSET_INDEX];

    inline constexpr int PlayerRunCommandOffsets[NUM_CONFIGS] = { 421, 430, 431 };
    inline constexpr int PlayerRunCommand = PlayerRunCommandOffsets[OFFSET_INDEX];

    // IClient
    inline constexpr int IsProximityHearingClientOffsets[NUM_CONFIGS] = { 38, 38, 39 };
    inline constexpr int IsProximityHearingClient = IsProximityHearingClientOffsets[OFFSET_INDEX];
    
    inline constexpr int ProcessVoiceDataOffsets[NUM_CONFIGS] = { 3, 3, 4 };
    inline constexpr int ProcessVoiceData = ProcessVoiceDataOffsets[OFFSET_INDEX];

    // IScriptManager
    inline constexpr int CreateVMOffsets[NUM_CONFIGS] = { -1, 5, 6 };
    inline constexpr int CreateVM = CreateVMOffsets[OFFSET_INDEX];

    inline constexpr int DestroyVMOffsets[NUM_CONFIGS] = { -1, 6, 7 };
    inline constexpr int DestroyVM = DestroyVMOffsets[OFFSET_INDEX];

    // CGameRules
    inline constexpr int WeaponTraceEntityOffsets[NUM_CONFIGS] = { 121, 121, 122 };
    inline constexpr int WeaponTraceEntity = WeaponTraceEntityOffsets[OFFSET_INDEX];
}

#undef OFFSET_INDEX
#undef NUM_CONFIGS
