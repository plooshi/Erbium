#include "pch.h"
#include "../Public/BuildingSMActor.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/FortGameMode.h"
#include "../Public/FortWeapon.h"
#include "../Public/FortKismetLibrary.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../../Erbium/Public/Configuration.h"


void ABuildingSMActor::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, __int64 HitInfo, AActor* InstigatedBy, AActor* DamageCauser, __int64 EffectContext)
{
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	auto GameMode = ((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode);
	auto Controller = (AFortPlayerControllerAthena*)InstigatedBy;

	/*auto bIsWeakspot = Damage == 100.f && Actor->IsA<ABuildingSMActor>() && DamageCauser->IsA<AFortWeapon>() && ((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass());

	if (bIsWeakspot)
	{
		FGameplayTagContainer TargetTags{};

		auto Interface = (IGameplayTagAssetInterface*)Actor->GetInterface(IGameplayTagAssetInterface::StaticClass());
		if (Interface)
		{
			auto GetOwnedGameplayTags = (void(*)(IGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->Vft[0x2];
			GetOwnedGameplayTags(Interface, &TargetTags);
			//Interface->GetOwnedGameplayTags(&TargetTags);
		}

		// add Quest.MetaData.HitWeakSpot to source tags

		if (Controller)
			Controller->GetQuestManager(1)->SendStatEvent(Controller, EFortQuestObjectiveStatEvent::GetComplexCustom(), 1, Actor, TargetTags);
	}*/

	if (!InstigatedBy || !Actor->IsA<ABuildingSMActor>() || Actor->bPlayerPlaced || Actor->GetHealth() == 1 || (Actor->HasbAllowResourceDrop() && !Actor->bAllowResourceDrop))
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	if (!DamageCauser || !DamageCauser->IsA<AFortWeapon>() || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
	if (!Resource)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	auto MaxMat = Resource->GetMaxStackSize();

	
    static auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;
	static auto GameData = Playlist ? Playlist->ResourceRates.Get() : nullptr;
	if (!GameData)
		GameData = FindObject<UCurveTable>(GameMode->HasWarmupRequiredPlayerCount() ? L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates" : L"/Game/Balance/DataTables/ResourceRates.ResourceRates");

	int ResCount = 0;
	if (Actor->HasBuildingResourceAmountOverride())
	{
		FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out = 0.f;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

			float RC = Out / (Actor->GetMaxHealth() / Damage);

			ResCount = (int)round(RC);
		}
	}
	else
	{
		auto ClassData = Actor->GetClassData();
		FCurveTableRowHandle& BuildingResourceAmountOverride = ClassData->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out = 0.f;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

			float RC = Out / (Actor->GetMaxHealth() / Damage);

			ResCount = (int)round(RC);
		}
	}

	if (ResCount > 0)
	{
		auto ItemP = Controller->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
			{
				return entry->ItemEntry.ItemDefinition == Resource;
			});
		auto itemEntry = Controller->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemDefinition == Resource;
			}, FFortItemEntry::Size());

		if (ItemP)
		{
			auto Item = *ItemP;

			/*for (int i = 0; i < Item->ItemEntry.StateValues.Num(); i++)
			{
				auto& StateValue = Item->ItemEntry.StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
			}*/


			itemEntry->Count += ResCount;
			if (itemEntry->Count > MaxMat)
			{
				AFortInventory::SpawnPickup(Controller->Pawn->K2_GetActorLocation(), Item->ItemEntry.ItemDefinition, itemEntry->Count - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), Controller->MyFortPawn);
				itemEntry->Count = MaxMat;
			}

			for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
				break;
			}


			Item->ItemEntry.Count = itemEntry->Count;
			Controller->WorldInventory->UpdateEntry(*itemEntry);
			Item->ItemEntry.bIsDirty = true;
		}
		else
		{
			if (ResCount > MaxMat)
			{
				AFortInventory::SpawnPickup(Controller->Pawn->K2_GetActorLocation(), Resource, ResCount - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), Controller->MyFortPawn);
				ResCount = MaxMat;
			}

			Controller->WorldInventory->GiveItem(Resource, ResCount, 0, 0, false);
		}
	}


	if (ResCount > 0)
		Controller->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType(EFortResourceType__Enum::GetNone()) : Actor->ResourceType, ResCount, Actor->GetHealth() - Damage <= 0, Damage == 100.f);

	Actor->ForceNetUpdate();
	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

uint32 SpawnDecoVft = 0;
uint32 ShouldAllowServerSpawnDecoVft = 0;
void AFortDecoTool::ServerSpawnDeco_(UObject* Context, FFrame& Stack)
{
	FVector Location;
	FRotator Rotation;
	ABuildingSMActor* AttachedActor;
	uint8_t InBuildingAttachmentType;
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&AttachedActor);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.IncrementCode();
	auto DecoTool = (AFortDecoTool*)Context;

	auto Pawn = (AFortPlayerPawnAthena*)DecoTool->Owner;
	if (!Pawn)
		return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return;

	if (VersionInfo.FortniteVersion >= 18) // idk when they stripped it, guessing s18
	{
		auto ItemDefinition = (UFortDecoItemDefinition*)DecoTool->ItemDefinition;

		if (auto ContextTrapTool = DecoTool->Cast<AFortDecoTool_ContextTrap>()) {
			switch ((int)InBuildingAttachmentType) {
			case 0:
			case 6:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->FloorTrap;
				break;
			case 7:
			case 2:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->CeilingTrap;
				break;
			case 1:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->WallTrap;
				break;
			case 8:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->StairTrap;
				break;
			}
		}

		auto ShouldAllowServerSpawnDeco = (bool (*)(AFortDecoTool*, FVector&, FRotator&, ABuildingSMActor*, uint8_t)) DecoTool->Vft[ShouldAllowServerSpawnDecoVft];

		if (ShouldAllowServerSpawnDecoVft && !ShouldAllowServerSpawnDeco(DecoTool, Location, Rotation, AttachedActor, InBuildingAttachmentType))
			return;

		ABuildingSMActor* NewTrap = nullptr;
		if (VersionInfo.FortniteVersion >= 27)
		{
			auto SpawnDeco = (ABuildingSMActor * (*)(AFortDecoTool*, TSubclassOf<ABuildingSMActor>&, FVector&, FRotator&, ABuildingSMActor*, int, uint8_t)) DecoTool->Vft[SpawnDecoVft];

			TSubclassOf<ABuildingSMActor> SubclassOf;
			SubclassOf.ClassPtr = ItemDefinition->BlueprintClass.Get();
			NewTrap = SpawnDecoVft ? SpawnDeco(DecoTool, SubclassOf, Location, Rotation, AttachedActor, 0, InBuildingAttachmentType) : nullptr;
		}
		else
		{
			auto SpawnDeco = (ABuildingSMActor * (*)(AFortDecoTool*, UClass*, FVector&, FRotator&, ABuildingSMActor*, int, uint8_t)) DecoTool->Vft[SpawnDecoVft];

			NewTrap = SpawnDecoVft ? SpawnDeco(DecoTool, ItemDefinition->BlueprintClass.Get(), Location, Rotation, AttachedActor, 0, InBuildingAttachmentType) : nullptr;
		}

		auto Resource = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(AttachedActor->ResourceType);
		auto item = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* Item) {
			return Item->ItemEntry.ItemDefinition == DecoTool->ItemDefinition;
			});
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
			return entry.ItemDefinition == DecoTool->ItemDefinition;
			}, FFortItemEntry::Size());
		if (!itemEntry)
			return;

		itemEntry->Count--;
		if (itemEntry->Count <= 0)
			PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
		else
		{
			(*item)->ItemEntry.Count = itemEntry->Count;
			PlayerController->WorldInventory->UpdateEntry(*itemEntry);
			(*item)->ItemEntry.bIsDirty = true;
		}

		/*if (NewTrap && NewTrap->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex)
		{
			NewTrap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			NewTrap->Team = NewTrap->TeamIndex;
		}*/
	}

	if (VersionInfo.FortniteVersion < 18)
	{
		callOG(DecoTool, Stack.GetCurrentNativeFunction(), ServerSpawnDeco, Location, Rotation, AttachedActor, InBuildingAttachmentType);

		/*static auto TrapClass = FindClass("BuildingTrap");
		auto trapPtr = AttachedActor->AttachedBuildingActors.Search([&](ABuildingSMActor*& actor) {
			return actor->IsA(TrapClass) && actor->Team != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			});

		auto trap = trapPtr ? *trapPtr : nullptr;
		*if (trap) {
			trap->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;

			if (trap->HasTeamIndex())
				trap->TeamIndex = trap->Team;
		}*/
	}
}

void AFortDecoTool_ContextTrap::ServerSpawnDeco_Implementation(UObject* Context, FFrame& Stack)
{
	auto& Location = *(FVector*)Stack.Locals;
	auto& Rotation = *(FRotator*)(__int64(Stack.Locals) + FVector::Size());
	auto& AttachedActor = *(ABuildingSMActor**)(__int64(Stack.Locals) + FVector::Size() + FRotator::Size());
	auto& InBuildingAttachmentType = *(uint8_t*)(__int64(Stack.Locals) + FVector::Size() + FRotator::Size() + 8);
	auto DecoTool = (AFortDecoTool_ContextTrap*)Context;

	auto Pawn = (AFortPlayerPawnAthena*)DecoTool->Owner;
	if (!Pawn)
		return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return;

	if (VersionInfo.FortniteVersion >= 18) // idk when they stripped it, guessing s18
	{
		auto ItemDefinition = (UFortDecoItemDefinition*)DecoTool->ItemDefinition;

		if (auto ContextTrapTool = DecoTool->Cast<AFortDecoTool_ContextTrap>()) {
			switch ((int)InBuildingAttachmentType) {
			case 0:
			case 6:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->FloorTrap;
				break;
			case 7:
			case 2:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->CeilingTrap;
				break;
			case 1:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->WallTrap;
				break;
			case 8:
				ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->StairTrap;
				break;
			}
		}

		auto ShouldAllowServerSpawnDeco = (bool (*)(AFortDecoTool*, FVector&, FRotator&, ABuildingSMActor*, uint8_t)) DecoTool->Vft[ShouldAllowServerSpawnDecoVft];

		if (ShouldAllowServerSpawnDecoVft && !ShouldAllowServerSpawnDeco(DecoTool, Location, Rotation, AttachedActor, InBuildingAttachmentType))
			return;

		ABuildingSMActor* NewTrap = nullptr;
		if (VersionInfo.FortniteVersion >= 27)
		{
			auto SpawnDeco = (ABuildingSMActor * (*)(AFortDecoTool*, TSubclassOf<ABuildingSMActor>&, FVector&, FRotator&, ABuildingSMActor*, int, uint8_t)) DecoTool->Vft[SpawnDecoVft];

			TSubclassOf<ABuildingSMActor> SubclassOf;
			SubclassOf.ClassPtr = ItemDefinition->BlueprintClass.Get();
			NewTrap = SpawnDecoVft ? SpawnDeco(DecoTool, SubclassOf, Location, Rotation, AttachedActor, 0, InBuildingAttachmentType) : nullptr;
		}
		else
		{
			auto SpawnDeco = (ABuildingSMActor * (*)(AFortDecoTool*, UClass*, FVector&, FRotator&, ABuildingSMActor*, int, uint8_t)) DecoTool->Vft[SpawnDecoVft];

			NewTrap = SpawnDecoVft ? SpawnDeco(DecoTool, ItemDefinition->BlueprintClass.Get(), Location, Rotation, AttachedActor, 0, InBuildingAttachmentType) : nullptr;
		}

		auto Resource = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(AttachedActor->ResourceType);
		auto item = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* Item) {
			return Item->ItemEntry.ItemDefinition == DecoTool->ItemDefinition;
			});
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
			return entry.ItemDefinition == DecoTool->ItemDefinition;
			}, FFortItemEntry::Size());
		if (!itemEntry)
			return;

		itemEntry->Count--;
		if (itemEntry->Count <= 0)
			PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
		else
		{
			(*item)->ItemEntry.Count = itemEntry->Count;
			PlayerController->WorldInventory->UpdateEntry(*itemEntry);
			(*item)->ItemEntry.bIsDirty = true;
		}

		/*if (NewTrap && NewTrap->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex)
		{
			NewTrap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			NewTrap->Team = NewTrap->TeamIndex;
		}*/
	}

	if (VersionInfo.FortniteVersion < 18)
	{
		ServerSpawnDeco_ImplementationOG(Context, Stack);
		
		/*static auto TrapClass = FindClass("BuildingTrap");
		auto trapPtr = AttachedActor->AttachedBuildingActors.Search([&](ABuildingSMActor*& actor) {
			return actor->IsA(TrapClass) && actor->Team != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			});

		auto trap = trapPtr ? *trapPtr : nullptr;
		if (trap) {
			trap->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;

			if (trap->HasTeamIndex())
				trap->TeamIndex = trap->Team;
		}*/
	}
}


uint8 GetBuildingTypeFromBuildingAttachmentType(uint8 BuildingAttachmentType)
{
	if (uint8(BuildingAttachmentType) <= 7)
	{
		LONG Val = 0xC5;
		if (BitTest(&Val, uint8(BuildingAttachmentType)))
			return 1;
	}
	if (BuildingAttachmentType == 1)
		return 0;
	return 12;
}

extern uint64_t PayBuildableClassPlacementCost_;
extern uint64_t CanAffordToPlaceBuildableClass_;
extern uint64_t CantBuild_;
void AFortDecoTool::ServerCreateBuildingAndSpawnDeco(UObject* Context, FFrame& Stack)
{
	FVector BuildingLocation;
	FRotator BuildingRotation;
	FVector Location;
	FRotator Rotation;
	uint8_t InBuildingAttachmentType;
	bool bSpawnDecoOnExtraPiece;
	FVector BuildingExtraPieceLocation;
	Stack.StepCompiledIn(&BuildingLocation);
	Stack.StepCompiledIn(&BuildingRotation);
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.StepCompiledIn(&bSpawnDecoOnExtraPiece);
	Stack.StepCompiledIn(&BuildingExtraPieceLocation);
	Stack.IncrementCode();
	auto Tool = (AFortDecoTool*)Context;

	auto Pawn = (AFortPlayerPawnAthena*)Tool->Owner;
	if (!Pawn)
		return;

	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return;

	auto ItemDefinition = (UFortDecoItemDefinition*)Tool->ItemDefinition;


	if (auto ContextTrapTool = Tool->Cast<AFortDecoTool_ContextTrap>()) {
		switch ((int)InBuildingAttachmentType) {
		case 0:
		case 6:
			ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->FloorTrap;
			break;
		case 7:
		case 2:
			ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->CeilingTrap;
			break;
		case 1:
			ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->WallTrap;
			break;
		case 8:
			ItemDefinition = (UFortDecoItemDefinition*)ContextTrapTool->ContextTrapItemDefinition->StairTrap;
			break;
		}
	}

	TArray<const UObject*> AutoCreateAttachmentBuildingShapes;
	for (auto& AutoCreateAttachmentBuildingShape : ItemDefinition->AutoCreateAttachmentBuildingShapes)
	{
		AutoCreateAttachmentBuildingShapes.Add(AutoCreateAttachmentBuildingShape);
	}

	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto bIgnoreCanAffordCheck = UFortKismetLibrary::DoesItemDefinitionHaveGameplayTag(ItemDefinition, FGameplayTag(FName(L"Trap.ExtraPiece.Cost.Ignore")));
	TSubclassOf<AActor> BuildingClass{};
	auto ResourceType = PlayerController->CurrentResourceType;

	if (ItemDefinition->HasAutoCreateAttachmentBuildingResourceType() && ItemDefinition->AutoCreateAttachmentBuildingResourceType != EFortResourceType(EFortResourceType__Enum::GetNone()))
		ResourceType = ItemDefinition->AutoCreateAttachmentBuildingResourceType;
	auto BuildingType = GetBuildingTypeFromBuildingAttachmentType(InBuildingAttachmentType);

	for (auto Shape : AutoCreateAttachmentBuildingShapes)
	{
		for (auto& Class : GameState->AllPlayerBuildableClasses)
		{
			auto Default = (ABuildingSMActor*)Class->GetDefaultObj();

			UObject* EditModePatternData = nullptr;

			if (Default->HasEditModePatternData())
				EditModePatternData = Default->EditModePatternData;
			else
			{
				auto ClassData = Default->GetClassData();

				EditModePatternData = ClassData->EditModePatternData;
			}

			if (Default->ResourceType == ResourceType && Default->BuildingType == BuildingType && EditModePatternData == Shape)
			{
				BuildingClass = Class;
				goto _out;
			}
		}
	}
_out:

	FBuildingClassData BuildingClassData;
	BuildingClassData.BuildingClass = BuildingClass;
	BuildingClassData.PreviousBuildingLevel = -1;

	static auto UpgradeLevelOffset = FBuildingClassData::StaticStruct()->GetOffset("UpgradeLevel");
	if (VersionInfo.EngineVersion >= 5.3)
		*(uint8*)(__int64(&BuildingClassData) + UpgradeLevelOffset) = 0;
	else
		*(uint32*)(__int64(&BuildingClassData) + UpgradeLevelOffset) = 0;

	UFortWorldItem* Item = nullptr;
	if (!FConfiguration::bInfiniteMats)
	{
		auto CanAffordToPlaceBuildableClass = (bool(*)(AFortPlayerControllerAthena*, FBuildingClassData)) CanAffordToPlaceBuildableClass_;

		if (CanAffordToPlaceBuildableClass)
		{
			if (!CanAffordToPlaceBuildableClass(PlayerController, BuildingClassData))
				return;
		}
		else if (!PlayerController->bBuildFree)
		{
			auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->GetDefaultObj())->ResourceType);

			auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
				{ return entry->ItemEntry.ItemDefinition == Resource; });

			if (!ItemP)
				return;

			Item = *ItemP;

			if (Item->ItemEntry.Count < 10)
				return;
		}
	}

	struct _Pad_0xC
	{
		uint8_t Padding[0xC];
	};
	struct _Pad_0x18
	{
		uint8_t Padding[0x18];
	};

	TArray<ABuildingSMActor*> RemoveBuildings;
	if (VersionInfo.FortniteVersion >= 27)
	{
		char _Unk_OutVar1;
		auto CantBuild = (__int64 (*)(UWorld*, TSubclassOf<AActor>&, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;

		if (CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildingLocation, *(_Pad_0x18*)&BuildingRotation, false, &RemoveBuildings, &_Unk_OutVar1))
			return;
	}
	else
	{
		char _Unk_OutVar1;
		auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
		auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;

		if (VersionInfo.FortniteVersion >= 20.00 ? CantBuildNew(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildingLocation, *(_Pad_0x18*)&BuildingRotation, false, &RemoveBuildings, &_Unk_OutVar1) : CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0xC*)&BuildingLocation, *(_Pad_0xC*)&BuildingRotation, false, &RemoveBuildings, &_Unk_OutVar1))
			return;
	}

	for (auto& RemoveBuilding : RemoveBuildings)
		RemoveBuilding->K2_DestroyActor();
	//RemoveBuildings.Free();

	static auto K2_SpawnBuildingActor = ABuildingSMActor::GetDefaultObj()->GetFunction("K2_SpawnBuildingActor");

	ABuildingSMActor* Building = nullptr;
	if (K2_SpawnBuildingActor)
	{
		FTransform SpawnTransform(BuildingLocation, BuildingRotation);
		Building = ABuildingSMActor::K2_SpawnBuildingActor(PlayerController, BuildingClass, SpawnTransform, PlayerController, nullptr, false, false);
	}
	else
		Building = UWorld::SpawnActor<ABuildingSMActor>(BuildingClass, BuildingLocation, BuildingRotation, PlayerController);

	Building->bPlayerPlaced = true;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr, false);
	//UWorld::FinishSpawnActor(Building, BuildLoc, BuildRot);

	if (!PlayerController->bBuildFree && !FConfiguration::bInfiniteMats)
	{
		auto PayBuildableClassPlacementCost = (int(*)(AFortPlayerControllerAthena*, FBuildingClassData)) PayBuildableClassPlacementCost_;

		PayBuildableClassPlacementCost(PlayerController, BuildingClassData);
	}

	/*Building->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	if (Building->HasTeamIndex())
		Building->TeamIndex = Building->Team;*/
	Tool->ServerSpawnDeco(Location, Rotation, Building, InBuildingAttachmentType);
}

void ABuildingSMActor::PostLoadHook()
{
	if (!GetDefaultObj()->HasBuildingResourceAmountOverride())
	{
		GetSparseClassData_ = Memcury::Scanner::FindPattern("48 83 EC ? 48 8B 81 ? ? ? ? 45 33 C0 4C 8B C9").Get();

		if (!GetSparseClassData_)
			GetSparseClassData_ = Memcury::Scanner::FindPattern("48 83 EC ? 48 8B 81 ? ? ? ? 48 85 C0 74 ? 48 83 C4 ? C3").Get();

		if (!GetSparseClassData_)
			GetSparseClassData_ = Memcury::Scanner::FindPattern("48 83 EC ? 48 8B 81 ? ? ? ? 45 33 C0 48 85 C0 75").Get();
	}
	if (VersionInfo.FortniteVersion >= 18)
	{
		SpawnDecoVft = FindSpawnDecoVft();
		ShouldAllowServerSpawnDecoVft = FindShouldAllowServerSpawnDecoVft();
	}

	auto OnDamageServerAddr = FindFunctionCall(L"OnDamageServer", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x4C, 0x89, 0x4C } : VersionInfo.EngineVersion == 4.19 || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x40, 0x55 });

	Utils::Hook(OnDamageServerAddr, OnDamageServer, OnDamageServerOG);

	if (VersionInfo.FortniteVersion >= 18)
		Utils::ExecHook(AFortDecoTool::GetDefaultObj()->GetFunction("ServerSpawnDeco"), AFortDecoTool::ServerSpawnDeco_, AFortDecoTool::ServerSpawnDeco_OG);
	Utils::ExecHook(AFortDecoTool::GetDefaultObj()->GetFunction("ServerCreateBuildingAndSpawnDeco"), AFortDecoTool::ServerCreateBuildingAndSpawnDeco, AFortDecoTool::ServerCreateBuildingAndSpawnDecoOG);
	if (AFortDecoTool_ContextTrap::StaticClass())
	{
		auto Func = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerSpawnDeco");

		if (!Func)
			Func = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerSpawnDeco_Implementation");

		if (VersionInfo.FortniteVersion >= 18)
			Utils::ExecHook(Func, AFortDecoTool_ContextTrap::ServerSpawnDeco_Implementation, AFortDecoTool_ContextTrap::ServerSpawnDeco_ImplementationOG);

		auto Func2 = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerCreateBuildingAndSpawnDeco_Implementation");

		if (!Func2)
			Func2 = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerCreateBuildingAndSpawnDeco");

		Utils::ExecHook(Func2, AFortDecoTool::ServerCreateBuildingAndSpawnDeco, AFortDecoTool::ServerCreateBuildingAndSpawnDecoOG);
	}
}
