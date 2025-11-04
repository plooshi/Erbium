#include "pch.h"
#include "../Public/NetDriver.h"
#include "../../Erbium/Public/Finders.h"
#include "../../FortniteGame/Public/FortGameModeAthena.h"
#include "../../Erbium/Public/GUI.h"
#include "../../Erbium/Public/Configuration.h"
#include "../../FortniteGame/Public/BattleRoyaleGamePhaseLogic.h"

uint32_t NetworkObjectListOffset = 0;
uint32_t ReplicationFrameOffset = 0;
uint32_t ClientWorldPackageNameOffset = 0;
uint32_t DestroyedStartupOrDormantActorsOffset = 0;
uint32_t DestroyedStartupOrDormantActorGUIDsOffset = 0;
uint32_t ClientVisibleLevelNamesOffset = 0;

std::unordered_map<UNetConnection*, TArray<FNetViewer*>> ViewerMap;


const ULevel* GetLevel(const AActor* Actor)
{
	auto Outer = Actor->Outer;

	while (Outer)
	{
		if (Outer->Class == ULevel::StaticClass())
			return (ULevel*)Outer;
		else
			Outer = Outer->Outer;
	}

	return nullptr;
}


void BuildViewerMap(UNetDriver* Driver)
{
	//Log(L"PC");
	for (auto& Conn : Driver->ClientConnections)
	{
		auto Owner = Conn->OwningActor;
		if (Owner)
		{
			auto OutViewTarget = Owner;
			if (auto Controller = Conn->PlayerController)
				if (auto ViewTarget = Controller->GetViewTarget())
						OutViewTarget = ViewTarget;

			Conn->ViewTarget = OutViewTarget;

			TArray<FNetViewer*> Viewers;
			Viewers.Reserve(1 + Conn->Children.Num());
			Viewers.Add(FNetViewer::Create(Conn));

			for (auto& Child : Conn->Children)
			{
				if (auto Controller = Child->PlayerController)
				{
					Child->ViewTarget = Controller->GetViewTarget();
					Viewers.Add(FNetViewer::Create(Child));
				}
				else
					Child->ViewTarget = nullptr;
			}
			ViewerMap[Conn] = Viewers;
		}
		else
		{
			Conn->ViewTarget = nullptr;
			for (auto& Child : Conn->Children)
				Child->ViewTarget = nullptr;
		}
	}
}

static FNetworkObjectList& GetNetworkObjectList(UNetDriver* Driver)
{
	return *(*(class TSharedPtr<FNetworkObjectList>*)(__int64(Driver) + NetworkObjectListOffset));
}

UNetConnection* IsActorOwnedByAndRelevantToConnection(const AActor* Actor, TArray<FNetViewer*>& ConnViewers, bool& bOutHasNullViewTarget)
{
	auto IsNetRelevantForIdx = FindIsNetRelevantForVft();
	if (IsNetRelevantForIdx == 0)
		return ConnViewers[0]->Connection;

	bool (*&IsRelevancyOwnerFor)(const AActor*, const AActor*, const AActor*, const AActor*) = decltype(IsRelevancyOwnerFor)(Actor->Vft[IsNetRelevantForIdx + 2]);
	AActor* (*&GetNetOwner)(const AActor*) = decltype(GetNetOwner)(Actor->Vft[IsNetRelevantForIdx + (VersionInfo.EngineVersion >= 4.19 ? 6 : 5)]);

	const AActor* ActorOwner = GetNetOwner(Actor);

	bOutHasNullViewTarget = false;

	for (auto& Viewer : ConnViewers)
	{
		auto Conn = Viewer->Connection;

		if (Conn->ViewTarget == nullptr)
		{
			bOutHasNullViewTarget = true;
		}

		if (ActorOwner == Conn->PlayerController ||
			(Conn->PlayerController && ActorOwner == Conn->PlayerController->Pawn) ||
			(Conn->ViewTarget && IsRelevancyOwnerFor(Conn->ViewTarget, Actor, ActorOwner, Conn->OwningActor)))
		{
			return Conn;
		}
	}

	return nullptr;
}

const UClass* WorldPartitionHLODClass = nullptr;

bool IsActorRelevantToConnection(const AActor* Actor, const TArray<FNetViewer*>& ConnectionViewers)
{
	auto IsNetRelevantForIdx = FindIsNetRelevantForVft();
	if (IsNetRelevantForIdx == 0)
		return true;

	if (WorldPartitionHLODClass && Actor->IsA(WorldPartitionHLODClass))
		return true;

	bool (*&IsNetRelevantFor)(const AActor*, const AActor*, const AActor*, const FVector&) = decltype(IsNetRelevantFor)(Actor->Vft[IsNetRelevantForIdx]);

	for (auto& Viewer : ConnectionViewers)
	{
		if (!Viewer)
			continue;

		if (IsNetRelevantFor(Actor, Viewer->InViewer, Viewer->ViewTarget, Viewer->ViewLocation))
		{
			return true;
		}
	}

	return false;
}

bool IsLevelInitializedForActor(const UNetDriver* NetDriver, const AActor* InActor, UNetConnection* InConnection)
{
	static bool (*ClientHasInitializedLevelFor)(const UNetConnection*, const UObject*) = decltype(ClientHasInitializedLevelFor)(FindClientHasInitializedLevelFor());

	const bool bCorrectWorld = NetDriver->WorldPackage != nullptr && 
		(!ClientWorldPackageNameOffset || *(FName*)(__int64(InConnection) + ClientWorldPackageNameOffset) == NetDriver->WorldPackage->Name) && 
		(!ClientHasInitializedLevelFor || ClientHasInitializedLevelFor(InConnection, InActor));
	const bool bIsConnectionPC = (InActor == InConnection->PlayerController);
	return bCorrectWorld || bIsConnectionPC;
}

struct FPrioActor
{
	FNetworkObjectInfo* ActorInfo;
	UActorChannel* Channel;
	float Priority;

	bool operator<(FPrioActor& _Rhs)
	{
		return Priority < _Rhs.Priority;
	}
};

std::unordered_map<UNetConnection*, UEAllocatedVector<FPrioActor>> PriorityLists;

void (*GetActorLocation)(AActor*, FFrame&, FVector*);
void ServerReplicateActors(UNetDriver* Driver, float DeltaSeconds)
{
	if (!ReplicationFrameOffset)
		return;

	(*(int*)(__int64(Driver) + ReplicationFrameOffset))++;

	BuildViewerMap(Driver);
	if (ViewerMap.size() == 0)
		return;

	static FName ActorName = FName(L"Actor");

	auto& NetworkObjectList = GetNetworkObjectList(Driver);
	auto& ActiveNetworkObjects = NetworkObjectList.ActiveNetworkObjects;
	auto IsNetReady = (int32(*)(UNetConnection*, bool))FindIsNetReady();
	static auto CloseActorChannel = (void(*)(UActorChannel*, uint8_t)) FindCloseActorChannel();

	for (auto& ViewerPair : ViewerMap)
	{
		auto& Conn = ViewerPair.first;
		auto& Viewers = ViewerPair.second;

		UEAllocatedVector<FPrioActor> List;
		List.reserve(ActiveNetworkObjects.Num());

		PriorityLists[Conn] = List;
	}

	FFrame FakeStack;
	for (auto& ActorInfo : ActiveNetworkObjects)
	{
		//if (!ActorInfo->bPendingNetUpdate && Time <= ActorInfo->NextUpdateTime)
		//	continue;

		auto Actor = ActorInfo->Actor;

		if (!Actor || /*!Actor->bActorInitialized || */Actor->NetDriverName != Driver->NetDriverName)
		{
			continue;
		}

		auto Outer = Actor->Outer;
		if (Actor->bActorIsBeingDestroyed || (TUObjectArray::GetItemByIndex(Actor->Index)->Flags & ((1 << 29) | (1 << 21))) || Actor->RemoteRole == 0 || ((Actor->HasbNetStartup() ? Actor->bNetStartup : false) && Actor->NetDormancy == 4))
		{
			//RemoveNetworkActor(&NetworkObjectList, Actor);
			continue;
		}

		bool bAnyRelevant = false;
		for (auto& ViewerPair : ViewerMap)
		{
			auto Conn = ViewerPair.first;
			auto& Viewers = ViewerPair.second;
			UActorChannel* Channel = nullptr;

			for (auto& Chan : Conn->OpenChannels)
			{
				if (Chan->Class != UActorChannel::StaticClass() || Chan->Actor != Actor)	
					continue;

				Channel = Chan;
				break;
			}

			bool bRelevant = IsActorRelevantToConnection(Actor, Viewers);
			bool bLevelInitializedForActor = IsLevelInitializedForActor(Driver, Actor, Conn);
			if (!Channel && (!bRelevant || !bLevelInitializedForActor))
			{
				continue;
			}

			if (Channel && !Actor->bTearOff && !bRelevant && (!bLevelInitializedForActor || !(Actor->HasbNetStartup() ? Actor->bNetStartup : true)))
			{
				CloseActorChannel(Channel, 3);
				continue;
			}

			auto PriorityConn = Conn;
			bool bDoCullCheck = true;

			if (Actor->bAlwaysRelevant || Actor->bTearOff)
				bDoCullCheck = false;

			if (Actor->bOnlyRelevantToOwner)
			{
				bDoCullCheck = false;
				bool bHasNullViewTarget = false;

				if (!(PriorityConn = IsActorOwnedByAndRelevantToConnection(Actor, Viewers, bHasNullViewTarget)))
				{
					if (!bHasNullViewTarget && Channel != NULL && !bRelevant)
						CloseActorChannel(Channel, 3);
					
					continue;
				}
			}
			else
			{
				if (VersionInfo.FortniteVersion == 1.72 || VersionInfo.FortniteVersion == 0.00)
				{
					auto& DormantConnections = *(TSet<TWeakObjectPtr<UNetConnection>>*)(__int64(ActorInfo.Get()) + 0x28);
					
					if (DormantConnections.Contains(Conn))
						continue;
				}
				else if (ActorInfo->DormantConnections.Contains(Conn))
					continue;

				static auto FlushDormancy = FindFlushDormancy();
				if (VersionInfo.FortniteVersion != 1.72 && VersionInfo.FortniteVersion != 0.00 && (VersionInfo.FortniteVersion >= 20 || FlushDormancy))
					if (Actor->GetNetDormancy() > 1 && Channel && !Channel->IsPendingDormancy() && !Channel->IsDormant())
						((int32(*)(UActorChannel*))FindStartBecomingDormant())(Channel);
			}

			if (Actor->bTearOff || (bRelevant && bLevelInitializedForActor))
			{
				bAnyRelevant = true;
				auto Priority = 0.f;

				static auto PCClass = FindClass("PlayerController");
				if (Actor->RootComponent && !Actor->IsA(PCClass))
				{
					FVector Loc;

					GetActorLocation(Actor, FakeStack, &Loc);

					float SmallestDisSquared = (std::numeric_limits<float>::max)();
					int32 ViewersThatSkipActor = 0;

					for (auto& Viewer : Viewers)
					{
						auto DistanceSquared = (Loc - Viewer->ViewLocation).SizeSquared();
						SmallestDisSquared = (float)min(SmallestDisSquared, DistanceSquared);

						if (bDoCullCheck && DistanceSquared > Actor->NetCullDistanceSquared)
							ViewersThatSkipActor++;
					}

					if (bDoCullCheck && ViewersThatSkipActor == Viewers.Num())
						continue;

					const float MaxDistanceScaling = 60000.f * 60000.f;

					const float DistanceFactor = std::clamp((SmallestDisSquared) / MaxDistanceScaling, 0.f, 1.f);

					Priority += DistanceFactor;
				}

				if (Actor->NetDormancy > 1)
					Priority -= 1.5f;

				for (auto& Viewer : Viewers)
					if (Actor == Viewer->ViewTarget || Actor == Viewer->InViewer)
						Priority = -(std::numeric_limits<float>::max)();


				auto& PriorityList = PriorityLists[Conn];
				PriorityList.push_back({ ActorInfo.Get(), Channel, Priority });
			}
		}

		if (bAnyRelevant)
			((void(*)(AActor*, UNetDriver*)) FindCallPreReplication())(Actor, Driver);
	}


	for (auto& PriorityListPair : PriorityLists)
	{
		auto Conn = PriorityListPair.first;
		auto& Viewers = ViewerMap[Conn];
		auto& PriorityActors = PriorityListPair.second;
		int i = 0;

		if (!Conn->ViewTarget)
			goto _out;

		std::sort(PriorityActors.begin(), PriorityActors.end());

		if (IsNetReady && VersionInfo.FortniteVersion < 22 && !IsNetReady(Conn, false))
			goto _out;

		if (DestroyedStartupOrDormantActorGUIDsOffset)
		{
			static auto& DestroyedStartupOrDormantActors = *(TMap<uint32, FActorDestructionInfo*>*)(__int64(Driver) + DestroyedStartupOrDormantActorsOffset);
			auto& DestroyedStartupOrDormantActorGUIDs = *(TSet<uint32>*)(__int64(Conn) + DestroyedStartupOrDormantActorGUIDsOffset);
			auto& ClientVisibleLevelNames = *(TSet<int32>*)(__int64(Conn) + ClientVisibleLevelNamesOffset);
			static auto SetChannelActorForDestroy = (void(*)(UActorChannel*, FActorDestructionInfo*)) FindSetChannelActorForDestroy();
			static auto SendDestructionInfo = (void(*)(UNetDriver*, UNetConnection*, FActorDestructionInfo*)) FindSendDestructionInfo();

			for (auto& NetGUID : DestroyedStartupOrDormantActorGUIDs)
			{
				auto Equals = [](const uint32& LeftKey, const uint32& RightKey) -> bool
					{
						return LeftKey == RightKey;
					};

				auto DestructionInfoPtr = DestroyedStartupOrDormantActors.Search([&](uint32& GUID, FActorDestructionInfo*& InfoUPtr)
					{
						return GUID == NetGUID/* && (InfoUPtr->StreamingLevelName == FName(0) || ClientVisibleLevelNames.Contains(InfoUPtr->StreamingLevelName.ComparisonIndex))*/;
					});

				if (DestructionInfoPtr)
				{
					auto DestructionInfo = *DestructionInfoPtr;

					if (SetChannelActorForDestroy)
					{
						auto Channel = ((UActorChannel * (*)(UNetConnection*, FName*, uint8_t, int))FindCreateChannel())(Conn, &ActorName, 2, -1);

						if (Channel)
							SetChannelActorForDestroy(Channel, DestructionInfo);
					}
					else if (SendDestructionInfo)
						SendDestructionInfo(Driver, Conn, DestructionInfo);
					//printf("Path: %s\n", DestructionInfo->PathName.ToString().c_str());
				}
			}
			DestroyedStartupOrDormantActorGUIDs.Reset();
		}


		static auto SendClientAdjustment = FindSendClientAdjustment();
		if (SendClientAdjustment)
			for (auto& Viewer : Viewers)
			{
				if (Viewer->Connection->PlayerController)
					((void(*)(AFortPlayerControllerAthena*)) SendClientAdjustment)(Viewer->Connection->PlayerController);
			}

		for (auto& PriorityActor : PriorityActors)
		{
			auto ActorInfo = PriorityActor.ActorInfo;

			UActorChannel* Channel = PriorityActor.Channel;

			if (!Channel || Channel->Actor)
			{
				auto Actor = ActorInfo->Actor;

				if (!Channel)
				{
					if (VersionInfo.FortniteVersion >= 20)
						Channel = ((UActorChannel * (*)(UNetConnection*, FName*, uint8_t, int))FindCreateChannel())(Conn, &ActorName, 2, -1);
					else
						Channel = ((UActorChannel*(*)(UNetConnection*, int, bool, int32_t))FindCreateChannel())(Conn, 2, true, -1);

					if (Channel)
						((void(*)(UActorChannel*, AActor*, uint8_t))FindSetChannelActor())(Channel, Actor, 0);
				}

				if (Channel)
				{
					if (VersionInfo.FortniteVersion >= 22 || (IsNetReady && IsNetReady(Conn, false))) // actually uchannel::isnetready
						((int32(*)(UActorChannel*))FindReplicateActor())(Channel);
					else
						Actor->ForceNetUpdate();

					if (IsNetReady && VersionInfo.FortniteVersion < 22 && !IsNetReady(Conn, false))
					{
						break;
					}
				}

				if (Channel && Actor->bTearOff)
					CloseActorChannel(Channel, 4);
			}
			i++;
		}

	_out:
		PriorityActors.clear();
	}
	PriorityLists.clear();

	for (auto& ViewerPair : ViewerMap)
	{
		for (auto& Viewer : ViewerPair.second)
			free(Viewer);

		ViewerPair.second.Free();
	}

	ViewerMap.clear();
}

void UNetDriver::TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
	if (VersionInfo.FortniteVersion >= 25.20)
	{
		auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
		GamePhaseLogic->Tick();
	}

	if (Driver->ClientConnections.Num() > 0)
		ServerReplicateActors(Driver, DeltaSeconds);

    if (GUI::gsStatus == 1 && VersionInfo.FortniteVersion >= 11.00)
    { 
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		static auto bSkipAircraft = GameState->CurrentPlaylistInfo.BasePlaylist ? GameState->CurrentPlaylistInfo.BasePlaylist->bSkipAircraft : false;
        if (!bSkipAircraft && GameState->HasWarmupCountdownEndTime() && Driver->ClientConnections.Num() > 0 && GameMode->bWorldIsReady && GameState->WarmupCountdownEndTime <= Time)
        {
			GUI::gsStatus = 2;

            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
        }
	}
	else if (GUI::gsStatus == 2 && FConfiguration::bAutoRestart)
	{
		auto WorldNetDriver = UWorld::GetWorld()->NetDriver;
		if (Driver == WorldNetDriver && Driver->ClientConnections.Num() == 0)
			TerminateProcess(GetCurrentProcess(), 0);
	}

    return TickFlushOG(Driver, DeltaSeconds);
}


void UNetDriver::TickFlush__RepGraph(UNetDriver* Driver, float DeltaSeconds)
{
	static auto ServerReplicateActors_ = FindServerReplicateActors();
	if (Driver->ReplicationDriver)
	{
		// this is our main netdriver
		if (Driver->ClientConnections.Num() > 0)
			((void (*)(UObject*, float)) ServerReplicateActors_)(Driver->ReplicationDriver, DeltaSeconds);


		if (GUI::gsStatus == 1 && VersionInfo.FortniteVersion >= 11.00)
		{
			auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			static auto bSkipAircraft = GameState->CurrentPlaylistInfo.BasePlaylist ? GameState->CurrentPlaylistInfo.BasePlaylist->bSkipAircraft : false;
			if (!bSkipAircraft && Driver->ClientConnections.Num() > 0 && GameMode->bWorldIsReady && GameState->WarmupCountdownEndTime <= Time)
			{
				GUI::gsStatus = 2;

				UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
			}
		}
		else if (GUI::gsStatus == 2 && FConfiguration::bAutoRestart)
		{
			if (Driver->ClientConnections.Num() == 0)
				TerminateProcess(GetCurrentProcess(), 0);
		}
	}

	return TickFlushOG(Driver, DeltaSeconds);
}


enum class EReplicationSystemSendPass : unsigned
{
	Invalid,
	PostTickDispatch,
	TickFlush,
};

struct FSendUpdateParams
{
	EReplicationSystemSendPass SendPass = EReplicationSystemSendPass::TickFlush;
	float DeltaSeconds = 0.f;
};

void SendClientMoveAdjustments(UNetDriver* Driver)
{
	static auto SendClientAdjustment = (void(*)(AFortPlayerControllerAthena*)) FindSendClientAdjustment();
	if (SendClientAdjustment)
	{
		for (UNetConnection* Connection : Driver->ClientConnections)
		{
			if (Connection == nullptr || Connection->ViewTarget == nullptr)
				continue;

			if (AFortPlayerControllerAthena* PC = Connection->PlayerController)
				SendClientAdjustment(PC);

			for (UNetConnection* ChildConnection : Connection->Children)
			{
				if (ChildConnection == nullptr)
					continue;

				if (AFortPlayerControllerAthena* PC = ChildConnection->PlayerController)
					SendClientAdjustment(PC);
			}
		}
	}
}

void UNetDriver::TickFlush__Iris(UNetDriver* Driver, float DeltaSeconds)
{
	auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
	GamePhaseLogic->Tick();

	if (Driver->ClientConnections.Num() > 0)
	{
		auto ReplicationSystem = *(UObject**)(__int64(&Driver->ReplicationDriver) + 8);

		if (ReplicationSystem)
		{
			static void(*UpdateIrisReplicationViews)(UNetDriver*) = decltype(UpdateIrisReplicationViews)(FindUpdateIrisReplicationViews());
			static void(*PreSendUpdate)(UObject*, FSendUpdateParams&) = decltype(PreSendUpdate)(FindPreSendUpdate());

			UpdateIrisReplicationViews(Driver);
			SendClientMoveAdjustments(Driver);
			FSendUpdateParams Params;
			Params.DeltaSeconds = DeltaSeconds;
			PreSendUpdate(ReplicationSystem, Params);
		}
	}

	if (GUI::gsStatus == 2 && FConfiguration::bAutoRestart)
	{
		auto WorldNetDriver = UWorld::GetWorld()->NetDriver;
		if (Driver == WorldNetDriver && Driver->ClientConnections.Num() == 0)
			TerminateProcess(GetCurrentProcess(), 0);
	}

	return TickFlushOG(Driver, DeltaSeconds);
}

void (*SetNetDormancyOG)(AActor* Actor, int NewDormancy);
void SetNetDormancy(AActor* Actor, int NewDormancy)
{
	auto Driver = (UNetDriver*)UWorld::GetWorld()->NetDriver;

	SetNetDormancyOG(Actor, NewDormancy);

	if (Driver)
		if (NewDormancy <= 1)
			for (auto& Conn : Driver->ClientConnections)
				((void(*)(UNetConnection*, AActor*)) FindFlushDormancy())(Conn, Actor);
}

void (*FlushNetDormancyOG)(AActor* Actor);
void FlushNetDormancy(AActor* Actor)
{
	auto Driver = (UNetDriver*)UWorld::GetWorld()->NetDriver;

	FlushNetDormancyOG(Actor);

	if (Driver)
		if (Actor->NetDormancy > 1)
			for (auto& Conn : Driver->ClientConnections)
				((void(*)(UNetConnection*, AActor*)) FindFlushDormancy())(Conn, Actor);
}

void UNetDriver::PostLoadHook()
{
	if (VersionInfo.EngineVersion == 4.16)
	{
		NetworkObjectListOffset = 0x3f8;
		ReplicationFrameOffset = 0x288;
	}
	else if (VersionInfo.EngineVersion == 4.19)
	{
		NetworkObjectListOffset = 0x490;
		ReplicationFrameOffset = 0x2c8;
	}
	else if (VersionInfo.FortniteVersion >= 2.5 && VersionInfo.FortniteVersion <= 3.1)
	{
		NetworkObjectListOffset = VersionInfo.FortniteVersion == 3.1 ? 0x4F8 : 0x4F0;
		ReplicationFrameOffset = 0x328;
	}
	else if (VersionInfo.FortniteVersion <= 3.3)
	{
		NetworkObjectListOffset = VersionInfo.FortniteVersion == 3.3 ? 0x508 : 0x500;
		ReplicationFrameOffset = 0x330;
	}
	else if (VersionInfo.FortniteVersion >= 20 && VersionInfo.FortniteVersion < 22)
	{
		NetworkObjectListOffset = 0x6b8;
		ReplicationFrameOffset = 0x3d8;
	}
	else if (std::floor(VersionInfo.FortniteVersion) == 22)
	{
		NetworkObjectListOffset = 0x708;
		ReplicationFrameOffset = 0x428;
	}
	else if (VersionInfo.FortniteVersion >= 23 && VersionInfo.FortniteVersion < 25)
	{
		ReplicationFrameOffset = VersionInfo.FortniteVersion == 24.20 ? 0x438 : 0x440;
		NetworkObjectListOffset = VersionInfo.FortniteVersion < 24 ? 0x720 : 0x730;
	}
	else if (VersionInfo.FortniteVersion >= 25 && VersionInfo.FortniteVersion < 27)
	{
		NetworkObjectListOffset = 0x750;
		ReplicationFrameOffset = 0x458;
	}
	else if (VersionInfo.FortniteVersion >= 27)
	{
		NetworkObjectListOffset = 0x760;
		ReplicationFrameOffset = 0x468;
	}

	if (VersionInfo.FortniteVersion <= 1.72 && VersionInfo.FortniteVersion != 1.1 && VersionInfo.FortniteVersion != 1.11)
		ClientWorldPackageNameOffset = 0x336A8;
	else if (VersionInfo.FortniteVersion == 1.8 || VersionInfo.FortniteVersion == 1.81 || VersionInfo.FortniteVersion == 1.82 || VersionInfo.FortniteVersion == 1.9)
		ClientWorldPackageNameOffset = 0x33788;
	else if (VersionInfo.FortniteVersion == 1.10)
		ClientWorldPackageNameOffset = 0x337A8;
	else if (VersionInfo.FortniteVersion == 1.11)
		ClientWorldPackageNameOffset = 0x337B8;
	else if (VersionInfo.FortniteVersion >= 2.2 && VersionInfo.FortniteVersion <= 2.4)
		ClientWorldPackageNameOffset = 0xA17A8;
	else if (VersionInfo.FortniteVersion == 2.42 || VersionInfo.FortniteVersion == 2.5)
		ClientWorldPackageNameOffset = 0x17F8;
	else if (VersionInfo.FortniteVersion == 3.1)
		ClientWorldPackageNameOffset = 0x1818;
	else if (VersionInfo.FortniteVersion == 3.2)
		ClientWorldPackageNameOffset = 0x1820;
	else if (VersionInfo.FortniteVersion == 3.3)
		ClientWorldPackageNameOffset = 0x1828;
	else if (VersionInfo.FortniteVersion < 24 && VersionInfo.FortniteVersion > 23.10) 
		ClientWorldPackageNameOffset = 0x17D0;
	else if (VersionInfo.FortniteVersion >= 23 && VersionInfo.FortniteVersion <= 23.10) 
		ClientWorldPackageNameOffset = 0x1780;
	else if (std::floor(VersionInfo.FortniteVersion) == 22)
		ClientWorldPackageNameOffset = 0x1730;
	else if (VersionInfo.FortniteVersion >= 27)
		ClientWorldPackageNameOffset = 0x1828;
	else if (VersionInfo.FortniteVersion >= 24)
		ClientWorldPackageNameOffset = 0x1820;
	else if (VersionInfo.FortniteVersion >= 20)
		ClientWorldPackageNameOffset = 0x16b8;

	if (VersionInfo.FortniteVersion >= 25)
	{
		DestroyedStartupOrDormantActorsOffset = VersionInfo.FortniteVersion >= 27 ? 0x328 : 0x318;
		DestroyedStartupOrDormantActorGUIDsOffset = VersionInfo.FortniteVersion >= 27 ? 0x14b8 : 0x14b0;
		ClientVisibleLevelNamesOffset = DestroyedStartupOrDormantActorGUIDsOffset + (VersionInfo.FortniteVersion < 24 ? 0x190 : 0x1e0);
	}
	else if (VersionInfo.FortniteVersion >= 23)
	{
		DestroyedStartupOrDormantActorsOffset = VersionInfo.FortniteVersion >= 24 ? 0x2f8 : 0x300;
		DestroyedStartupOrDormantActorGUIDsOffset = 0x14b0;
		ClientVisibleLevelNamesOffset = DestroyedStartupOrDormantActorGUIDsOffset + (VersionInfo.FortniteVersion < 24 ? 0x190 : 0x1e0);
	}
	else if (VersionInfo.FortniteVersion >= 20)
	{
		DestroyedStartupOrDormantActorsOffset = 0x2e8;
		DestroyedStartupOrDormantActorGUIDsOffset = VersionInfo.EngineVersion >= 5.1 ? 0x14b0 : 0x1488;
		ClientVisibleLevelNamesOffset = DestroyedStartupOrDormantActorGUIDsOffset + (VersionInfo.EngineVersion >= 5.1 ? 0xf0 : 0xa0);
	}

	if (!FindServerReplicateActors())
	{
		if (VersionInfo.EngineVersion >= 5.3 && FConfiguration::bEnableIris)
		{
			FindSendClientAdjustment();
			FindUpdateIrisReplicationViews();
			FindPreSendUpdate();
			Utils::Hook(FindTickFlush(), TickFlush__Iris, TickFlushOG);
			return;
		}
		// cache
		FindCreateChannel();
		FindSetChannelActor();
		FindReplicateActor();
		FindSendClientAdjustment();
		FindIsNetRelevantForVft();
		FindCallPreReplication();
		FindCloseActorChannel();
		FindStartBecomingDormant();
		FindClientHasInitializedLevelFor();
		FindSetChannelActorForDestroy();
		FindSendDestructionInfo();
		FindIsNetReady();

		if (VersionInfo.FortniteVersion < 3.4)
			FindFlushDormancy();
		else
		{
			FindGetNamePool();
			WorldPartitionHLODClass = FindClass("WorldPartitionHLOD");
		}

		GetActorLocation = (void(*)(AActor*, FFrame&, FVector*))AActor::GetDefaultObj()->GetFunction("K2_GetActorLocation")->GetNativeFunc();

		Utils::Hook(FindTickFlush(), TickFlush, TickFlushOG);
	}
	else
	{
		Utils::Hook(FindTickFlush(), TickFlush__RepGraph, TickFlushOG);
	}

	if (VersionInfo.FortniteVersion < 3.4 && FindFlushDormancy())
	{
		Utils::Hook(__int64(AActor::GetDefaultObj()->GetFunction("FlushNetDormancy")->GetImpl()), FlushNetDormancy, FlushNetDormancyOG);
		Utils::Hook(__int64(AActor::GetDefaultObj()->GetFunction("SetNetDormancy")->GetImpl()), SetNetDormancy, SetNetDormancyOG);
	}
}
