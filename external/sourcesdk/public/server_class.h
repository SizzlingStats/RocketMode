
#pragma once

class SendTable;

class ServerClass
{
public:
    const char* GetName() const { return m_pNetworkName; }
    SendTable* GetTable() const { return m_pTable; }
    ServerClass* GetNext() const { return m_pNext; }

public:
    const char* m_pNetworkName;
    SendTable* m_pTable;
    ServerClass* m_pNext;
    int m_ClassID;	// Managed by the engine.

    // This is an index into the network string table (sv.GetInstanceBaselineTable()).
    int m_InstanceBaselineIndex; // INVALID_STRING_INDEX if not initialized yet.
};
