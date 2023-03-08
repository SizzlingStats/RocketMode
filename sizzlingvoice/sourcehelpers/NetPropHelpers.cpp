
#include "NetPropHelpers.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/dt_common.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/server_class.h"
#include <string.h>
#include <stdio.h>

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
    char mString[N+1];
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

void NetPropHelpers::PrintAllServerClassTables(IServerGameDLL* serverGameDll)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        PrintServerClass(pClass);
        pClass = pClass->GetNext();
    }
}

void NetPropHelpers::PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = GetServerClass(serverGameDll, classname);
    if (pClass)
    {
        PrintServerClass(pClass);
    }
}

ServerClass* NetPropHelpers::GetServerClass(IServerGameDLL* serverGameDll, const char* classname)
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

SendProp* NetPropHelpers::GetProp(IServerGameDLL* serverGameDll, const char* className, const char* tableName, const char* propName)
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

SendProp* NetPropHelpers::GetProp(ServerClass* serverClass, const char* tableName, const char* propName)
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
