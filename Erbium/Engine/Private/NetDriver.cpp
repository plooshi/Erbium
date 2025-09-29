#include "pch.h"
#include "../Public/NetDriver.h"
#include "../../Erbium/Public/Finders.h"

void UNetDriver::TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
    static auto HasReplicationDriver_ = Driver->HasReplicationDriver();

    if (HasReplicationDriver_ ? Driver->ReplicationDriver : nullptr)
    {
        ((void (*)(UObject*, float)) FindServerReplicateActors())(Driver->ReplicationDriver, DeltaSeconds);
    }

    return TickFlushOG(Driver, DeltaSeconds);
}

void UNetDriver::Hook()
{
    Utils::Hook(FindTickFlush(), TickFlush, TickFlushOG);
}