
#pragma once

class ServerClass;
class IVEngineServer;
class IServerGameDLL;
class bf_write;

namespace SendTablesFix
{
    bool ReconstructPartialSendTablesForModification(ServerClass* modifiedClass, IVEngineServer* engineServer);
    bool WriteFullSendTables(IServerGameDLL* serverGameDll, bf_write& buf);
}
