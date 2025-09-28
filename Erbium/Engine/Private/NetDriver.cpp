#include "pch.h"
#include "../Public/NetDriver.h"
#include "../../Erbium/Public/Finders.h"

void UNetDriver::TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
    static auto ReplicationDriverOffset = 0;
    if (ReplicationDriverOffset == 0)
    {
        ReplicationDriverOffset = Driver->GetOffset("ReplicationDriver");
    }

    auto ReplicationDriver = ReplicationDriverOffset ? GetFromOffset<UObject*>(Driver, ReplicationDriverOffset) : nullptr;
    if (ReplicationDriver)
    {
        ((void (*)(UObject*, float)) FindServerReplicateActors())(Driver->ReplicationDriver, DeltaSeconds);
    }

    return TickFlushOG(Driver, DeltaSeconds);
}

void UNetDriver::Hook()
{
    Utils::Hook(FindTickFlush(), TickFlush, TickFlushOG);
}