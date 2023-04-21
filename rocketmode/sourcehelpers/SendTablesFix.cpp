
#include "SendTablesFix.h"
#include "sourcesdk/common/netmessages.h"
#include "sourcesdk/engine/dt.h"
#include "sourcesdk/engine/net.h"
#include "sourcesdk/public/dt_send.h"
#include "sourcesdk/public/eiface.h"
#include "sourcesdk/public/server_class.h"
#include "sourcesdk/public/tier1/utlvector.h"
#include "sourcesdk/public/tier1/bitbuf.h"

class CEventInfo;
struct edict_t;

struct CGameServerHack
{
    edict_t* edicts;			// Can array index now, edict_t is fixed
    IChangeInfoAccessor* edictchangeinfo; // HACK to allow backward compat since we can't change edict_t layout

    int m_nMaxClientsLimit;    // Max allowed on server.

    bool allowsignonwrites;
    bool dll_initialized;    // Have we loaded the game dll.

    bool m_bIsLevelMainMenuBackground;	// true if the level running only as the background to the main menu

    CUtlVector<CEventInfo*>	m_TempEntities;		// temp entities

    bf_write m_FullSendTables;
    CUtlMemory<char> m_FullSendTablesBuffer;
};

static void DataTable_ClearWriteFlags_R(SendTable* pTable)
{
    pTable->SetWriteFlag(false);

    for (int i = 0; i < pTable->m_nProps; i++)
    {
        SendProp* pProp = &pTable->m_pProps[i];
        if (pProp->m_Type == DPT_DataTable)
        {
            DataTable_ClearWriteFlags_R(pProp->GetDataTable());
        }
    }
}

static void DataTable_ClearWriteFlags(ServerClass* pClasses)
{
    for (ServerClass* pCur = pClasses; pCur; pCur = pCur->m_pNext)
    {
        DataTable_ClearWriteFlags_R(pCur->m_pTable);
    }
}

static bool SendTable_WriteInfos(SendTable* pTable, bf_write* pBuf)
{
    pBuf->WriteString(pTable->GetName());
    pBuf->WriteUBitLong(pTable->GetNumProps(), PROPINFOBITS_NUMPROPS);

    // Send each property.
    for (int iProp = 0; iProp < pTable->m_nProps; iProp++)
    {
        const SendProp* pProp = &pTable->m_pProps[iProp];

        pBuf->WriteUBitLong((unsigned int)pProp->m_Type, PROPINFOBITS_TYPE);
        pBuf->WriteString(pProp->GetName());
        // we now have some flags that aren't networked so strip them off
        unsigned int networkFlags = pProp->GetFlags() & ((1 << PROPINFOBITS_FLAGS) - 1);
        pBuf->WriteUBitLong(networkFlags, PROPINFOBITS_FLAGS);

        if (pProp->m_Type == DPT_DataTable)
        {
            // Just write the name and it will be able to reuse the table with a matching name.
            pBuf->WriteString(pProp->GetDataTable()->m_pNetTableName);
        }
        else
        {
            if (pProp->IsExcludeProp())
            {
                pBuf->WriteString(pProp->GetExcludeDTName());
            }
            else if (pProp->GetType() == DPT_Array)
            {
                pBuf->WriteUBitLong(pProp->GetNumElements(), PROPINFOBITS_NUMELEMENTS);
            }
            else
            {
                pBuf->WriteBitFloat(pProp->m_fLowValue);
                pBuf->WriteBitFloat(pProp->m_fHighValue);
                pBuf->WriteUBitLong(pProp->m_nBits, PROPINFOBITS_NUMBITS);
            }
        }
    }

    return !pBuf->IsOverflowed();
}

// If the table's ID is -1, writes its info into the buffer and increments curID.
static void SV_MaybeWriteSendTable(SendTable* pTable, bf_write& pBuf, bool bNeedDecoder)
{
    // Already sent?
    if (pTable->GetWriteFlag())
        return;

    pTable->SetWriteFlag(true);

    alignas(4) byte tmpbuf[8192];
    bf_write dataOut;
    dataOut.StartWriting(tmpbuf, sizeof(tmpbuf));

    // write send table properties into message buffer
    bool notOverflowed = SendTable_WriteInfos(pTable, &dataOut);
    assert(notOverflowed);

    // write message to stream
    notOverflowed = SVC_SendTable::WriteToBuffer(pBuf, bNeedDecoder, dataOut);
    assert(notOverflowed);
}

// Calls SV_MaybeWriteSendTable recursively.
static void SV_MaybeWriteSendTable_R(SendTable* pTable, bf_write& pBuf)
{
    SV_MaybeWriteSendTable(pTable, pBuf, false);

    // Make sure we send child send tables..
    for (int i = 0; i < pTable->m_nProps; i++)
    {
        SendProp* pProp = &pTable->m_pProps[i];
        if (pProp->m_Type == DPT_DataTable)
        {
            SV_MaybeWriteSendTable_R(pProp->GetDataTable(), pBuf);
        }
    }
}

static void SV_WriteSingleClassSendTables(ServerClass* pClass, bf_write& pBuf)
{
    DataTable_ClearWriteFlags_R(pClass->GetTable());
    SV_MaybeWriteSendTable(pClass->GetTable(), pBuf, true);
    SV_MaybeWriteSendTable_R(pClass->GetTable(), pBuf);
}

static void SV_WriteClassInfos(ServerClass* pClasses, bf_write& pBuf)
{
    int numClasses = 0;
    for (ServerClass* pClass = pClasses; pClass; pClass = pClass->m_pNext)
    {
        numClasses += 1;
    }

    SVC_ClassInfo::class_t* classes = new SVC_ClassInfo::class_t[numClasses];
    int i = 0;
    for (ServerClass* pClass = pClasses; pClass; pClass = pClass->m_pNext)
    {
        SVC_ClassInfo::class_t& svclass = classes[i++];

        svclass.classID = pClass->m_ClassID;
        strncpy(svclass.datatablename, pClass->m_pTable->GetName(), sizeof(svclass.datatablename));
        strncpy(svclass.classname, pClass->m_pNetworkName, sizeof(svclass.classname));
    }
    assert(i == numClasses);

    SVC_ClassInfo::WriteToBuffer(pBuf, false, classes, numClasses);

    delete[] classes;
}

bool SendTablesFix::ReconstructPartialSendTablesForModification(ServerClass* modifiedClass, IVEngineServer* engineServer)
{
    // When sv_sendtables=1 and the server goes to build m_FullSendTables:
    // - In SV_MaybeWriteSendTable, CTFPlayer m_ConditionData overflows the 4096 byte buffer.
    // - m_nLength in SVC_SendTable::WriteToBuffer for that same table is beyond the range of int16.
    //
    // These issues corrupt the net msg stream.
    // This code reimplements that path of building m_FullSendTables to work around those issues.
    const CGameServerHack* hack = nullptr;
    {
        const int* worldspawn = reinterpret_cast<const int*>(engineServer->PEntityOfEntIndex(0));
        const int** edicts = reinterpret_cast<const int**>(engineServer->GetIServer());
        while (*edicts != worldspawn)
        {
            edicts += 1;
        }
        hack = reinterpret_cast<const CGameServerHack*>(edicts);
    }

    bf_write& fullSendTables = *const_cast<bf_write*>(&hack->m_FullSendTables);
    CUtlMemory<char>& fullSendTablesBuffer = *const_cast<CUtlMemory<char>*>(&hack->m_FullSendTablesBuffer);

    fullSendTablesBuffer.EnsureCapacity(NET_MAX_PAYLOAD);
    fullSendTables.StartWriting(fullSendTablesBuffer.Base(), fullSendTablesBuffer.Count());

    // Send the modified SendTables first.
    // This puts them at the front of g_ClientSendTables.
    // When the send/recv linking step happens, it'll catch our modified tables first.
    SV_WriteSingleClassSendTables(modifiedClass, fullSendTables);
    if (fullSendTables.IsOverflowed())
    {
        return false;
    }

    // Create the rest of the tables from the client records.
    const bool createOnClient = true;
    SVC_ClassInfo::WriteToBuffer(fullSendTables, createOnClient, nullptr, 0);
    if (fullSendTables.IsOverflowed())
    {
        return false;
    }

    return true;
}

bool SendTablesFix::WriteFullSendTables(IServerGameDLL* serverGameDll, bf_write& buf)
{
    ServerClass* serverClasses = serverGameDll->GetAllServerClasses();

    DataTable_ClearWriteFlags(serverClasses);
    for (ServerClass* serverClass = serverClasses; serverClass; serverClass = serverClass->m_pNext)
    {
        SV_MaybeWriteSendTable(serverClass->m_pTable, buf, true);
    }
    for (ServerClass* serverClass = serverClasses; serverClass; serverClass = serverClass->m_pNext)
    {
        SV_MaybeWriteSendTable_R(serverClass->m_pTable, buf);
    }
    SV_WriteClassInfos(serverClasses, buf);

    return !buf.IsOverflowed();
}
