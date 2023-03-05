
#pragma once

struct datamap_t;
class CBaseEntity;

namespace DatamapHelpers
{
    int GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName);
    int GetDatamapVarOffsetFromEnt(CBaseEntity* pEntity, const char* szVarName);
}
