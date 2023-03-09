
#pragma once

class ServerClass;
class IVEngineServer;

namespace SendTablesFix
{
    bool ReconstructFullSendTablesForModification(ServerClass* modifiedClass, IVEngineServer* engineServer);
}
