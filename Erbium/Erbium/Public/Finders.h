#pragma once
#include "../../pch.h"

uint64 FindGIsClient();
uint64 FindGIsServer();
uint64 FindGetNetMode();
uint64 FindGetWorldContext();
uint64 FindCreateNetDriver();
uint64 FindCreateNetDriverWorldContext();
uint64 FindInitListen();
uint64 FindSetWorld();
uint64 FindTickFlush();
uint64 FindServerReplicateActors();
uint64 FindSendRequestNow();
uint64 FindGetMaxTickRate();
uint64 FindGiveAbility();
uint64 FindConstructAbilitySpec();
uint64 FindInternalTryActivateAbility();
uint64 FindApplyCharacterCustomization();
uint64 FindHandlePostSafeZonePhaseChanged();
uint64 FindSpawnLoot();
uint64 FindFinishedTargetSpline();
uint64 FindPickTeam();
uint64 FindCantBuild();
uint64 FindReplaceBuildingActor();
uint64 FindKickPlayer();
uint64 FindEncryptionPatch();
uint64 FindRemoveInventoryItem();
uint64 FindRemoveInventoryStateValue();
uint64 FindSetInventoryStateValue();
uint64 FindOnRep_ZiplineState();
uint64 FindGiveAbilityAndActivateOnce();
uint64 FindGameSessionPatch();
uint64 FindRemoveFromAlivePlayers();
uint64 FindStartAircraftPhase();
uint64 FindSetPickupItems();
uint64 FindCallPreReplication();
uint64 FindSendClientAdjustment();
uint64 FindSetChannelActor();
uint64 FindSetChannelActorForDestroy();
uint64 FindCreateChannel();
uint64 FindReplicateActor();
uint64 FindCloseActorChannel();
uint64 FindClientHasInitializedLevelFor();
uint64 FindStartBecomingDormant();
uint64 FindFlushDormancy();
int32 FindIsNetRelevantForVft();
uint64 FindSendDestructionInfo();
uint64 FindEnterAircraft();
uint64 FindClearAbility();
uint64 FindGetPlayerViewPoint();
uint32 FindOnItemInstanceAddedVft();
uint64 FindGetNamePool();
uint64 FindIsNetReady();
uint64 FindSpawnInitialSafeZone();
uint64 FindUpdateSafeZonesPhase();
uint64 FindUpdateIrisReplicationViews();
uint64 FindPreSendUpdate();
uint64 FindHandleMatchHasStarted();
uint64 FindInitializeBuildingActor();
uint64 FindPostInitializeSpawnedBuildingActor();
uint64 FindInitializeFlightPath();
uint64 FindReset();
uint64 FindNotifyGameMemberAdded();
uint64 FindSetGamePhase();
uint64 FindPayBuildableClassPlacementCost();
uint64 FindCanAffordToPlaceBuildableClass();
uint64 FindCanPlaceBuildableClassInStructuralGrid();
uint64 FindLoadPlayset(const std::vector<uint8_t>& Bytes = std::vector<uint8_t>({ 0x48, 0x89, 0x5C }), int recursive = 0);
uint32 FindSpawnDecoVft();
uint32 FindShouldAllowServerSpawnDecoVft();
uint64 FindSetState();
uint64 FindMinigameSettingsBuilding__BeginPlay();
uint64 FindPickSupplyDropLocation();
uint64 FindSetPickupTarget();
uint64 FindInitializePlayerGameplayAbilities();
uint64 FindListenCall();
template <typename CVarT>
CVarT* FindCVar(const wchar_t* CVarStr)
{
    auto sRef = Memcury::Scanner::FindStringRef(CVarStr);

    if (!sRef.IsValid())
        return nullptr;

    uint64_t BeforeVars = 0;

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x83 && *(Ptr + 2) == 0xEC)
        {
            BeforeVars = uint64_t(Ptr);
            break;
        }
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x81 && *(Ptr + 2) == 0xEC)
        {
            BeforeVars = uint64_t(Ptr);
            break;
        }
    }


    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(BeforeVars + i);

        if (*Ptr == 0x4C && *(Ptr + 1) == 0x8D && *(Ptr + 2) == 0x05)
            return Memcury::Scanner(Ptr).RelativeOffset(3).GetAs<CVarT*>();
    }

    return nullptr;
}


inline std::vector<uint64_t> NullFuncs = {};
inline std::vector<uint64_t> RetTrueFuncs = {};

void FindNullsAndRetTrues();