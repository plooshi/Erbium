#include "pch.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortGameModeAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/BuildingSMActor.h"
#include "../Public/FortKismetLibrary.h"


uint64_t FindGetPlayerViewPoint()
{
	uint64 ftspAddr = 0;
	auto ftspRef = Memcury::Scanner::FindStringRef(L"%s failed to spawn a pawn", true, 0, VersionInfo.FortniteVersion >= 19).Get();

	for (int i = 0; i < 1000; i++)
	{
		if (*(uint8_t*)(ftspRef - i) == 0x40 && *(uint8_t*)(ftspRef - i + 1) == 0x53)
		{
			ftspAddr = ftspRef - i;
			break;
		}
		else if (*(uint8_t*)(ftspRef - i) == 0x48 && *(uint8_t*)(ftspRef - i + 1) == 0x89 && *(uint8_t*)(ftspRef - i + 2) == 0x5C)
		{
			ftspAddr = ftspRef - i;
			break;
		}
	}

	if (!ftspAddr)
		return 0;

	auto PCVft = AFortPlayerControllerAthena::GetDefaultObj()->Vft;
	int ftspIdx = 0;

	for (int i = 0; i < 500; i++)
	{
		if (PCVft[i] == (void*)ftspAddr)
		{
			ftspIdx = i;
			break;
		}
	}

	if (ftspIdx == 0)
		return 0;

	return __int64(PCVft[ftspIdx - 1]);
}

void AFortPlayerControllerAthena::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Loc, FRotator& Rot)
{
	static auto SFName = UKismetStringLibrary::Conv_StringToName(FString(L"Spectating"));
	if (PlayerController->StateName == SFName)
	{
		Loc = PlayerController->LastSpectatorSyncLocation;
		Rot = PlayerController->LastSpectatorSyncRotation;
	}
	else if (PlayerController->GetViewTarget())
	{
		Loc = PlayerController->GetViewTarget()->K2_GetActorLocation();
		Rot = PlayerController->GetControlRotation();
	}
	else return GetPlayerViewPointOG(PlayerController, Loc, Rot);
}

void AFortPlayerControllerAthena::ServerAcknowledgePossession(UObject* Context, FFrame& Stack)
{
	AActor* Pawn;
	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	PlayerController->AcknowledgedPawn = Pawn;
}

void AFortPlayerControllerAthena::ServerAttemptAircraftJump(UObject* Context, FFrame& Stack)
{
	FRotator Rotation;
	Stack.StepCompiledIn(&Rotation);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = nullptr;
	auto GameMode = (AFortGameModeAthena*) UWorld::GetWorld()->AuthorityGameMode;

	static auto ControllerCompClass = FindClass("FortControllerComponent_Aircraft");
	if (Context->IsA(ControllerCompClass))
		PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
	else
		PlayerController = (AFortPlayerControllerAthena*)Context;

	GameMode->RestartPlayer(PlayerController);
	//PlayerController->ClientSetRotation(Rotation, true);

	if (PlayerController->MyFortPawn)
	{
		PlayerController->MyFortPawn->BeginSkydiving(true);
		PlayerController->MyFortPawn->SetHealth(100.f);
	}
}

void AFortPlayerControllerAthena::ServerExecuteInventoryItem(UObject* Context, FFrame& Stack)
{
	FGuid ItemGuid;
	Stack.StepCompiledIn(&ItemGuid);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemGuid == ItemGuid;
	}, FFortItemEntry::Size());

	if (!entry || !PlayerController->MyFortPawn) 
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*) entry->ItemDefinition;
	PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid, entry->HasTrackerGuid() ? entry->TrackerGuid : FGuid(), false);
}


void AFortPlayerControllerAthena::ServerExecuteInventoryWeapon(UObject* Context, FFrame& Stack)
{
	AFortWeapon* Weapon;
	Stack.StepCompiledIn(&Weapon);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemGuid == Weapon->ItemEntryGuid;
		}, FFortItemEntry::Size());

	if (!entry || !PlayerController->MyFortPawn)
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*)entry->ItemDefinition;
	PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, Weapon->ItemEntryGuid, entry->HasTrackerGuid() ? entry->TrackerGuid : FGuid(), false);
}

void AFortPlayerControllerAthena::ServerCreateBuildingActor(UObject* Context, FFrame& Stack)
{
	const UClass* BuildingClass = nullptr;
	FVector BuildLoc;
	FRotator BuildRot;
	bool bMirrored;
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	struct _Pad_0xC
	{
		uint8_t Padding[0xC];
	};
	struct _Pad_0x18
	{
		uint8_t Padding[0x18];
	};

	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	struct FBuildingClassData { UClass* BuildingClass; int PreviousBuildingLevel; int UpgradeLevel; };
	if (VersionInfo.FortniteVersion >= 8.30)
	{
		struct FCreateBuildingActorData { uint32_t BuildingClassHandle; _Pad_0xC BuildLoc; _Pad_0xC BuildRot; bool bMirrored; };
		struct FCreateBuildingActorData_New { uint32_t BuildingClassHandle; _Pad_0x18 BuildLoc; _Pad_0x18 BuildRot; bool bMirrored; };

		if (VersionInfo.FortniteVersion >= 20.00)
		{
			FCreateBuildingActorData_New CreateBuildingData;
			Stack.StepCompiledIn(&CreateBuildingData);

			BuildLoc = *(FVector*)&CreateBuildingData.BuildLoc;
			BuildRot = *(FRotator*)&CreateBuildingData.BuildRot;
			bMirrored = CreateBuildingData.bMirrored;

			auto BuildingClassPtr = GameState->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](TSubclassOf<AActor> Class, int32 Handle) {
				return Handle == CreateBuildingData.BuildingClassHandle;
				});
			if (!BuildingClassPtr)
			{
				Stack.IncrementCode();
				return;
			}

			BuildingClass = BuildingClassPtr->Get();
		}
		else
		{
			FCreateBuildingActorData CreateBuildingData;
			Stack.StepCompiledIn(&CreateBuildingData);

			BuildLoc = *(FVector*)&CreateBuildingData.BuildLoc;
			BuildRot = *(FRotator*)&CreateBuildingData.BuildRot;
			bMirrored = CreateBuildingData.bMirrored;

			auto BuildingClassPtr = GameState->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](TSubclassOf<AActor> Class, int32 Handle) {
				return Handle == CreateBuildingData.BuildingClassHandle;
				});
			if (!BuildingClassPtr)
			{
				Stack.IncrementCode();
				return;
			}

			BuildingClass = BuildingClassPtr->Get();
		}
	}
	else
	{

		FBuildingClassData BuildingClassData;
		Stack.StepCompiledIn(&BuildingClassData);
		Stack.StepCompiledIn(&BuildLoc);
		Stack.StepCompiledIn(&BuildRot);
		Stack.StepCompiledIn(&bMirrored);

		if (GameState->HasAllPlayerBuildableClasses() && !GameState->AllPlayerBuildableClasses.Contains(BuildingClassData.BuildingClass))
		{
			Stack.IncrementCode();
			return;
		}
		BuildingClass = BuildingClassData.BuildingClass;
	}
	Stack.IncrementCode();

	if (!BuildingClass)
		return;

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->GetDefaultObj())->ResourceType);

	FFortItemEntry* ItemEntry = nullptr;
	if (!PlayerController->bBuildFree)
	{
		ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemDefinition == Resource; }, FFortItemEntry::Size());

		if (!ItemEntry || ItemEntry->Count < 10)
			return;
	}

	TArray<ABuildingSMActor*> RemoveBuildings;
	char _Unk_OutVar1;
	static auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))FindCantBuild();
	static auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))FindCantBuild();
	if (VersionInfo.FortniteVersion >= 20.00 ? CantBuildNew(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildLoc, *(_Pad_0x18*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1) : CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0xC*)&BuildLoc, *(_Pad_0xC*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1))
		return;

	for (auto& RemoveBuilding : RemoveBuildings)
		RemoveBuilding->K2_DestroyActor();
	RemoveBuildings.Free();


	ABuildingSMActor* Building = UWorld::SpawnActor<ABuildingSMActor>(BuildingClass, BuildLoc, BuildRot, PlayerController);
	if (!Building)
		return;

	//Building->CurrentBuildingLevel = CreateBuildingData.BuildingClassData.UpgradeLevel;
	//Building->OnRep_CurrentBuildingLevel();

	Building->SetMirrored(bMirrored);

	Building->bPlayerPlaced = true;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr);


	if (!PlayerController->bBuildFree)
	{
		ItemEntry->Count -= 10;
		if (ItemEntry->Count <= 0)
			PlayerController->WorldInventory->Remove(ItemEntry->ItemGuid);
		else
			PlayerController->WorldInventory->UpdateEntry(*ItemEntry);
	}

	Building->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	Building->Team = Building->TeamIndex;
}

void AFortPlayerControllerAthena::Hook()
{
	auto DefaultFortPC = DefaultObjImpl("FortPlayerController");

	Utils::Hook(FindGetPlayerViewPoint(), GetPlayerViewPoint, GetPlayerViewPointOG);
	// they only stripped it on athena for some reason
	auto ServerAcknowledgePossessionIdx = GetDefaultObj()->GetFunction("ServerAcknowledgePossession")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerAcknowledgePossessionIdx, DefaultFortPC->Vft[ServerAcknowledgePossessionIdx]);

	auto ServerAttemptAircraftJumpPC = GetDefaultObj()->GetFunction("ServerAttemptAircraftJump");
	if (!ServerAttemptAircraftJumpPC)
		Utils::ExecHook(DefaultObjImpl("FortControllerComponent_Aircraft")->GetFunction("ServerAttemptAircraftJump"), ServerAttemptAircraftJump);
	else
		Utils::ExecHook(ServerAttemptAircraftJumpPC, ServerAttemptAircraftJump);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerAcknowledgePossession"), ServerAcknowledgePossession);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryItem"), ServerExecuteInventoryItem);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryWeapon"), ServerExecuteInventoryWeapon); // S9 shenanigans

	// same as serveracknowledgepossession
	auto ServerReturnToMainMenuIdx = GetDefaultObj()->GetFunction("ServerReturnToMainMenu")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerReturnToMainMenuIdx, DefaultFortPC->Vft[ServerReturnToMainMenuIdx]);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCreateBuildingActor"), ServerCreateBuildingActor);
}