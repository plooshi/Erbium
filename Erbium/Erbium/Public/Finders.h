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

inline std::vector<uint64_t> NullFuncs = {};
inline std::vector<uint64_t> RetTrueFuncs = {};

void FindNullsAndRetTrues();