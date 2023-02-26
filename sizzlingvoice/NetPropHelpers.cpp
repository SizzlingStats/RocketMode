
#include "NetPropHelpers.h"
#include "sourcesdk/public/eiface.h"
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

void NetPropHelpers::PrintAllServerClassTables(IServerGameDLL* serverGameDll)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        printf("%s\n", pClass->GetName());
        SendTable* table = pClass->GetTable();
        if (table)
        {
            StringBuilder<128> spacing;
            spacing.Append("  |");
            RecurseServerTable(table, spacing);
            printf("\n");
        }
        pClass = pClass->GetNext();
    }
}

void NetPropHelpers::PrintServerClassTables(IServerGameDLL* serverGameDll, const char* classname)
{
    ServerClass* pClass = serverGameDll->GetAllServerClasses();
    while (pClass)
    {
        const char* name = pClass->GetName();
        if (!strcmp(classname, name))
        {
            printf("%s\n", name);
            SendTable* table = pClass->GetTable();
            if (table)
            {
                StringBuilder<128> spacing;
                spacing.Append("  |");
                RecurseServerTable(table, spacing);
                printf("\n");
            }
            return;
        }
        pClass = pClass->m_pNext;
    }
}
