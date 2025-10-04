#include "pch.h"
#include "../Public/NetDriver.h"
#include "../../Erbium/Public/Finders.h"
#include "../../FortniteGame/Public/FortGameModeAthena.h"

void UNetDriver::TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
    static auto HasReplicationDriver_ = Driver->HasReplicationDriver();

    if (HasReplicationDriver_ ? Driver->ReplicationDriver : nullptr)
    {
        ((void (*)(UObject*, float)) FindServerReplicateActors())(Driver->ReplicationDriver, DeltaSeconds);
    }

    static bool bStartedBus = false;
    if (!bStartedBus && VersionInfo.FortniteVersion >= 15.30)
    { 
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->bWorldIsReady && ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WarmupCountdownEndTime <= Time)
        {
            bStartedBus = true;

            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
        }
    }

    return TickFlushOG(Driver, DeltaSeconds);
}

void UNetDriver::Hook()
{
    Utils::Hook(FindTickFlush(), TickFlush, TickFlushOG);
}
