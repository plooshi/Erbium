#include "pch.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortGameModeAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/BuildingSMActor.h"
#include "../Public/FortKismetLibrary.h"
#include "../../Erbium/Public/Configuration.h"
#include "../Public/FortLootPackage.h"
#include "../../Erbium/Public/Events.h"
#include "../../Erbium/Public/LateGame.h"
#include "../Public/BuildingItemCollectorActor.h"

void AFortPlayerControllerAthena::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Loc, FRotator& Rot)
{
	static auto SFName = UKismetStringLibrary::Conv_StringToName(FString(L"Spectating"));
	if (PlayerController->StateName == SFName)
	{
		Loc = PlayerController->LastSpectatorSyncLocation;
		Rot = PlayerController->LastSpectatorSyncRotation;
	}
	else
	{
		auto ViewTarget = PlayerController->GetViewTarget();

		if (ViewTarget)
		{
			Loc = ViewTarget->K2_GetActorLocation();
			if (auto TargetPawn = ViewTarget->Cast<AFortPlayerPawnAthena>())
				Loc.Z += TargetPawn->BaseEyeHeight;
			Rot = PlayerController->GetControlRotation();
		}
		else
			PlayerController->GetActorEyesViewPoint(&Loc, &Rot);
	}
}

void AFortPlayerControllerAthena::ServerAcknowledgePossession(UObject* Context, FFrame& Stack)
{
	AActor* Pawn;
	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	static auto FortPCServerAcknowledgePossession = (void(*)(AFortPlayerControllerAthena*, AActor*))DefaultObjImpl("FortPlayerController")->Vft[Stack.GetCurrentNativeFunction()->GetVTableIndex()];
	FortPCServerAcknowledgePossession(PlayerController, Pawn);

	auto Num = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num();

	if (Num == 0)
	{
		static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");
		static bool HasCosmeticLoadoutPC = PlayerController->HasCosmeticLoadoutPC();
		static bool HasCustomizationLoadout = PlayerController->HasCustomizationLoadout();

		if (HasCosmeticLoadoutPC && PlayerController->CosmeticLoadoutPC.Pickaxe)
			PlayerController->WorldInventory->GiveItem(PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
		else if (HasCustomizationLoadout && PlayerController->CustomizationLoadout.Pickaxe)
			PlayerController->WorldInventory->GiveItem(PlayerController->CustomizationLoadout.Pickaxe->WeaponDefinition);
		else
		{
			static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

			PlayerController->WorldInventory->GiveItem(DefaultPickaxe);
		}

		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		for (int i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

			if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
				PlayerController->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
		}


		for (auto& AbilitySet : AFortGameModeAthena::AbilitySets)
			PlayerController->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);
	}
}

void AFortPlayerControllerAthena::ServerAttemptAircraftJump_(UObject* Context, FFrame& Stack)
{
	FRotator Rotation;
	Stack.StepCompiledIn(&Rotation);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = nullptr;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	if (VersionInfo.FortniteVersion >= 11.00 || FConfiguration::bLateGame)
	{
		static auto bIsComp = Context->IsA(FindClass("FortControllerComponent_Aircraft"));
		if (bIsComp)
			PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
		else
			PlayerController = (AFortPlayerControllerAthena*)Context;

		GameMode->RestartPlayer(PlayerController);
		PlayerController->SetControlRotation(Rotation);

		if (PlayerController->MyFortPawn)
		{
			PlayerController->MyFortPawn->BeginSkydiving(true);
			PlayerController->MyFortPawn->SetHealth(100.f);
			
			static auto Effect = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");

			bool Found = false;
			auto AbilitySystemComponent = PlayerController->PlayerState->AbilitySystemComponent;

			for (int i = 0; i < AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Num(); i++)
			{
				auto& ActiveEffect = AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Get(i, FActiveGameplayEffect::Size());

				if (ActiveEffect.Spec.Def)
					if (ActiveEffect.Spec.Def->IsA(Effect))
					{
						Found = true;
						break;
					}
			}

			if (!Found)
			{
				auto EffectHandle = FGameplayEffectContextHandle();
				auto SpecHandle = AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Effect, 0, EffectHandle);

				AbilitySystemComponent->SetActiveGameplayEffectLevel(SpecHandle, 0);

				AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(SpecHandle,
					FGameplayTag(UKismetStringLibrary::Conv_StringToName(FString(L"SetByCaller.StormCampingDamage"))), 1);
			}

			PlayerController->MyFortPawn->bIsInAnyStorm = false;
			PlayerController->MyFortPawn->OnRep_IsInAnyStorm();
			PlayerController->MyFortPawn->bIsInsideSafeZone = true;
			PlayerController->MyFortPawn->OnRep_IsInsideSafeZone();

			if (FConfiguration::bLateGame)
			{
				PlayerController->MyFortPawn->SetShield(100.f);

				FVector AircraftLocation = GameState->Aircrafts[0]->K2_GetActorLocation();

				float Angle = (float)rand() / 5215.03002625f;
				float Radius = (float)(rand() % 1000);

				float OffsetX = cosf(Angle) * Radius;
				float OffsetY = sinf(Angle) * Radius;

				FVector Offset;
				Offset.X = OffsetX;
				Offset.Y = OffsetY;
				Offset.Z = 0.0f;

				FVector NewLoc = AircraftLocation + Offset;

				PlayerController->MyFortPawn->K2_SetActorLocation(NewLoc, false, nullptr, true);
			}
		}
	}
	else
	{
		static auto ServerAttemptAircraftJumpOG = (void(*)(AFortPlayerControllerAthena*, FRotator&)) ((AFortPlayerControllerAthena*)Context)->Vft[((AFortPlayerControllerAthena*)Context)->GetFunction("ServerAttemptAircraftJump")->GetVTableIndex()];

		ServerAttemptAircraftJumpOG((AFortPlayerControllerAthena*)Context, Rotation);
	}
}

void AFortPlayerControllerAthena::ServerExecuteInventoryItem_(UObject* Context, FFrame& Stack)
{
	FGuid ItemGuid;
	Stack.StepCompiledIn(&ItemGuid);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemGuid == ItemGuid;
		}, FFortItemEntry::Size());

	if (!entry || !PlayerController->MyFortPawn)
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*)entry->ItemDefinition;

	static auto ContextTrapClass = FindClass("FortDecoTool_ContextTrap");

	if (ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
		ItemDefinition = ItemDefinition->GetWeaponItemDefinition();
	else if (ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass()))
	{
		PlayerController->MyFortPawn->PickUpActor(nullptr, ItemDefinition);
		((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ItemEntryGuid = ItemGuid;

		if (PlayerController->MyFortPawn->CurrentWeapon->IsA(ContextTrapClass))
			((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = ItemDefinition;

		return;
	}

	auto Weapon = PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid, entry->HasTrackerGuid() ? entry->TrackerGuid : FGuid(), false);
	if (VersionInfo.FortniteVersion <= 2.5)
	{
		static auto BuildingToolClass = FindClass("FortWeap_BuildingTool");
		if (Weapon->IsA(BuildingToolClass))
		{
			static auto RoofPiece = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_RoofS.BuildingItemData_RoofS");
			static auto FloorPiece = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Floor.BuildingItemData_Floor");
			static auto WallPiece = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Wall.BuildingItemData_Wall");
			static auto StairPiece = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Stair_W.BuildingItemData_Stair_W");

			static auto RoofMetadata = FindObject<UObject>(L"/Game/Building/EditModePatterns/Roof/EMP_Roof_RoofC.EMP_Roof_RoofC");
			static auto StairMetadata = FindObject<UObject>(L"/Game/Building/EditModePatterns/Stair/EMP_Stair_StairW.EMP_Stair_StairW");
			static auto WallMetadata = FindObject<UObject>(L"/Game/Building/EditModePatterns/Wall/EMP_Wall_Solid.EMP_Wall_Solid");
			static auto FloorMetadata = FindObject<UObject>(L"/Game/Building/EditModePatterns/Floor/EMP_Floor_Floor.EMP_Floor_Floor");

			static auto DefaultMetadataOffset = Weapon->GetOffset("DefaultMetadata");
			static auto OnRep_DefaultMetadata = Weapon->GetFunction("OnRep_DefaultMetadata");

			if (ItemDefinition == RoofPiece)
				GetFromOffset<const UObject*>(Weapon, DefaultMetadataOffset) = RoofMetadata;
			else if (ItemDefinition == StairPiece)
				GetFromOffset<const UObject*>(Weapon, DefaultMetadataOffset) = StairMetadata;
			else if (ItemDefinition == WallPiece)
				GetFromOffset<const UObject*>(Weapon, DefaultMetadataOffset) = WallMetadata;
			else if (ItemDefinition == FloorPiece)
				GetFromOffset<const UObject*>(Weapon, DefaultMetadataOffset) = FloorMetadata;

			Weapon->ProcessEvent(OnRep_DefaultMetadata, nullptr);
		}
	}
}


void AFortPlayerControllerAthena::ServerExecuteInventoryWeapon(UObject* Context, FFrame& Stack)
{
	AFortWeapon* Weapon;
	Stack.StepCompiledIn(&Weapon);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemGuid == Weapon->ItemEntryGuid;
		}, FFortItemEntry::Size());

	if (!entry || !PlayerController->MyFortPawn)
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*)entry->ItemDefinition;

	PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, Weapon->ItemEntryGuid, entry->HasTrackerGuid() ? entry->TrackerGuid : FGuid(), false);
}

uint64_t CantBuild_ = 0;
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
		struct FCreateBuildingActorData { uint32_t BuildingClassHandle; _Pad_0xC BuildLoc; _Pad_0xC BuildRot; bool bMirrored; uint8_t Pad_1[0x3]; float SyncKey; uint8 Pad_2[0x4]; uint8_t BuildingClassData[0x10]; };
		struct FCreateBuildingActorData_New { uint32_t BuildingClassHandle; uint8_t Pad_1[0x4]; _Pad_0x18 BuildLoc; _Pad_0x18 BuildRot; bool bMirrored; uint8_t Pad_2[0x3]; float SyncKey; uint8_t BuildingClassData[0x10]; };

		if (VersionInfo.FortniteVersion >= 20.00)
		{
			FCreateBuildingActorData_New CreateBuildingData;
			Stack.StepCompiledIn(&CreateBuildingData);

			BuildLoc = *(FVector*)&CreateBuildingData.BuildLoc;
			BuildRot = *(FRotator*)&CreateBuildingData.BuildRot;
			bMirrored = CreateBuildingData.bMirrored;

			auto BuildingClassPtr = GameState->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](TSubclassOf<AActor> Class, int32 Handle)
				{
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

			auto BuildingClassPtr = GameState->AllPlayerBuildableClassesIndexLookup.SearchForKey([&](TSubclassOf<AActor> Class, int32 Handle)
				{
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

		static auto HasAllPlayerBuildableClasses = GameState->HasAllPlayerBuildableClasses();
		if (HasAllPlayerBuildableClasses && !GameState->AllPlayerBuildableClasses.Contains(BuildingClassData.BuildingClass))
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

	UFortWorldItem* Item = nullptr;
	if (!PlayerController->bBuildFree && !FConfiguration::bInfiniteMats)
	{	
		auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
			{ return entry->ItemEntry.ItemDefinition == Resource; });

		if (!ItemP)
			return;

		Item = *ItemP;

		if (Item->ItemEntry.Count < 10)
			return;
	}

	/*TArray<ABuildingSMActor*> RemoveBuildings;
	char _Unk_OutVar1;
	static auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
	static auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
	if (VersionInfo.FortniteVersion >= 20.00 ? CantBuildNew(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildLoc, *(_Pad_0x18*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1) : CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0xC*)&BuildLoc, *(_Pad_0xC*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1))
	{
		printf("cantbuild?");
		return;
	}*/
	static auto FuncPtr = GameState->StructuralSupportSystem->GetFunction("CanAddBuildingActorClassToGrid");

	TArray<ABuildingSMActor*> RemoveBuildings;
	if (FuncPtr)
	{
		if (GameState->StructuralSupportSystem->Call<uint8_t>(FuncPtr, UWorld::GetWorld(), BuildingClass, BuildLoc, BuildRot, bMirrored, &RemoveBuildings, nullptr, false))
		{
			printf("cantbuild?");
			return;
		}
	}
	else
	{
		char _Unk_OutVar1;
		static auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
		static auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
		if (VersionInfo.FortniteVersion >= 20.00 ? CantBuildNew(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildLoc, *(_Pad_0x18*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1) : CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0xC*)&BuildLoc, *(_Pad_0xC*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1))
		{
			printf("cantbuild?");
			return;
		}
	}	

	for (auto& RemoveBuilding : RemoveBuildings)
		RemoveBuilding->K2_DestroyActor();
	RemoveBuildings.Free();


	static auto K2_SpawnBuildingActor = ABuildingSMActor::GetDefaultObj()->GetFunction("K2_SpawnBuildingActor");

	ABuildingSMActor* Building = nullptr;
	if (K2_SpawnBuildingActor)
	{
		FTransform SpawnTransform(BuildLoc, BuildRot);
		Building = ABuildingSMActor::K2_SpawnBuildingActor(PlayerController, BuildingClass, SpawnTransform, PlayerController, nullptr, false, false);
	}
	else
		Building = UWorld::SpawnActor<ABuildingSMActor>(BuildingClass, BuildLoc, BuildRot, PlayerController);
	if (!Building)
	{
		printf("building didnt spawn for some reason\n");
		return;
	}

	//Building->CurrentBuildingLevel = CreateBuildingData.BuildingClassData.UpgradeLevel;
	//Building->OnRep_CurrentBuildingLevel();

	Building->SetMirrored(bMirrored);

	Building->bPlayerPlaced = true;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr);

		
	if (!PlayerController->bBuildFree && !FConfiguration::bInfiniteMats)
	{
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemDefinition == Resource; }, FFortItemEntry::Size());

		/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
		{
			auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

			if (StateValue.StateType != 2)
				continue;

			StateValue.IntValue = 0;
		}*/

		Item->ItemEntry.Count -= 10;
		if (Item->ItemEntry.Count <= 0)
			PlayerController->WorldInventory->Remove(Item->ItemEntry.ItemGuid);
		else
		{
			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
				break;
			}*/


			PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
		}
	}

	Building->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	if (Building->HasTeamIndex())
		Building->TeamIndex = Building->Team;
}

void SetEditingPlayer(ABuildingSMActor* _this, AFortPlayerStateAthena* NewEditingPlayer)
{
	if (_this->Role == 3 && (!_this->EditingPlayer || !NewEditingPlayer))
	{
		_this->SetNetDormancy(2 - (NewEditingPlayer != 0));
		_this->ForceNetUpdate();

		auto EditingPlayer = _this->EditingPlayer;
		if (EditingPlayer)
		{
			auto Handle = EditingPlayer->Owner;

			if (Handle)
				if (auto PlayerController = Handle->Cast<AFortPlayerControllerAthena>())
				{
					_this->EditingPlayer = NewEditingPlayer;
					return;
				}
		}
		else
		{
			if (!NewEditingPlayer)
			{
				_this->EditingPlayer = NewEditingPlayer;
				return;
			}

			auto Handle = NewEditingPlayer->Owner;

			if (auto PlayerController = Handle->Cast<AFortPlayerControllerAthena>())
				_this->EditingPlayer = NewEditingPlayer;
		}
	}
}

bool CanBePlacedByPlayer(TSubclassOf<AActor> BuildClass)
{
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	static auto HasAllPlayerBuildableClasses = GameState->HasAllPlayerBuildableClasses();
	return HasAllPlayerBuildableClasses ? GameState->AllPlayerBuildableClasses.Search([BuildClass](TSubclassOf<AActor> Class)
		{ return Class == BuildClass; }) != 0 : true;
}

void AFortPlayerControllerAthena::ServerBeginEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex)
		return;


	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
	if (!PlayerState)
		return;

	SetEditingPlayer(Building, PlayerState);

	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass());
		}, FFortItemEntry::Size());


	PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);

	if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
	{
		EditTool->EditActor = Building;
		EditTool->OnRep_EditActor();
	}
}


uint64_t ReplaceBuildingActor_ = 0;
uint64_t InitializeBuildingActor_ = 0;
uint64_t PostInitializeSpawnedBuildingActor_ = 0;
void AFortPlayerControllerAthena::ServerEditBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	TSubclassOf<AActor> NewClass;
	uint8 RotationIterations;
	bool bMirrored;
	Stack.StepCompiledIn(&Building);
	Stack.StepCompiledIn(&NewClass);
	Stack.StepCompiledIn(&RotationIterations);
	Stack.StepCompiledIn(&bMirrored);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !Building || !NewClass || !Building->IsA<ABuildingSMActor>() || !CanBePlacedByPlayer(NewClass) || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex || Building->bDestroyed)
	{
		return;
	}

	SetEditingPlayer(Building, nullptr);

	static auto ReplaceBuildingActor = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, TSubclassOf<AActor>, unsigned int, int, bool, AFortPlayerControllerAthena*)) ReplaceBuildingActor_;
	static auto ReplaceBuildingActor__New = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, TSubclassOf<AActor>&, unsigned int, int, bool, AFortPlayerControllerAthena*)) ReplaceBuildingActor_;

	ABuildingSMActor* NewBuild;
	
	if (VersionInfo.FortniteVersion < 27)
		NewBuild = ReplaceBuildingActor(Building, 1, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);
	else
		NewBuild = ReplaceBuildingActor__New(Building, 1, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);

	/*else
	{
		// reimpl of replacebuildingactor
		NewBuild = ABuildingSMActor::K2_SpawnBuildingActor(PlayerController, NewClass, Building->GetTransform(), nullptr, nullptr, true, false);

		NewBuild->CurrentBuildingLevel = Building->CurrentBuildingLevel;
		NewBuild->SetMirrored(bMirrored);
		NewBuild->SetTeam(Building->TeamIndex);
		auto InitializeBuildingActor = (void(*)(ABuildingSMActor*,
			uint8 Reason,
			int16 InOwnerPersistentID,
			ABuildingSMActor * BuildingOwner,
			const ABuildingSMActor * ReplacedBuilding,
			bool bForcePlayBuildUpAnim)) InitializeBuildingActor_;
		InitializeBuildingActor(NewBuild, 2, ((AFortPlayerStateAthena*)PlayerController->PlayerState)->WorldPlayerId, nullptr, Building, true);
		NewBuild->BuildingReplacementType = 1;

		Building->ReplacementDestructionReason = 1;
		Building->OnReplacementDestruction.Process(1, NewBuild);
		Building->bAutoReleaseCurieContainerOnDestroyed = false;

		auto PostInitializeSpawnedBuildingActor = (void(*)(ABuildingSMActor*, uint8_t Reason)) PostInitializeSpawnedBuildingActor_;
		PostInitializeSpawnedBuildingActor(NewBuild, 2);
		UWorld::FinishSpawnActor(NewBuild, Building->K2_GetActorLocation(), Building->K2_GetActorRotation());
		Building->SilentDie(true);
	}*/
	
	if (NewBuild)
	{
		NewBuild->bPlayerPlaced = true;
	}

	if (VersionInfo.EngineVersion >= 4.24)
	{
		// serverendeditingbuildingactor cus they made it not call idk why
		SetEditingPlayer(Building, nullptr);

		auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass());
			}, FFortItemEntry::Size());

		PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);

		if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
		{
			EditTool->EditActor = nullptr;
			EditTool->OnRep_EditActor();
		}
	}
}

void AFortPlayerControllerAthena::ServerEndEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != PlayerController->PlayerState || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass());
		}, FFortItemEntry::Size());

	if (VersionInfo.EngineVersion >= 4.24)
		PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);

	if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
	{
		EditTool->EditActor = nullptr;
		EditTool->OnRep_EditActor();
	}
}


void AFortPlayerControllerAthena::ServerRepairBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto Price = (int32)std::floor((10.f * (1.f - Building->GetHealthPercent())) * 0.75f);
	auto res = UFortKismetLibrary::K2_GetResourceItemDefinition(Building->ResourceType);
	auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([res](UFortWorldItem* entry)
		{
			return entry->ItemEntry.ItemDefinition == res;
		});
	if (!ItemP)
		return;
	auto Item = *ItemP;
	if ((Item->ItemEntry.Count - Price) < 0)
		return;

	Item->ItemEntry.Count -= Price;
	if (Item->ItemEntry.Count <= 0)
		PlayerController->WorldInventory->Remove(Item->ItemEntry.ItemGuid);
	else
		PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);

	Building->RepairBuilding(PlayerController, Price);
}

void AFortPlayerControllerAthena::ServerAttemptInventoryDrop(UObject* Context, FFrame& Stack)
{
	FGuid Guid;
	int32 Count;
	bool bTrash = false; // this only exists on some newer builds
	Stack.StepCompiledIn(&Guid);
	Stack.StepCompiledIn(&Count);
	Stack.StepCompiledIn(&bTrash);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	if (!PlayerController || !PlayerController->Pawn)
		return;

	auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
		{ return entry->ItemEntry.ItemGuid == Guid; });
	if (!ItemP)
		return;
	auto Item = *ItemP;

	Item->ItemEntry.Count -= Count;
	AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), Item->ItemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn, Count);
	if (Item->ItemEntry.Count <= 0 || Count < 0)
		PlayerController->WorldInventory->Remove(Guid);
	else
		PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
}

class UAthenaToyItemDefinition : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UAthenaToyItemDefinition);

	DEFINE_PROP(ToySpawnAbility, TSoftClassPtr<UClass>);
};

extern uint64_t ConstructAbilitySpec;
uint64_t GiveAbilityAndActivateOnce;
void AFortPlayerControllerAthena::ServerPlayEmoteItem(UObject* Context, FFrame& Stack)
{
	UObject* Asset;
	float RandomNumber = 0.f;
	Stack.StepCompiledIn(&Asset);
	Stack.StepCompiledIn(&RandomNumber);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	if (!PlayerController || !PlayerController->MyFortPawn || !Asset)
		return;

	auto* AbilitySystemComponent = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->AbilitySystemComponent;
	auto Spec = (FGameplayAbilitySpec*)malloc(FGameplayAbilitySpec::Size());
	__stosb(PBYTE(Spec), 0, FGameplayAbilitySpec::Size());
	UObject* AbilityToUse = nullptr;

	static auto SprayClass = FindClass("AthenaSprayItemDefinition");
	if (Asset->IsA(SprayClass))
	{
		static auto SprayAbilityClass = FindObject<UClass>(L"/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");
		AbilityToUse = SprayAbilityClass->GetDefaultObj();
	}
	else if (auto ToyAsset = Asset->Cast<UAthenaToyItemDefinition>())
		AbilityToUse = ToyAsset->ToySpawnAbility->GetDefaultObj();
	else if (auto DanceAsset = Asset->Cast<UAthenaDanceItemDefinition>())
	{
		static auto HasbMovingEmote = PlayerController->MyFortPawn->HasbMovingEmote();
		if (HasbMovingEmote)
			PlayerController->MyFortPawn->bMovingEmote = DanceAsset->bMovingEmote;

		static auto HasWalkForwardSpeed = PlayerController->MyFortPawn->HasEmoteWalkSpeed();
		if (HasWalkForwardSpeed)
			PlayerController->MyFortPawn->EmoteWalkSpeed = DanceAsset->WalkForwardSpeed;

		static auto HasbMovingEmoteForwardOnly = PlayerController->MyFortPawn->HasbMovingEmoteForwardOnly();
		if (HasbMovingEmoteForwardOnly)
			PlayerController->MyFortPawn->bMovingEmoteForwardOnly = DanceAsset->bMoveForwardOnly;

		static auto HasbMovingEmoteFollowingOnly = PlayerController->MyFortPawn->HasbMovingEmoteFollowingOnly();
		if (HasbMovingEmoteFollowingOnly)
			PlayerController->MyFortPawn->bMovingEmoteFollowingOnly = DanceAsset->bMoveFollowingOnly;

		static auto EmoteAbilityClass = FindObject<UClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		AbilityToUse = EmoteAbilityClass->GetDefaultObj();
	}

	if (AbilityToUse)
	{
		if (ConstructAbilitySpec)
			((void (*)(FGameplayAbilitySpec*, const UObject*, int, int, UObject*)) ConstructAbilitySpec)(Spec, AbilityToUse, 1, -1, Asset);
		else
		{
			Spec->MostRecentArrayReplicationKey = -1;
			Spec->ReplicationID = -1;
			Spec->ReplicationKey = -1;
			Spec->Ability = (UFortGameplayAbility*)AbilityToUse;
			Spec->Level = 1;
			Spec->InputID = -1;
			Spec->Handle.Handle = rand();
			Spec->SourceObject = Asset;
		}
		FGameplayAbilitySpecHandle handle;
		((void (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec*, void*)) GiveAbilityAndActivateOnce)(AbilitySystemComponent, &handle, Spec, nullptr);
	}
}

uint8 ToDeathCause(AFortPlayerPawnAthena* Pawn, FGameplayTagContainer& DeathTags, bool bDBNO)
{
	static auto ToDeathCause = AFortPlayerStateAthena::GetDefaultObj()->GetFunction("ToDeathCause");
	if (ToDeathCause)
	{
		if (!AFortPlayerStateAthena::ToDeathCause__Ptr)
			AFortPlayerStateAthena::ToDeathCause__Ptr = ToDeathCause;

		return AFortPlayerStateAthena::ToDeathCause(DeathTags, bDBNO);
	}
	else if (VersionInfo.EngineVersion >= 4.19)
	{
		static uint64_t ToDeathCauseNative = 0;

		if (!ToDeathCauseNative)
		{
			if (VersionInfo.EngineVersion == 4.19)
				ToDeathCauseNative = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 41 0F B6 F8 48 8B DA 48 8B F1 E8 ? ? ? ? 33 ED").Get();
			else if (VersionInfo.EngineVersion == 4.20)
				ToDeathCauseNative = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 0F B6 FA 48 8B D9 E8 ? ? ? ? 33 F6 48 89 74 24").Get();
			else if (VersionInfo.EngineVersion == 4.21)
				ToDeathCauseNative = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 0F B6 FA 48 8B D9 E8 ? ? ? ? 33").Get();
		}


		if (ToDeathCauseNative)
		{
			if (VersionInfo.EngineVersion == 4.19)
			{
				static uint8(*ToDeathCause_)(AFortPlayerPawnAthena * Pawn, FGameplayTagContainer TagContainer, char bDBNO) = decltype(ToDeathCause_)(ToDeathCauseNative);
				return ToDeathCause_(Pawn, DeathTags, bDBNO);
			}
			else
			{
				static uint8(*ToDeathCause_)(FGameplayTagContainer TagContainer, char bDBNO) = decltype(ToDeathCause_)(ToDeathCauseNative);
				return ToDeathCause_(DeathTags, bDBNO);
			}
		}
	}

	return 0;
}

uint64 RemoveFromAlivePlayers_ = 0;
void AFortPlayerControllerAthena::ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
{
	if (!PlayerController)
		return ClientOnPawnDiedOG(PlayerController, DeathReport);
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

	if (!GameState->IsRespawningAllowed(PlayerState) && PlayerController->WorldInventory && PlayerController->MyFortPawn)
	{
		bool bHasMats = false;
		for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (entry.ItemDefinition->bCanBeDropped)
				AFortInventory::SpawnPickup(PlayerController->MyFortPawn->K2_GetActorLocation(), entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetPlayerElimination(), PlayerController->MyFortPawn);
		}
	}


	auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;

	if (PlayerState->HasPawnDeathLocation())
		PlayerState->PawnDeathLocation = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector();

	PlayerState->DeathInfo.bDBNO = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false;
	if (FDeathInfo::HasKiller())
		PlayerState->DeathInfo.Killer = KillerPlayerState;
	if (FDeathInfo::HasDeathLocation())
		PlayerState->DeathInfo.DeathLocation = PlayerState->HasPawnDeathLocation() ? PlayerState->PawnDeathLocation : (PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector());
	if (FDeathInfo::HasDeathTags())
		PlayerState->DeathInfo.DeathTags = DeathReport.Tags;
	PlayerState->DeathInfo.DeathCause = ToDeathCause(PlayerController->MyFortPawn, DeathReport.Tags, PlayerState->DeathInfo.bDBNO);
	//PlayerState->DeathInfo.Downer = KillerPlayerState;
	if (FDeathInfo::HasFinisherOrDowner())
		PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
	if (FDeathInfo::HasFinisherOrDownerTags())
		PlayerState->DeathInfo.FinisherOrDownerTags = KillerPawn ? KillerPawn->GameplayTags : PlayerController->MyFortPawn->GameplayTags;
	if (FDeathInfo::HasVictimTags())
		PlayerState->DeathInfo.VictimTags = PlayerController->Pawn->GameplayTags;
	if (FDeathInfo::HasDistance())
		PlayerState->DeathInfo.Distance = PlayerController->MyFortPawn ? (PlayerState->DeathInfo.DeathCause != /*EDeathCause::FallDamage*/ 1 ? (KillerPawn ? KillerPawn->GetDistanceTo(PlayerController->MyFortPawn) : 0) : (PlayerController->MyFortPawn->HasLastFallDistance() ? PlayerController->MyFortPawn->LastFallDistance : 0)) : 0;
	if (FDeathInfo::HasbInitialized())
		PlayerState->DeathInfo.bInitialized = true;
	PlayerState->OnRep_DeathInfo();

	if (KillerPlayerState && KillerPawn && KillerPawn->Controller && KillerPawn->Controller != PlayerController)
	{
		if (KillerPlayerState->HasKillScore())
			KillerPlayerState->KillScore++;
		else
			KillerPlayerState->Kills++;
		KillerPlayerState->OnRep_Kills();
		if (KillerPlayerState->HasTeamKillScore())
		{
			KillerPlayerState->TeamKillScore++;
			KillerPlayerState->OnRep_TeamKillScore();
		}

		struct Test { AFortPlayerStateAthena* ps; uint8_t p[0x8]; };

		Test t{ PlayerState };
		KillerPlayerState->ClientReportKill(t);
		if (KillerPlayerState->HasTeamKillScore())
			KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);
	}

	if (!GameState->IsRespawningAllowed(PlayerState) && (PlayerController->MyFortPawn ? !PlayerController->MyFortPawn->IsDBNO() : true))
	{
		PlayerState->Place = GameState->PlayersLeft;
		PlayerState->OnRep_Place();

		AFortWeapon* DamageCauser = nullptr;
		static auto ProjectileBaseClass = FindClass("FortProjectileBase");
		if (DeathReport.DamageCauser ? DeathReport.DamageCauser->IsA(ProjectileBaseClass) : false)
			DamageCauser = (AFortWeapon*)DeathReport.DamageCauser->Owner;
		else if (auto Weapon = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortWeapon>() : nullptr)
			DamageCauser = Weapon;
		if (RemoveFromAlivePlayers_)
		{
			((void (*)(AFortGameModeAthena*, AFortPlayerControllerAthena*, AFortPlayerStateAthena*, AFortPlayerPawnAthena*, UFortItemDefinition*, uint8, char))RemoveFromAlivePlayers_)(GameMode, PlayerController, KillerPlayerState == PlayerState ? nullptr : KillerPlayerState, KillerPawn, DamageCauser ? DamageCauser->WeaponData : nullptr, PlayerState->DeathInfo.DeathCause, 0);
		}

		if (VersionInfo.FortniteVersion >= 15)
		{
			//static auto SpectatingName = UKismetStringLibrary::Conv_StringToName(FString(L"Spectating"));
			//PlayerController->StateName = SpectatingName;
			//PlayerController->ClientGotoState(SpectatingName);
			PlayerController->Pawn->CharacterMovement->ProcessEvent(PlayerController->Pawn->CharacterMovement->GetFunction("DisableMovement"), nullptr);
		}


		if (FConfiguration::SiphonAmount > 0 && VersionInfo.EngineVersion <= 5.0 && PlayerController->MyFortPawn && KillerPlayerState && KillerPlayerState->AbilitySystemComponent && KillerPawn && KillerPawn->Controller != PlayerController)
		{
			auto Handle = KillerPlayerState->AbilitySystemComponent->MakeEffectContext();
			FGameplayTag Tag;
			static auto Cue = UKismetStringLibrary::Conv_StringToName(FString(L"GameplayCue.Shield.PotionConsumed"));
			Tag.TagName = Cue;
			auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
			__stosb((PBYTE)PredictionKey, 0, FPredictionKey::Size());
			KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, *PredictionKey, Handle);
			KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, *PredictionKey, Handle);
			free(PredictionKey);

			auto Health = KillerPawn->GetHealth();
			auto Shield = KillerPawn->GetShield();

			if (Health == 100)
			{
				Shield += Shield + FConfiguration::SiphonAmount;
			}
			else if (Health + FConfiguration::SiphonAmount > 100)
			{
				Health = 100;
				Shield += (Health + FConfiguration::SiphonAmount) - 100;
			}
			else if (Health + FConfiguration::SiphonAmount <= 100)
			{
				Health += FConfiguration::SiphonAmount;
			}

			KillerPawn->SetHealth(Health);
			KillerPawn->SetShield(Shield);
			//forgot to add this back
		}

		if (PlayerController->MyFortPawn && ((KillerPlayerState && KillerPlayerState->Place == 1) || PlayerState->Place == 1))
		{
			if (PlayerState->Place == 1)
			{
				KillerPlayerState = PlayerState;
				KillerPawn = (AFortPlayerPawnAthena*)PlayerController->MyFortPawn;
			}

			auto KillerPlayerController = (AFortPlayerControllerAthena*)KillerPlayerState->Owner;
			auto KillerWeapon = DamageCauser ? DamageCauser->WeaponData : nullptr;


			KillerPlayerController->PlayWinEffects(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause, false);
			KillerPlayerController->ClientNotifyWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
			KillerPlayerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);

			if (KillerPlayerState != PlayerState && VersionInfo.FortniteVersion >= 19)
			{
				auto Crown = FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");
				KillerPlayerController->WorldInventory->GiveItem(Crown, 1);
			}

			GameState->WinningTeam = KillerPlayerState->TeamIndex;
			GameState->OnRep_WinningTeam();
			if (GameState->HasWinningPlayerState())
			{
				GameState->WinningPlayerState = KillerPlayerState;
				GameState->OnRep_WinningPlayerState();
			}
		}

	}

	return ClientOnPawnDiedOG(PlayerController, DeathReport);
}

void AFortPlayerControllerAthena::ServerClientIsReadyToRespawn(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

	if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
	{
		auto RespawnData = PlayerState->RespawnData;
		FTransform SpawnTransform{};

		FQuat Rotation = PlayerState->RespawnData.RespawnRotation;
		SpawnTransform.Translation = PlayerState->RespawnData.RespawnLocation;
		SpawnTransform.Rotation = Rotation;

		auto Scale = FVector(1, 1, 1);
		SpawnTransform.Scale3D = Scale;

		auto NewPawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, SpawnTransform);
		PlayerController->Possess(NewPawn);
		PlayerController->RespawnPlayerAfterDeath(true);

		NewPawn->SetHealth(100.f);
		NewPawn->SetShield(0.f);

		PlayerState->RespawnData.bClientIsReady = true;
	}
}

void AFortPlayerControllerAthena::InternalPickup(FFortItemEntry* PickupEntry)
{
	if (!PickupEntry || !PickupEntry->ItemDefinition)
		return;

	auto MaxStack = (int32)PickupEntry->ItemDefinition->GetMaxStackSize();
	int ItemCount = 0;


	if (!PickupEntry->ItemDefinition->HasbForceIntoOverflow() || !PickupEntry->ItemDefinition->bForceIntoOverflow)
		for (int i = 0; i < WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Item = WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (AFortInventory::IsPrimaryQuickbar(Item.ItemDefinition) && (!Item.ItemDefinition->HasbForceIntoOverflow() || !Item.ItemDefinition->bForceIntoOverflow))
				ItemCount += Item.ItemDefinition->HasNumberOfSlotsToTake() ? Item.ItemDefinition->NumberOfSlotsToTake : 1;
		}

	//printf("br: %d\n", ItemCount);
	auto GiveOrSwap = [&]()
		{
			if (ItemCount >= 5 && AFortInventory::IsPrimaryQuickbar(PickupEntry->ItemDefinition))
			{
				if (AFortInventory::IsPrimaryQuickbar(((AFortWeapon*)MyFortPawn->CurrentWeapon)->WeaponData))
				{
					auto itemEntry = WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
						{ return entry.ItemGuid == ((AFortWeapon*)MyFortPawn->CurrentWeapon)->ItemEntryGuid; }, FFortItemEntry::Size());


					AFortInventory::SpawnPickup(GetViewTarget()->K2_GetActorLocation(), *itemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn);
					WorldInventory->Remove(((AFortWeapon*)MyFortPawn->CurrentWeapon)->ItemEntryGuid);
					WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
				}
				else
					AFortInventory::SpawnPickup(GetViewTarget()->K2_GetActorLocation(), *PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn);
			}
			else
				WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
		};

	auto GiveOrSwapStack = [&](int32 OriginalCount)
		{
			if (PickupEntry->ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
				WorldInventory->GiveItem(*PickupEntry, OriginalCount - MaxStack, true);
			else
				AFortInventory::SpawnPickup(GetViewTarget()->K2_GetActorLocation(), *PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, OriginalCount - MaxStack);
		};

	if (MaxStack > 1)
	{
		auto item = WorldInventory->Inventory.ItemInstances.Search([PickupEntry, MaxStack](UFortWorldItem* entry)
			{ return entry->ItemEntry.ItemDefinition == PickupEntry->ItemDefinition && entry->ItemEntry.Count < MaxStack; });
		auto itemEntry = WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
			{ return entry.ItemDefinition == PickupEntry->ItemDefinition && entry.Count < MaxStack; }, FFortItemEntry::Size());

		if (item)
		{
			bool bFound = false;
			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				bFound = true;
				StateValue.IntValue = 0;
				break;
			}*/

			if (((*item)->ItemEntry.Count += PickupEntry->Count) > MaxStack)
			{
				auto OriginalCount = (*item)->ItemEntry.Count;
				(*item)->ItemEntry.Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			// full proper
			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;
				
				StateValue.IntValue = 1;
				break;
			}

			if (!bFound)
			{
				auto Value = (FFortItemEntryStateValue*)malloc(FFortItemEntryStateValue::Size());
				__stosb((PBYTE)Value, 0, FFortItemEntryStateValue::Size());
				Value->IntValue = true;
				Value->StateType = 2;
				itemEntry->StateValues.Add(*Value, FFortItemEntryStateValue::Size());
				free(Value);
			}*/



			//(*item)->ItemEntry.Count = itemEntry->Count;
			WorldInventory->UpdateEntry((*item)->ItemEntry);
		}
		else
		{
			if (PickupEntry->Count > MaxStack)
			{
				auto OriginalCount = PickupEntry->Count;
				PickupEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			GiveOrSwap();
		}
	}
	else
		GiveOrSwap();
}

extern uint64_t ApplyCharacterCustomization;
extern uint64_t NotifyGameMemberAdded_;
void AFortPlayerControllerAthena::ServerCheat(UObject* Context, FFrame& Stack)
{
	FString Msg;
	Stack.StepCompiledIn(&Msg);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto fullCommand = Msg.ToString();

	std::vector<UEAllocatedString> args;

	size_t pos = 0, lastPos = 0;
	while ((pos = fullCommand.find(' ', lastPos)) != std::string::npos)
	{
		args.push_back(fullCommand.substr(lastPos, pos - lastPos));

		lastPos = pos + 1;
	}

	args.push_back(fullCommand.substr(lastPos));

	if (args.size() == 0)
	{
	_help:
		PlayerController->ClientMessage(FString(LR"(Command List:
    cheat startaircraft - Starts the battle bus
    cheat pausesafezone - Pauses the storm
	cheat giveitem <WID/path> <Count = 1> - Gives you an item
	cheat startevent - Starts the event for the current version
	cheat spawnpickup <WID/path> <Count = 1> - Spawns a pickup at your player's location
	cheat spawnactor <class/path> - Spawns an actor at your location + 5 meters
    cheat tp <X> <Y> <Z> - Teleports to a location)"), FName(), 1);
	}
	else
	{
		auto& command = args[0];
		std::transform(command.begin(), command.end(), command.begin(), tolower);

		if (command == "startaircraft")
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
		else if (command == "resumesafezone")
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startsafezone"), nullptr);
		else if (command == "pausesafezone")
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"pausesafezone"), nullptr);
		else if (command == "demospeed")
		{
			if (args.size() != 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			auto ws = L"demospeed " + UEAllocatedWString(args[1].begin(), args[1].end());

			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(ws.c_str()), nullptr);
		}
		else if (command == "god")
		{
			PlayerController->Pawn->bCanBeDamaged ^= 1;
		}
		else if (command == "speed")
		{
			float Speed = 1.0f;

			if (args.size() > 1)
			{
				try { Speed = std::stof(std::string(args[1])); }
				catch (...)
				{
					PlayerController->ClientMessage(FString(L"Invalid speed value"), FName(), 1);
					return;
				}
			}

			auto Pawn = PlayerController->Pawn;
			if (!Pawn)
				PlayerController->ClientMessage(FString(L"No pawn to set speed"), FName(), 1);

			static auto SetMovementSpeedFn = Pawn->GetFunction("SetMovementSpeed");
			if (!SetMovementSpeedFn)
				SetMovementSpeedFn = Pawn->GetFunction("SetMovementSpeedMultiplier");

			if (!SetMovementSpeedFn)
				return;

			Pawn->ProcessEvent(SetMovementSpeedFn, &Speed);
		}
		else if (command == "timeofday" || command == "time" || command == "t")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;

			}

			float NewTOD = 0.f;
			try { NewTOD = std::stof(std::string(args[1])); }
			catch (...) {}

			UFortKismetLibrary::SetTimeOfDay(UWorld::GetWorld(), NewTOD);
		}
		else if (command == "spawnbot")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			int Count = 1;

			if (args.size() >= 3)
			{
				Count = std::stoi(args[2].c_str(), nullptr);
			}

			for (int i = 0; i < Count; i++)
			{
				auto Transform = PlayerController->Pawn->GetTransform();
				Transform.Translation.Z += 500.f;

				auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
				auto GameState = GameMode->GameState;
				//auto PlayerController = (AFortPlayerControllerAthena*)UWorld::SpawnActor(GameMode->PlayerControllerClass, FVector{});
				auto Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(GameMode->DefaultPawnClass, Transform);
				auto PlayerController = (AFortPlayerControllerAthena*)UWorld::SpawnActor(Pawn->AIControllerClass, FVector{});
				//auto PlayerState = PlayerController->PlayerState;

				PlayerController->Possess(Pawn);

				auto PlayerState = (AFortPlayerStateAthena*)UWorld::SpawnActor(AFortPlayerStateAthena::StaticClass(), FVector{});

				PlayerState->SetOwner(PlayerController);

				PlayerController->PlayerState = PlayerState;
				PlayerController->OnRep_PlayerState();

				Pawn->PlayerState = PlayerState;
				Pawn->OnRep_PlayerState();

				Pawn->SetMaxHealth(100.f);
				Pawn->SetHealth(100.f);

				PlayerState->TeamIndex = AFortGameModeAthena::PickTeam(GameMode, 0, PlayerController);
				if (PlayerState->HasSquadId())
					PlayerState->SquadId = PlayerState->TeamIndex - 3;
				if (PlayerState->HasbIsABot())
					PlayerState->bIsABot = true;


				if (GameState->HasGameMemberInfoArray())
				{
					auto Member = (FGameMemberInfo*)malloc(FGameMemberInfo::Size());
					__stosb((PBYTE)Member, 0, FGameMemberInfo::Size());

					Member->MostRecentArrayReplicationKey = -1;
					Member->ReplicationID = -1;
					Member->ReplicationKey = -1;
					Member->TeamIndex = PlayerState->TeamIndex;
					Member->SquadId = PlayerState->SquadId;
					Member->MemberUniqueId = PlayerState->UniqueId;

					GameState->GameMemberInfoArray.Members.Add(*Member, FGameMemberInfo::Size());
					GameState->GameMemberInfoArray.MarkItemDirty(*Member);

					auto NotifyGameMemberAdded = (void(*)(AFortGameStateAthena*, uint8_t, uint8_t, FUniqueNetIdRepl*)) NotifyGameMemberAdded_;
					if (NotifyGameMemberAdded)
						NotifyGameMemberAdded(GameState, Member->SquadId, Member->TeamIndex, &Member->MemberUniqueId);

					free(Member);
				}

				for (auto& AbilitySet : AFortGameModeAthena::AbilitySets)
					PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);

				/*PlayerController->WorldInventory = (AFortInventory*)UWorld::SpawnActor(AFortInventory::StaticClass(), FVector{});
				PlayerController->WorldInventory->SetOwner(PlayerController);
				PlayerController->WorldInventory->InventoryType = 0;*/
				//PlayerController->bHasInitializedWorldInventory = true;

				GameState->PlayersLeft++;
				GameState->OnRep_PlayersLeft();

				GameMode->AlivePlayers.Add(PlayerController);

				static auto Commando = FindObject(L"/Game/Athena/Heroes/HID_001_Athena_Commando_F.HID_001_Athena_Commando_F", nullptr);
				static auto Commando2 = FindObject(L"/Game/Athena/Heroes/HID_Commando_Athena_01.HID_Commando_Athena_01", nullptr);
				PlayerState->HeroType = Commando ? Commando : Commando2;

				if (ApplyCharacterCustomization)
					((void (*)(AActor*, AFortPlayerPawnAthena*)) ApplyCharacterCustomization)(PlayerState, Pawn);

				/*static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

				PlayerController->WorldInventory->GiveItem(DefaultPickaxe);

				static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");

				for (int i = 0; i < GameMode->StartingItems.Num(); i++)
				{
					auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

					if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
						PlayerController->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
				}*/
			}
		}
		else if (command == "startevent")
		{
			Events::StartEvent();
		}
		else if (command == "bugitgo" || command == "tp")
		{
			if (args.size() != 4)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			double X = 0., Y = 0., Z = 0.;

			X = strtod(args[1].c_str(), nullptr);
			Y = strtod(args[2].c_str(), nullptr);
			Z = strtod(args[3].c_str(), nullptr);

			if (PlayerController->Pawn)
				PlayerController->Pawn->K2_SetActorLocation(FVector(X, Y, Z), false, nullptr, true);
		}
		else if (command == "giveitem")
		{
			if (args.size() != 2 && args.size() != 3)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			auto ItemDefinition = FindObject<UFortItemDefinition>(UEAllocatedWString(args[1].begin(), args[1].end()));
			if (!ItemDefinition)
				ItemDefinition = TUObjectArray::FindObject<UFortItemDefinition>(args[1].c_str());

			if (!ItemDefinition)
				return PlayerController->ClientMessage(FString(L"Failed to find item! Try passing it as a path or check your spelling & casing"), FName(), 1);

			int32 Count = 1;

			if (args.size() == 3)
				Count = strtol(args[2].c_str(), nullptr, 10);

			auto ItemEntry = AFortInventory::MakeItemEntry(ItemDefinition, Count, 0);
			PlayerController->InternalPickup(ItemEntry);
			free(ItemEntry);
			//PlayerController->WorldInventory->GiveItem(ItemDefinition, Count);
		}
		else if (command == "spawnpickup")
		{
			if (args.size() != 2 && args.size() != 3)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			auto ItemDefinition = FindObject<UFortItemDefinition>(UEAllocatedWString(args[1].begin(), args[1].end()));
			if (!ItemDefinition)
				ItemDefinition = TUObjectArray::FindObject<UFortItemDefinition>(args[1].c_str());

			if (!ItemDefinition)
				return PlayerController->ClientMessage(FString(L"Failed to find item! Try passing it as a path or check your spelling & casing"), FName(), 1);

			long Count = 1;

			if (args.size() == 3)
				Count = strtol(args[2].c_str(), nullptr, 10);

			if (PlayerController->Pawn)
				AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation(), ItemDefinition, Count, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), PlayerController->Pawn);
		}
		else if (command == "spawnactor" || command == "summon")
		{
			if (args.size() != 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1);
				return;
			}

			auto Loc = PlayerController->Pawn->K2_GetActorLocation();
			Loc.Z += 500.f;

			auto Class = FindObject<UClass>(UEAllocatedWString(args[1].begin(), args[1].end()).c_str());

			if (!Class)
				Class = FindClass(args[1].c_str());

			if (Class)
				UWorld::SpawnActor(Class, Loc);
			else
				return PlayerController->ClientMessage(FString(L"Failed to find class! Try passing it as a path or check your spelling & casing"), FName(), 1);
		}
		else
			goto _help;
	}
}

extern bool bDidntFind;
void AFortPlayerControllerAthena::ServerAttemptInteract_(UObject* Context, FFrame& Stack)
{
	AActor* ReceivingActor = *(AActor**)Stack.Locals;
	/*AActor* ReceivingActor;
	UObject* InteractComponent;
	uint8_t InteractType;
	UObject* OptionalObjectData;
	uint8_t InteractionBeingAttempted;
	int32 RequestID;

	Stack.StepCompiledIn(&ReceivingActor);
	Stack.StepCompiledIn(&InteractComponent);
	Stack.StepCompiledIn(&InteractType);
	Stack.StepCompiledIn(&OptionalObjectData);
	Stack.StepCompiledIn(&InteractionBeingAttempted);
	Stack.StepCompiledIn(&RequestID);
	Stack.IncrementCode();*/

	AFortPlayerControllerAthena* PlayerController = nullptr;

	static auto bIsComp = Context->IsA(FindClass("FortControllerComponent_Interaction"));
	if (bIsComp)
		PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
	else
		PlayerController = (AFortPlayerControllerAthena*)Context;

	if (auto Container = bDidntFind ? ReceivingActor->Cast<ABuildingContainer>() : nullptr)
		UFortLootPackage::SpawnLootHook(Container);
	else if (auto CollectorActor = ReceivingActor->Cast<ABuildingItemCollectorActor>())
	{
		CollectorActor->ControllingPlayer = PlayerController;


		auto Collection = CollectorActor->ItemCollections.Search([&](FCollectorUnitInfo& Coll)
			{
				return Coll.InputItem == CollectorActor->ActiveInputItem;
			}, FCollectorUnitInfo::Size());

		if (!Collection)
		{
			CollectorActor->bCurrentInteractionSuccess = false;
			CollectorActor->ControllingPlayer = nullptr;
			CollectorActor->Call(CollectorActor->GetFunction("BlueprintOnInteract"), PlayerController->MyFortPawn);
			return ServerAttemptInteract_OG(Context, Stack);
		}

		float Cost = Collection->InputCount.Evaluate((float)CollectorActor->StartingGoalLevel);
		if (Cost > 0)
		{
			auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
				{
					return entry->ItemEntry.ItemDefinition == Collection->InputItem;
				});

			if (!ItemP || (*ItemP)->ItemEntry.Count < (int)Cost)
			{
				CollectorActor->bCurrentInteractionSuccess = false;
				CollectorActor->ControllingPlayer = nullptr;
				CollectorActor->Call(CollectorActor->GetFunction("BlueprintOnInteract"), PlayerController->MyFortPawn);
				//CollectorActor->PlayVendFailFX();
				return ServerAttemptInteract_OG(Context, Stack);
			}

			auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
				{
					return entry.ItemDefinition == Collection->InputItem;
				}, FFortItemEntry::Size());
			auto Item = *ItemP;

			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
			}*/

			Item->ItemEntry.Count -= (int)Cost;
			if (Item->ItemEntry.Count <= 0)
				PlayerController->WorldInventory->Remove(Item->ItemEntry.ItemGuid);
			else
			{
				/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
				{
					auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

					if (StateValue.StateType != 2)
						continue;

					StateValue.IntValue = 0;
					break;
				}*/

				PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
			}
		}

		CollectorActor->ClientPausedActiveInputItem = CollectorActor->ActiveInputItem;
		CollectorActor->bCurrentInteractionSuccess = true;
		CollectorActor->Call(CollectorActor->GetFunction("BlueprintOnInteract"), PlayerController->MyFortPawn);
		//CollectorActor->PlayVendFX();
	}

	return ServerAttemptInteract_OG(Context, Stack);
	//return bIsComp ? (void)callOG(((UFortControllerComponent_Interaction*)Context), Stack.GetCurrentNativeFunction(), ServerAttemptInteract, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID) : callOG(PlayerController, Stack.GetCurrentNativeFunction(), ServerAttemptInteract, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID);
}

void AFortPlayerControllerAthena::ServerDropAllItems(UObject* Context, FFrame& Stack)
{
	UFortItemDefinition* IgnoreItemDef;

	Stack.StepCompiledIn(&IgnoreItemDef);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto Loc = PlayerController->MyFortPawn->K2_GetActorLocation();
	for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

		if (Entry.ItemDefinition != IgnoreItemDef && Entry.ItemDefinition->bCanBeDropped)
		{
			AFortInventory::SpawnPickup(Loc, Entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
			PlayerController->WorldInventory->Remove(Entry.ItemGuid);
		}
	}
}

uint64_t ClearAbility_ = 0;
void (*OnUnEquipOG)(AFortWeapon*);
void OnUnEquip(AFortWeapon* Weapon)
{
	auto ClearAbility = (void(*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle&)) ClearAbility_;
	auto PlayerController = (AFortPlayerControllerAthena*)((AFortPlayerPawnAthena*)Weapon->Instigator)->Controller;
	if (!PlayerController)
		return;
	auto AbilitySystemComponent = PlayerController->PlayerState->AbilitySystemComponent;

	if (Weapon->PrimaryAbilitySpecHandle.Handle != -1)
	{
		ClearAbility(AbilitySystemComponent, Weapon->PrimaryAbilitySpecHandle);
		Weapon->PrimaryAbilitySpecHandle.Handle = -1;
	}
	if (Weapon->SecondaryAbilitySpecHandle.Handle != -1)
	{
		ClearAbility(AbilitySystemComponent, Weapon->SecondaryAbilitySpecHandle);
		Weapon->SecondaryAbilitySpecHandle.Handle = -1;
	}
	if (Weapon->ReloadAbilitySpecHandle.Handle != -1)
	{
		ClearAbility(AbilitySystemComponent, Weapon->ReloadAbilitySpecHandle);
		Weapon->ReloadAbilitySpecHandle.Handle = -1;
	}
	if (Weapon->ImpactAbilitySpecHandle.Handle != -1)
	{
		ClearAbility(AbilitySystemComponent, Weapon->ImpactAbilitySpecHandle);
		Weapon->ImpactAbilitySpecHandle.Handle = -1;
	}
	if (Weapon->EquippedAbilityHandles.Num())
	{
		for (int i = 0; i < Weapon->EquippedAbilityHandles.Num(); i++)
		{
			auto& Handle = Weapon->EquippedAbilityHandles.Get(i, FGameplayAbilitySpecHandle::Size());
			if (Handle.Handle != -1)
			{
				ClearAbility(AbilitySystemComponent, Handle);
			}
		}
		Weapon->EquippedAbilityHandles.ResetNum();
	}
	if (Weapon->EquippedAbilitySetHandles.Num())
	{
		for (int i = 0; i < Weapon->EquippedAbilitySetHandles.Num(); i++)
		{
			auto& Handle = Weapon->EquippedAbilitySetHandles.Get(i, FFortAbilitySetHandle::Size());

			UFortKismetLibrary::UnequipFortAbilitySet(Handle);
		}

		PlayerController->AppliedInGameModifierAbilitySetHandles.Reset();
		Weapon->EquippedAbilitySetHandles.ResetNum();
	}

	return OnUnEquipOG(Weapon);
}

class UFortHeldObjectComponent : public UActorComponent
{
public:
	UCLASS_COMMON_MEMBERS(UFortHeldObjectComponent);

	DEFINE_PROP(EquippedWeaponItemDefinition, TSoftObjectPtr<UFortItemDefinition>);
	DEFINE_PROP(HeldObjectState, uint8);
	DEFINE_PROP(OwningPawn, AFortPlayerPawnAthena*);
	DEFINE_PROP(PreviousOwningPawn, TWeakObjectPtr<AFortPlayerPawnAthena>);
	DEFINE_PROP(GrantedWeaponItem, TWeakObjectPtr<UFortWorldItem>);
	DEFINE_PROP(GrantedWeapon, TWeakObjectPtr<AFortWeapon>);
	DEFINE_PROP(OnHeldObjectOwningPawnChanged, TMulticastInlineDelegate<void()>);

	DEFINE_FUNC(OnRep_OwningPawn, void);
};


void SetHeldObject(AFortPlayerPawnAthena* Pawn, AActor* HeldObject)
{
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;

	Pawn->HeldObject = HeldObject;
	PlayerController->bHoldingObject = HeldObject != nullptr;
}

void SetupOwningPawn(UFortHeldObjectComponent* HeldObjectComponent, AFortPlayerPawnAthena* Pawn)
{
	auto HeldObject = (AActor*)HeldObjectComponent->GetOwner();

	if (!HeldObject)
		return;

	HeldObject->FlushNetDormancy();

	if (Pawn)
		HeldObjectComponent->HeldObjectState = 1;

	AFortPlayerPawnAthena* PreviousPawn = nullptr;
	if (HeldObjectComponent->HasOwningPawn())
	{
		auto OldPawn = HeldObjectComponent->OwningPawn;
		HeldObjectComponent->PreviousOwningPawn = !Pawn ? HeldObjectComponent->OwningPawn : nullptr;
		HeldObjectComponent->OwningPawn = Pawn;
		//HeldObjectComponent->OnRep_OwningPawn(OldPawn);
		PreviousPawn = OldPawn;
	}
	else
	{
		// its a weakobjectptr on older builds
		static auto OwningPawnOff = HeldObjectComponent->GetOffset("OwningPawn", 0x8000000);
		auto& OwningPawn = *(TWeakObjectPtr<AFortPlayerPawnAthena>*)(__int64(HeldObjectComponent) + OwningPawnOff);

		HeldObjectComponent->PreviousOwningPawn = !Pawn ? OwningPawn.Get() : nullptr;
		auto OldPawn = OwningPawn.Get();
		OwningPawn = Pawn;
		//HeldObjectComponent->OnRep_OwningPawn(OldPawn);
		PreviousPawn = OldPawn;
	}

	if (Pawn)
		SetHeldObject(Pawn, HeldObject);

	HeldObject->ForceNetUpdate();
	HeldObjectComponent->OnHeldObjectOwningPawnChanged.Process();
	if (Pawn)
		Pawn->OnHeldObjectPickedUp.Process(HeldObject);
	else
		PreviousPawn->OnHeldObjectDropped.Process(HeldObject);
}

void PickupHeldObject(UObject* Context, FFrame& Stack)
{
	AFortPlayerPawnAthena* Pawn;

	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto HeldObjectComponent = (UFortHeldObjectComponent*)Context;

	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;

	SetupOwningPawn(HeldObjectComponent, Pawn);

	if (!HeldObjectComponent->GrantedWeaponItem.Get())
	{
		auto ItemDefinition = HeldObjectComponent->EquippedWeaponItemDefinition.Get();

		auto Item = PlayerController->WorldInventory->GiveItem(ItemDefinition, 1, 99999);

		if (!Item)
			return;

		auto Weapon = (AFortWeapon*)Pawn->EquipWeaponDefinition(ItemDefinition, Item->ItemEntry.ItemGuid, FFortItemEntry::HasTrackerGuid() ? Item->ItemEntry.TrackerGuid : FGuid(), false);

		if (!Weapon)
			return;
		PlayerController->ClientEquipItem(Item->ItemEntry.ItemGuid, true);

		HeldObjectComponent->GrantedWeapon = Weapon;
		HeldObjectComponent->GrantedWeaponItem = Item;
	}
}


void PlaceHeldObject(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	printf("PlaceHeldObject\n");
}

void ThrowHeldObject(UObject* Context, FFrame& Stack)
{
	FVector DetachLocation;
	FRotator ThrowDirection;

	Stack.StepCompiledIn(&DetachLocation);
	Stack.StepCompiledIn(&ThrowDirection);
	Stack.IncrementCode();
	printf("ThrowHeldObject\n");
}

void DropHeldObject(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	printf("DropHeldObject\n");
	auto HeldObjectComponent = (UFortHeldObjectComponent*)Context;

	if (!HeldObjectComponent->OwningPawn)
		return;


	auto PlayerController = (AFortPlayerControllerAthena*)HeldObjectComponent->OwningPawn->Controller;

	/*auto Item = HeldObjectComponent->GrantedWeaponItem.Get();

	if (!Item)HeldObjectComponent->GrantedWeaponItem
		return;*/
		//printf("GUID: %d %d %d %d, ID: %s\n", Item->ItemEntry.ItemGuid.A, Item->ItemEntry.ItemGuid.B, Item->ItemEntry.ItemGuid.C, Item->ItemEntry.ItemGuid.D, Item->ItemEntry.ItemDefinition->Name.ToString().c_str());

		/*PlayerController->WorldInventory->Remove(Item->ItemEntry.ItemGuid);

		HeldObjectComponent->GrantedWeapon = nullptr;
		HeldObjectComponent->GrantedWeaponItem = nullptr;*/

	HeldObjectComponent->HeldObjectState = 4;


	if (HeldObjectComponent->HasOwningPawn())
	{
		auto OldPawn = HeldObjectComponent->OwningPawn;
		printf("%p\n", OldPawn->PreviousWeapon);
		auto PreviousIns = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemGuid == ((AFortWeapon*)OldPawn->PreviousWeapon)->ItemEntryGuid;
			}, FFortItemEntry::Size());

		if (PreviousIns)
		{
			auto Weapon = (AFortWeapon*)OldPawn->EquipWeaponDefinition(PreviousIns->ItemDefinition, PreviousIns->ItemGuid, FFortItemEntry::HasTrackerGuid() ? PreviousIns->TrackerGuid : FGuid(), false);

			PlayerController->ClientEquipItem(Weapon->ItemEntryGuid, true);
		}
	}
	else
	{
		// its a weakobjectptr on older builds
		static auto OwningPawnOff = HeldObjectComponent->GetOffset("OwningPawn", 0x8000000);
		auto& OwningPawn = *(TWeakObjectPtr<AFortPlayerPawnAthena>*)(__int64(Context) + OwningPawnOff);

		auto PreviousIns = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemGuid == ((AFortWeapon*)OwningPawn->PreviousWeapon)->ItemEntryGuid;
			}, FFortItemEntry::Size());

		if (PreviousIns)
		{
			auto Weapon = (AFortWeapon*)OwningPawn->EquipWeaponDefinition(PreviousIns->ItemDefinition, PreviousIns->ItemGuid, FFortItemEntry::HasTrackerGuid() ? PreviousIns->TrackerGuid : FGuid(), false);

			PlayerController->ClientEquipItem(Weapon->ItemEntryGuid, true);
		}
	}

	SetupOwningPawn(HeldObjectComponent, nullptr);
}


void AFortPlayerControllerAthena::SpawnToyInstance(UObject* Context, FFrame& Stack, AActor** Ret)
{
	TSubclassOf<AActor> ToyClass;
	FTransform SpawnPosition;

	Stack.StepCompiledIn(&ToyClass);
	Stack.StepCompiledIn(&SpawnPosition);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto Toy = UWorld::SpawnActor(ToyClass, SpawnPosition, PlayerController);
	PlayerController->ActiveToyInstances.Add(Toy);

	*Ret = Toy;
}

void AFortPlayerControllerAthena::EnterAircraft(UObject* Object, AActor* Aircraft)
{
	AFortPlayerControllerAthena* PlayerController = nullptr;

	static auto bIsComp = Object->IsA(FindClass("FortControllerComponent_Aircraft"));
	if (bIsComp)
		PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Object)->GetOwner();
	else
		PlayerController = (AFortPlayerControllerAthena*)Object;

	UEAllocatedVector<FGuid> GuidsToRemove;
	for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

		if (Entry.ItemDefinition->bCanBeDropped)
		{
			//NewPlayer->WorldInventory->Inventorxy.ReplicatedEntries.Remove(i, FFortItemEntry::Size());
			//i--;
			GuidsToRemove.push_back(Entry.ItemGuid);
		}
	}

	for (auto& Guid : GuidsToRemove)
		PlayerController->WorldInventory->Remove(Guid);

	if (FConfiguration::bLateGame)
	{
		auto Shotgun = LateGame::GetShotgun();
		auto AssaultRifle = LateGame::GetAssaultRifle();
		auto Sniper = LateGame::GetSniper();
		auto Heal = LateGame::GetHeal();
		auto HealSlot2 = LateGame::GetHeal();

		int ShotgunClipSize = Shotgun.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)Shotgun.Item)->ClipSize : 0;
		int AssaultRifleClipSize = AssaultRifle.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)AssaultRifle.Item)->ClipSize : 0;
		int SniperClipSize = Sniper.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)Sniper.Item)->ClipSize : 0;
		// for grappler
		int HealClipSize = Heal.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)Heal.Item)->ClipSize : 0;
		int HealSlot2ClipSize = HealSlot2.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)HealSlot2.Item)->ClipSize : 0;

		PlayerController->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::Wood), 500);
		PlayerController->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::Stone), 500);
		PlayerController->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::Metal), 500);

		PlayerController->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Assault), 250);
		PlayerController->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Shotgun), 50);
		PlayerController->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Submachine), 400);
		PlayerController->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Rocket), 6);
		PlayerController->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Sniper), 20);

		PlayerController->WorldInventory->GiveItem(Shotgun.Item, Shotgun.Count, ShotgunClipSize);
		PlayerController->WorldInventory->GiveItem(AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize);
		PlayerController->WorldInventory->GiveItem(Sniper.Item, Sniper.Count, SniperClipSize);
		PlayerController->WorldInventory->GiveItem(Heal.Item, Heal.Count, HealClipSize);
		PlayerController->WorldInventory->GiveItem(HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize);
	}

	return EnterAircraftOG(Object, Aircraft);
}

void AFortPlayerControllerAthena::PostLoadHook()
{
	if (VersionInfo.FortniteVersion >= 28)
	{
		InitializeBuildingActor_ = FindInitializeBuildingActor();
		PostInitializeSpawnedBuildingActor_ = FindPostInitializeSpawnedBuildingActor();
	}
	CantBuild_ = FindCantBuild();
	ReplaceBuildingActor_ = FindReplaceBuildingActor(); // pre-cache building offsets
	RemoveFromAlivePlayers_ = FindRemoveFromAlivePlayers();
	GiveAbilityAndActivateOnce = FindGiveAbilityAndActivateOnce();
	ClearAbility_ = FindClearAbility();

	auto DefaultFortPC = DefaultObjImpl("FortPlayerController");

	Utils::Hook(FindGetPlayerViewPoint(), GetPlayerViewPoint, GetPlayerViewPointOG);
	// they only stripped it on athena for some reason
	//auto ServerAcknowledgePossessionIdx = GetDefaultObj()->GetFunction("ServerAcknowledgePossession")->GetVTableIndex();
	//Utils::Hook<AFortPlayerControllerAthena>(ServerAcknowledgePossessionIdx, DefaultFortPC->Vft[ServerAcknowledgePossessionIdx]);

	if (VersionInfo.FortniteVersion >= 11)
	{
		auto ServerRestartPlayerIdx = GetDefaultObj()->GetFunction("ServerRestartPlayer")->GetVTableIndex();
		auto DefaultFortPCZone = DefaultObjImpl("FortPlayerControllerZone");
		Utils::Hook<AFortPlayerControllerAthena>(ServerRestartPlayerIdx, DefaultFortPCZone->Vft[ServerRestartPlayerIdx]);
	}

	auto ServerSuicideIdx = GetDefaultObj()->GetFunction("ServerSuicide")->GetVTableIndex();
	auto DefaultFortPCZone = DefaultObjImpl("FortPlayerControllerZone");
	Utils::Hook<AFortPlayerControllerAthena>(ServerSuicideIdx, DefaultFortPCZone->Vft[ServerSuicideIdx]);

	//if (VersionInfo.FortniteVersion >= 11)
	//{
	auto ServerAttemptAircraftJumpPC = GetDefaultObj()->GetFunction("ServerAttemptAircraftJump");
	if (!ServerAttemptAircraftJumpPC)
		Utils::ExecHook(DefaultObjImpl("FortControllerComponent_Aircraft")->GetFunction("ServerAttemptAircraftJump"), ServerAttemptAircraftJump_, ServerAttemptAircraftJump_OG);
	else
		Utils::ExecHook(ServerAttemptAircraftJumpPC, ServerAttemptAircraftJump_);
	//}

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerAcknowledgePossession"), ServerAcknowledgePossession);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryItem"), ServerExecuteInventoryItem_);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryWeapon"), ServerExecuteInventoryWeapon); // S9 shenanigans

	// same as serveracknowledgepossession
	auto ServerReturnToMainMenuIdx = GetDefaultObj()->GetFunction("ServerReturnToMainMenu")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerReturnToMainMenuIdx, DefaultFortPC->Vft[ServerReturnToMainMenuIdx]);

	if (VersionInfo.FortniteVersion != 1.72 && VersionInfo.FortniteVersion != 1.8)
	{
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCreateBuildingActor"), ServerCreateBuildingActor);
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerBeginEditingBuildingActor"), ServerBeginEditingBuildingActor);
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerEditBuildingActor"), ServerEditBuildingActor);
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerEndEditingBuildingActor"), ServerEndEditingBuildingActor);
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerRepairBuildingActor"), ServerRepairBuildingActor);

	}
	auto ServerAttemptInventoryDropFn = GetDefaultObj()->GetFunction("ServerAttemptInventoryDrop");
	if (ServerAttemptInventoryDropFn)
		Utils::ExecHook(ServerAttemptInventoryDropFn, ServerAttemptInventoryDrop);
	else
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerSpawnInventoryDrop"), ServerAttemptInventoryDrop);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerPlayEmoteItem"), ServerPlayEmoteItem);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerPlaySprayItem"), ServerPlayEmoteItem);

	auto ClientOnPawnDiedAddr = FindFunctionCall(L"ClientOnPawnDied", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x48, 0x89, 0x54 } : (VersionInfo.FortniteVersion >= 24 && VersionInfo.FortniteVersion < 25 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C }));
	Utils::Hook(ClientOnPawnDiedAddr, ClientOnPawnDied, ClientOnPawnDiedOG);

	if (VersionInfo.FortniteVersion >= 15)
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerClientIsReadyToRespawn"), ServerClientIsReadyToRespawn);

	if (FConfiguration::bEnableCheats)
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCheat"), ServerCheat);

	auto ServerAttemptInteractPC = GetDefaultObj()->GetFunction("ServerAttemptInteract");
	if (!ServerAttemptInteractPC)
		Utils::ExecHook(DefaultObjImpl("FortControllerComponent_Interaction")->GetFunction("ServerAttemptInteract"), ServerAttemptInteract_, ServerAttemptInteract_OG);
	else
		Utils::ExecHook(ServerAttemptInteractPC, ServerAttemptInteract_, ServerAttemptInteract_OG);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerDropAllItems"), ServerDropAllItems);

	auto DefaultWeaponComp = DefaultObjImpl("FortWeaponComponent");

	if (VersionInfo.FortniteVersion >= 14.00)
	{
		auto OnUnEquipAddr = FindFunctionCall(L"K2_OnUnEquip", std::vector<uint8_t>{ 0x48, 0x89, 0x5C });

		Utils::Hook(OnUnEquipAddr, OnUnEquip, OnUnEquipOG);

		//Utils::ExecHook(DefaultWeaponComp->GetFunction("OnUnEquip"), OnUnEquip);
	}

	auto DefaultHeldObjComp = DefaultObjImpl("FortHeldObjectComponent");

	if (DefaultHeldObjComp)
	{
		Utils::ExecHook(DefaultHeldObjComp->GetFunction("PickupHeldObject"), PickupHeldObject);
		Utils::ExecHook(DefaultHeldObjComp->GetFunction("DropHeldObject"), DropHeldObject);
		Utils::ExecHook(DefaultHeldObjComp->GetFunction("PlaceHeldObject"), PlaceHeldObject);
		Utils::ExecHook(DefaultHeldObjComp->GetFunction("ThrowHeldObject"), ThrowHeldObject);
	}

	Utils::ExecHook(GetDefaultObj()->GetFunction("SpawnToyInstance"), SpawnToyInstance);
	Utils::Hook(FindEnterAircraft(), EnterAircraft, EnterAircraftOG);
}
