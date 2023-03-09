
#include "EntityHelpers.h"

#include "sourcesdk/game/server/baseentity.h"
#include "sourcesdk/public/datamap.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/edict.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/server_class.h"

#include <string.h>
#include <stdio.h>

int EntityHelpers::GetDatamapVarOffset(datamap_t* pDatamap, const char* szVarName)
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

void EntityHelpers::FullStateChanged(edict_t* edict, IVEngineServer* engineServer)
{
    edict->m_fStateFlags |= (FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
    IChangeInfoAccessor* accessor = engineServer->GetChangeAccessor(edict);
    accessor->SetChangeInfoSerialNumber(0);
}

void EntityHelpers::StateChanged(edict_t* edict, unsigned short offset, IVEngineServer* engineServer)
{
    if (edict->m_fStateFlags & FL_FULL_EDICT_CHANGED)
        return;

    edict->m_fStateFlags |= FL_EDICT_CHANGED;

    IChangeInfoAccessor* accessor = engineServer->GetChangeAccessor(edict);
    CSharedEdictChangeInfo* sharedChangeInfo = engineServer->GetSharedEdictChangeInfo();

    if (accessor->GetChangeInfoSerialNumber() == sharedChangeInfo->m_iSerialNumber)
    {
        // Ok, I still own this one.
        CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];

        // Now add this offset to our list of changed variables.		
        for (unsigned short i = 0; i < p->m_nChangeOffsets; i++)
            if (p->m_ChangeOffsets[i] == offset)
                return;

        if (p->m_nChangeOffsets == MAX_CHANGE_OFFSETS)
        {
            // Invalidate our change info.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED; // So we don't get in here again.
        }
        else
        {
            p->m_ChangeOffsets[p->m_nChangeOffsets++] = offset;
        }
    }
    else
    {
        if (sharedChangeInfo->m_nChangeInfos == MAX_EDICT_CHANGE_INFOS)
        {
            // Shucks.. have to mark the edict as fully changed because we don't have room to remember this change.
            accessor->SetChangeInfoSerialNumber(0);
            edict->m_fStateFlags |= FL_FULL_EDICT_CHANGED;
        }
        else
        {
            // Get a new CEdictChangeInfo and fill it out.
            accessor->SetChangeInfo(sharedChangeInfo->m_nChangeInfos);
            sharedChangeInfo->m_nChangeInfos++;

            accessor->SetChangeInfoSerialNumber(sharedChangeInfo->m_iSerialNumber);

            CEdictChangeInfo* p = &sharedChangeInfo->m_ChangeInfos[accessor->GetChangeInfo()];
            p->m_ChangeOffsets[0] = offset;
            p->m_nChangeOffsets = 1;
        }
    }
}

static const char* sPropTypeNames[7] =
{
    "int", "float", "vector", "vectorxy", "string", "array", "datatable"
};

template<typename T, int N>
constexpr int ArrayLength(const T(&)[N]) { return N; }

template<int N>
struct StringBuilder
{
public:
    StringBuilder() :
        mLength(0)
    {
        mString[0] = '\0';
    }

    void Append(const char* str)
    {
        int length = mLength;
        char* thisStr = mString;

        int index = 0;
        while ((length < N) && (str[index] != '\0'))
        {
            thisStr[length++] = str[index++];
        }
        thisStr[length] = '\0';
        mLength = length;
    }

    void Reduce(int numChars)
    {
        int newLength = mLength - numChars;
        if (newLength < 0)
        {
            newLength = 0;
        }
        mLength = newLength;
        mString[newLength] = '\0';
    }

    const char* c_str() const
    {
        return mString;
    }

private:
    int mLength;
    char mString[N + 1];
};

static void RecurseServerTable(SendTable* pTable, StringBuilder<128>& spacing)
{
    printf("%s%s\n", spacing.c_str(), pTable->GetName());
    spacing.Append("  |");

    int num = pTable->GetNumProps();
    for (int i = 0; i < num; i++)
    {
        SendProp* pProp = pTable->GetProp(i);
        SendPropType PropType = pProp->m_Type;
        const char* typeString = nullptr;
        if (PropType < ArrayLength(sPropTypeNames))
        {
            typeString = sPropTypeNames[PropType];
        }

        printf("%s%s, Offset: %i (%s, %i bits)\n",
            spacing.c_str(), pProp->GetName(), pProp->GetOffset(), typeString, pProp->m_nBits);

        SendTable* subTable = pProp->GetDataTable();
        if (subTable)
        {
            spacing.Append("  |");
            RecurseServerTable(subTable, spacing);
        }
    }
    spacing.Reduce(6);
}

static void PrintServerClass(ServerClass* serverclass)
{
    printf("%s\n", serverclass->GetName());
    SendTable* table = serverclass->GetTable();
    if (table)
    {
        StringBuilder<128> spacing;
        spacing.Append("  |");
        RecurseServerTable(table, spacing);
        printf("\n");
    }
}

void EntityHelpers::PrintAllServerClassTables(IServerGameDLL* serverGameDll)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        PrintServerClass(pClass);
        pClass = pClass->GetNext();
    }
}

void EntityHelpers::PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = GetServerClass(serverGameDll, classname);
    if (pClass)
    {
        PrintServerClass(pClass);
    }
}

ServerClass* EntityHelpers::GetServerClass(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        const char* name = pClass->GetName();
        if (!strcmp(classname, name))
        {
            return pClass;
        }
        pClass = pClass->m_pNext;
    }
    return nullptr;
}

static SendTable* GetTableRecursive(SendTable* table, const char* name)
{
    if (!table)
    {
        return nullptr;
    }

    if (!strcmp(name, table->GetName()))
    {
        return table;
    }

    const int numProps = table->GetNumProps();
    for (int i = 0; i < numProps; ++i)
    {
        SendProp* prop = table->GetProp(i);
        SendTable* foundSubTable = GetTableRecursive(prop->GetDataTable(), name);
        if (foundSubTable)
        {
            return foundSubTable;
        }
    }
    return nullptr;
}

SendProp* EntityHelpers::GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName)
{
    ServerClass* pClass = GetServerClass(serverGameDll, className);
    if (pClass)
    {
        SendProp* prop = GetProp(pClass, tableName, propName);
        if (prop)
        {
            return prop;
        }
    }
    return nullptr;
}

SendProp* EntityHelpers::GetProp(ServerClass* serverClass, const char* tableName, const char* propName)
{
    SendTable* table = GetTableRecursive(serverClass->GetTable(), tableName);
    if (!table)
    {
        return nullptr;
    }

    const int numProps = table->GetNumProps();
    for (int i = 0; i < numProps; ++i)
    {
        SendProp* prop = table->GetProp(i);
        if ((prop->m_Flags & SPROP_EXCLUDE) != 0)
        {
            continue;
        }
        if (!strcmp(prop->GetName(), propName))
        {
            return prop;
        }
    }
    return nullptr;
}
