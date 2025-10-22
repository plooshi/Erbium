#include "pch.h"
#include "../Public/NetDriver.h"
#include "../../Erbium/Public/Finders.h"
#include "../../FortniteGame/Public/FortGameModeAthena.h"
#include "../../Erbium/Public/GUI.h"

uint32_t NetworkObjectListOffset = 0;
uint32_t ReplicationFrameOffset = 0;
uint32_t ClientWorldPackageNameOffset = 0;
uint32_t DestroyedStartupOrDormantActorsOffset = 0;
uint32_t DestroyedStartupOrDormantActorGUIDsOffset = 0;
uint32_t ClientVisibleLevelNamesOffset = 0;

std::unordered_map<UNetConnection*, TArray<FNetViewer*>> ViewerMap;

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

bool IsActorRelevantToConnection(const AActor* Actor, const TArray<FNetViewer*>& ConnectionViewers)
{
	auto IsNetRelevantForIdx = FindIsNetRelevantForVft();
	if (IsNetRelevantForIdx == 0)
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
	static bool (*ClientHasInitializedLevelFor)(const UNetConnection*, const AActor*) = decltype(ClientHasInitializedLevelFor)(FindClientHasInitializedLevelFor());

	const bool bCorrectWorld = NetDriver->WorldPackage != nullptr && (!ClientWorldPackageNameOffset || *(FName*)(__int64(InConnection) + ClientWorldPackageNameOffset) == NetDriver->WorldPackage->Name) && (!ClientHasInitializedLevelFor || ClientHasInitializedLevelFor(InConnection, InActor));
	const bool bIsConnectionPC = (InActor == InConnection->PlayerController);
	return bCorrectWorld || bIsConnectionPC;
}

struct FPrioActor
{
	FNetworkObjectInfo* ActorInfo;
	UActorChannel* Channel;
};

std::unordered_map<UNetConnection*, TArray<FPrioActor>> PriorityLists;

void ServerReplicateActors(UNetDriver* Driver, float DeltaSeconds)
{
	if (!ReplicationFrameOffset)
		return;

	(*(int*)(__int64(Driver) + ReplicationFrameOffset))++;

	BuildViewerMap(Driver);
	if (ViewerMap.size() == 0)
		return;

	static FName ActorName = UKismetStringLibrary::Conv_StringToName(FString(L"Actor"));

	auto& NetworkObjectList = GetNetworkObjectList(Driver);
	auto& ActiveNetworkObjects = NetworkObjectList.ActiveNetworkObjects;

	for (auto& ViewerPair : ViewerMap)
	{
		auto& Conn = ViewerPair.first;
		auto& Viewers = ViewerPair.second;

		TArray<FPrioActor> List;
		List.Reserve(ActiveNetworkObjects.Num());

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

		PriorityLists[Conn] = List;
	}

	for (auto& ActorInfo : ActiveNetworkObjects)
	{
		//if (!ActorInfo->bPendingNetUpdate && Time <= ActorInfo->NextUpdateTime)
		//	continue;

		auto Actor = ActorInfo->Actor;

		if (!Actor || /*!Actor->bActorInitialized || */Actor->NetDriverName != Driver->NetDriverName)
			continue;

		auto Outer = Actor->Outer;
		if ((VersionInfo.FortniteVersion >= 23.00 ? false : Actor->bActorIsBeingDestroyed) || Actor->RemoteRole == 0 || ((Actor->HasbNetStartup() ? Actor->bNetStartup : false) && Actor->NetDormancy == 4))
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
				if (!Chan->IsA<UActorChannel>() || Chan->Actor != Actor)	
					continue;

				Channel = Chan;
				break;
			}

			bool bRelevant = IsActorRelevantToConnection(Actor, Viewers);
			bool bLevelInitializedForActor = IsLevelInitializedForActor(Driver, Actor, Conn);
			if (!Channel && (!bRelevant || !bLevelInitializedForActor))
				continue;

			static auto CloseActorChannel = (void(*)(UActorChannel*, uint8_t)) FindCloseActorChannel();
			if (Channel && !bRelevant && (!bLevelInitializedForActor || !(Actor->HasbNetStartup() ? Actor->bNetStartup : true)))
			{
				CloseActorChannel(Channel, 3);
				continue;
			}


			auto PriorityConn = Conn;

			if (Actor->bOnlyRelevantToOwner)
			{
				bool bHasNullViewTarget = false;

				if (!(PriorityConn = IsActorOwnedByAndRelevantToConnection(Actor, Viewers, bHasNullViewTarget)))
				{
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

				if (VersionInfo.FortniteVersion != 1.72 && VersionInfo.FortniteVersion != 0.00)
					if (Actor->GetNetDormancy() > 1 && Channel && !Channel->IsPendingDormancy() && !Channel->IsDormant())
						((int32(*)(UActorChannel*))FindStartBecomingDormant())(Channel);
			}

			if (bRelevant && bLevelInitializedForActor)
			{
				bAnyRelevant = true;
				auto& PriorityList = PriorityLists[Conn];
				PriorityList.Add({ ActorInfo.Get(), Channel });
			}
		}

		if (bAnyRelevant)
			((void(*)(AActor*, UNetDriver*)) FindCallPreReplication())(Actor, Driver);
	}


	for (auto& PriorityListPair : PriorityLists)
	{
		auto Conn = PriorityListPair.first;

		if (!Conn->ViewTarget)
			continue;
		auto& PriorityActors = PriorityListPair.second;

		auto& Viewers = ViewerMap[Conn];

		static auto SendClientAdjustment = FindSendClientAdjustment();
		if (SendClientAdjustment)
			for (auto& Viewer : Viewers)
			{
				if (Viewer->Connection->PlayerController)
					((void(*)(AFortPlayerControllerAthena*)) SendClientAdjustment)(Viewer->Connection->PlayerController);
			}

		int i = 0;
		for (auto& PriorityActor : PriorityActors)
		{
			i++;
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
					((int32(*)(UActorChannel*))FindReplicateActor())(Channel);
				}
			}
		}

		PriorityActors.Free();
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
    static auto HasReplicationDriver_ = Driver->HasReplicationDriver();

	static auto ServerReplicateActors_ = FindServerReplicateActors();
    if (ServerReplicateActors_ && (HasReplicationDriver_ ? Driver->ReplicationDriver : nullptr))
    {
        ((void (*)(UObject*, float)) ServerReplicateActors_)(Driver->ReplicationDriver, DeltaSeconds);
    }
	else if (!HasReplicationDriver_ || !ServerReplicateActors_)
		ServerReplicateActors(Driver, DeltaSeconds);

    if (GUI::gsStatus == 1 && VersionInfo.FortniteVersion >= 11.00)
    { 
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		static auto bSkipAircraft = GameState->CurrentPlaylistInfo.BasePlaylist->bSkipAircraft;
        if (!bSkipAircraft && Driver->ClientConnections.Num() > 0 && GameMode->bWorldIsReady && GameState->WarmupCountdownEndTime <= Time)
        {
			GUI::gsStatus = 2;

            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
        }
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

void UNetDriver::Hook()
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
	if (VersionInfo.FortniteVersion >= 23 && VersionInfo.FortniteVersion < 25)
	{
		ReplicationFrameOffset = VersionInfo.FortniteVersion == 24.20 ? 0x438 : 0x440;
		NetworkObjectListOffset = VersionInfo.FortniteVersion < 24 ? 0x720 : 0x730;
	}

	if (VersionInfo.FortniteVersion == 1.72)
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
	else if (VersionInfo.FortniteVersion < 24 && VersionInfo.FortniteVersion >= 22) 
		ClientWorldPackageNameOffset = 0x17D0;
	else if (VersionInfo.FortniteVersion >= 20 && VersionInfo.FortniteVersion < 25)
		ClientWorldPackageNameOffset = 0x16b8;

	if (VersionInfo.FortniteVersion >= 23)
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

		if (VersionInfo.FortniteVersion < 3.4)
			FindFlushDormancy();
	}

    Utils::Hook(FindTickFlush(), TickFlush, TickFlushOG); 

	if (VersionInfo.FortniteVersion < 3.4 && FindFlushDormancy())
	{
		Utils::Hook(__int64(AActor::GetDefaultObj()->GetFunction("FlushNetDormancy")->GetImpl()), FlushNetDormancy, FlushNetDormancyOG);
		Utils::Hook(__int64(AActor::GetDefaultObj()->GetFunction("SetNetDormancy")->GetImpl()), SetNetDormancy, SetNetDormancyOG);
	}
}
