
#pragma once

enum PLUGIN_RESULT
{
    PLUGIN_CONTINUE = 0, // keep going
    PLUGIN_OVERRIDE, // run the game dll function but use our return value instead
    PLUGIN_STOP, // don't run the game dll function at all
};

enum EQueryCvarValueStatus
{
    eQueryCvarValueStatus_ValueIntact = 0,	// It got the value fine.
    eQueryCvarValueStatus_CvarNotFound = 1,
    eQueryCvarValueStatus_NotACvar = 2,		// There's a ConCommand, but it's not a ConVar.
    eQueryCvarValueStatus_CvarProtected = 3	// The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
};

