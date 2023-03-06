
#include "DatamapHelpers.h"
#include "sourcesdk/public/datamap.h"
#include "sourcesdk/game/server/baseentity.h"
#include <string.h>

int DatamapHelpers::GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName)
{
    while (pDatamap)
    {
        const int numFields = pDatamap->dataNumFields;
        typedescription_t* pFields = pDatamap->dataDesc;
        for (int i = 0; i < numFields; ++i)
        {
            typedescription_t* pField = &pFields[i];
            if (pField->fieldName && !strcmp(pField->fieldName, szVarName))
            {
                return pField->fieldOffset[TD_OFFSET_NORMAL];
            }
            else if (pField->td)
            {
                // there can be additional data tables inside this type description
                const int offset = GetDatamapVarOffset(pField->td, szVarName);
                if (offset > 0)
                {
                    return offset;
                }
            }
        }
        pDatamap = pDatamap->baseMap;
    }
    return 0;
}

int DatamapHelpers::GetDatamapVarOffsetFromEnt(CBaseEntity* pEntity, const char* szVarName)
{
    if (pEntity)
    {
        datamap_t* pDatamap = pEntity->GetDataDescMap();
        return GetDatamapVarOffset(pDatamap, szVarName);
    }
    return 0;
}
