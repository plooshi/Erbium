#include "pch.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortGameMode.h"
#include "../Public/FortWeapon.h"
#include "../Public/BuildingSMActor.h"
#include "../Public/FortKismetLibrary.h"
#include "../../Erbium/Public/Configuration.h"
#include "../Public/FortLootPackage.h"
#include "../../Erbium/Public/Events.h"
#include "../../Erbium/Public/LateGame.h"
#include "../Public/BuildingItemCollectorActor.h"
#include "../Public/FortPhysicsPawn.h"
#include "../Public/BattleRoyaleGamePhaseLogic.h"
#include "../../Erbium/Public/GUI.h"
#include "../Public/FortAthenaCreativePortal.h"

void AFortPlayerControllerAthena::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Loc, FRotator& Rot)
{
	// seems like this didnt exist on older builds
	if (auto Pawn = PlayerController->MyFortPawn)
	{
		Loc = Pawn->K2_GetActorLocation();
		Rot = PlayerController->GetControlRotation();
		return;
	}

	if (GetPlayerViewPointOG)
		GetPlayerViewPointOG(PlayerController, Loc, Rot);
	else
	{
		static auto SFName = FName(L"Spectating");
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
				//if (auto TargetPawn = ViewTarget->Cast<AFortPlayerPawnAthena>())
				//	Loc.Z += TargetPawn->BaseEyeHeight;
				Rot = ViewTarget->K2_GetActorRotation();
			}
		}
	}
}

extern uint64_t ApplyCharacterCustomization;
uint64_t InitializePlayerGameplayAbilities_;
void AFortPlayerControllerAthena::ServerAcknowledgePossession(UObject* Context, FFrame& Stack)
{
	AActor* Pawn;
	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	if (!Pawn)
		return;

	auto FortPawn = (AFortPlayerPawnAthena*)Pawn;

	static auto FortPCServerAcknowledgePossession = (void(*)(AFortPlayerControllerAthena*, AActor*))DefaultObjImpl("FortPlayerController")->Vft[Stack.GetCurrentNativeFunction()->GetVTableIndex()];
	FortPCServerAcknowledgePossession(PlayerController, Pawn);

	auto Num = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num();

	auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
	
    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;
	if (Playlist && Playlist->RespawnType > 0 && Num > 0)
	{
		if (FConfiguration::bLateGame)
			FortPawn->SetShield(100.f);
	}
	if ((!FConfiguration::bKeepInventory || FConfiguration::bLateGame) && PlayerController->WorldInventory)
	{	
		UEAllocatedVector<FGuid> GuidsToRemove;
		for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (Entry.ItemDefinition->CanBeDropped())
			{
				//NewPlayer->WorldInventory->Inventorxy.ReplicatedEntries.Remove(i, FFortItemEntry::Size());
				//i--;

				GuidsToRemove.push_back(Entry.ItemGuid);
			}
			if (VersionInfo.FortniteVersion < 3 && Entry.ItemDefinition->ItemType == EFortItemType::GetWeaponHarvest())
			{
				//PlayerController->ServerExecuteInventoryItem(Entry.ItemGuid);
				//PlayerController->QuickBars->ServerActivateSlotInternal(0, 0, 0.f, true);
			}
		}

		for (auto& Guid : GuidsToRemove)
			PlayerController->WorldInventory->Remove(Guid);
	}

	if (VersionInfo.FortniteVersion >= 18)
	{
		if (VersionInfo.FortniteVersion >= 25.20)
		{
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
				auto SpecHandle = AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Effect, 0.f, EffectHandle);

				//AbilitySystemComponent->SetActiveGameplayEffectLevel(SpecHandle, 1);

				AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(SpecHandle,
					FGameplayTag(FName(L"SetByCaller.StormCampingDamage")), 1.f);
			}
		}

		FortPawn->bIsInAnyStorm = false;
		FortPawn->OnRep_IsInAnyStorm();
		FortPawn->bIsInsideSafeZone = true;
		FortPawn->OnRep_IsInsideSafeZone();
	}

	if (VersionInfo.FortniteVersion < 15 && Num > 0 && Playlist && Playlist->RespawnType > 0)
	{
		auto Interface = PlayerController->PlayerState->GetInterface(IFortAbilitySystemInterface::StaticClass());
		if (InitializePlayerGameplayAbilities_ && Interface)
		{
			auto InitializePlayerGameplayAbilities = (void(*&)(const IInterface*))InitializePlayerGameplayAbilities_;

			InitializePlayerGameplayAbilities(Interface);
		}
		else
			for (auto& AbilitySet : AFortGameMode::AbilitySets)
				PlayerController->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);
	}

	if (Num == 0)
	{
		static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");
		static bool HasCosmeticLoadoutPC = PlayerController->HasCosmeticLoadoutPC();
		static bool HasCustomizationLoadout = PlayerController->HasCustomizationLoadout();

		if (HasCosmeticLoadoutPC && PlayerController->CosmeticLoadoutPC.Pickaxe)
			PlayerController->WorldInventory->GiveItem(PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
		else if (HasCustomizationLoadout && PlayerController->CustomizationLoadout.Pickaxe)
			PlayerController->WorldInventory->GiveItem(PlayerController->CustomizationLoadout.Pickaxe->WeaponDefinition);
		else if (HasCosmeticLoadoutPC || HasCustomizationLoadout) // fix ur backend gng
		{
			static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

			PlayerController->WorldInventory->GiveItem(DefaultPickaxe);
		}
		
		if (GameMode->StartingItems.Num() == 0)
		{
			static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
			static auto WallBuild = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Wall.BuildingItemData_Wall");
			static auto FloorBuild = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Floor.BuildingItemData_Floor");
			static auto StairBuild = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Stair_W.BuildingItemData_Stair_W");
			static auto ConeBuild = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_RoofS.BuildingItemData_RoofS");
			static auto EditTool = FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/EditTool.EditTool");

			PlayerController->WorldInventory->GiveItem(DefaultPickaxe);
			PlayerController->WorldInventory->GiveItem(WallBuild);
			PlayerController->WorldInventory->GiveItem(FloorBuild);
			PlayerController->WorldInventory->GiveItem(StairBuild);
			PlayerController->WorldInventory->GiveItem(ConeBuild);
			PlayerController->WorldInventory->GiveItem(EditTool);
		}
		else
			for (int i = 0; i < GameMode->StartingItems.Num(); i++)
			{
				auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

				if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
					PlayerController->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
			}


		/*auto pickaxeEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([](FFortItemEntry& entry)
			{ return entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>(); }, FFortItemEntry::Size());

		if (pickaxeEntry && VersionInfo.FortniteVersion > 3)
		{
			PlayerController->ServerExecuteInventoryItem(pickaxeEntry->ItemGuid);
			PlayerController->ClientEquipItem(pickaxeEntry->ItemGuid, true);
		}*/

		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerController->PlayerState);
		if (!UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization__Ptr && ApplyCharacterCustomization)
		{
			((void (*)(AActor*, AActor*)) ApplyCharacterCustomization)(PlayerController->PlayerState, Pawn);
		}

		auto Interface = PlayerController->PlayerState->GetInterface(IFortAbilitySystemInterface::StaticClass());
		if (InitializePlayerGameplayAbilities_ && Interface)
		{
			auto InitializePlayerGameplayAbilities = (void(*&)(const IInterface*))InitializePlayerGameplayAbilities_;

			InitializePlayerGameplayAbilities(Interface);
		}
		else
			for (auto& AbilitySet : AFortGameMode::AbilitySets)
				PlayerController->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);

		if (FConfiguration::bDisableJumpFatigue && FortPawn->HasCharacterMovement())
		{
			auto MovementCompAthena = (UFortMovementComp_CharacterAthena*)(FortPawn->GetCharacterMovement());
			if (MovementCompAthena->HasJumpPenaltyResetTime())
				MovementCompAthena->JumpPenaltyResetTime = 0.0f;
		}
	}
	else if (FConfiguration::bLateGame && (!FConfiguration::bKeepInventory || FConfiguration::bLateGame))
	{
		auto Shotgun = LateGame::GetShotgun();
		auto AssaultRifle = LateGame::GetAssaultRifle();
		auto Sniper = LateGame::GetSniper();
		auto Heal = LateGame::GetHeal();
		auto HealSlot2 = LateGame::GetHeal();

		int ShotgunClipSize = 0;
		int AssaultRifleClipSize = 0;
		int SniperClipSize = 0;
		int HealClipSize = 0;
		int HealSlot2ClipSize = 0;
		
		if (auto Weapon = Shotgun.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Shotgun.Item)->GetWeaponItemDefinition() : Shotgun.Item->Cast<UFortWeaponItemDefinition>())
			ShotgunClipSize = AFortInventory::GetStats(Weapon)->ClipSize;
		if (auto Weapon = AssaultRifle.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)AssaultRifle.Item)->GetWeaponItemDefinition() : AssaultRifle.Item->Cast<UFortWeaponItemDefinition>())
			AssaultRifleClipSize = AFortInventory::GetStats(Weapon)->ClipSize;
		if (auto Weapon = Sniper.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Sniper.Item)->GetWeaponItemDefinition() : Sniper.Item->Cast<UFortWeaponItemDefinition>())
			SniperClipSize = AFortInventory::GetStats(Weapon)->ClipSize;
		if (auto Weapon = Heal.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Heal.Item)->GetWeaponItemDefinition() : Heal.Item->Cast<UFortWeaponItemDefinition>())
			HealClipSize = AFortInventory::GetStats(Weapon)->ClipSize;
		if (auto Weapon = HealSlot2.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)HealSlot2.Item)->GetWeaponItemDefinition() : HealSlot2.Item->Cast<UFortWeaponItemDefinition>())
			HealSlot2ClipSize = AFortInventory::GetStats(Weapon)->ClipSize;

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
}

uint32 ServerAttemptAircraftJumpVft;
void AFortPlayerControllerAthena::ServerAttemptAircraftJump_(UObject* Context, FFrame& Stack)
{
	FRotator Rotation;
	Stack.StepCompiledIn(&Rotation);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = nullptr;
	auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	if (VersionInfo.FortniteVersion >= 11.00 || FConfiguration::bLateGame)
	{
		static auto bIsComp = Context->IsA(FindClass("FortControllerComponent_Aircraft"));
		if (bIsComp)
			PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
		else
			PlayerController = (AFortPlayerControllerAthena*)Context;

		GameMode->RestartPlayer(PlayerController);
		PlayerController->ClientSetRotation(Rotation, true);

		if (PlayerController->MyFortPawn)
		{
			//PlayerController->MyFortPawn->BeginSkydiving(true);
			//PlayerController->MyFortPawn->SetHealth(100.f);
		}
	}
	else
	{
		static auto ServerAttemptAircraftJumpOG = (void(*)(AFortPlayerControllerAthena*, FRotator&)) ((AFortPlayerControllerAthena*)Context)->Vft[ServerAttemptAircraftJumpVft];

		ServerAttemptAircraftJumpOG((AFortPlayerControllerAthena*)Context, Rotation);
	}
	
	if (FConfiguration::bLateGame)
	{
		PlayerController->MyFortPawn->SetShield(100.f);
		auto Aircraft = GameState->HasAircrafts() ? GameState->Aircrafts[0] : (GameState->HasAircraft() ? GameState->Aircraft : nullptr);
		if (!Aircraft) // gamephaselogic builds
		{
			auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

			Aircraft = GamePhaseLogic->Aircrafts_GameState[0].Get();
		}

		FVector AircraftLocation = Aircraft->K2_GetActorLocation();

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
	if (FConfiguration::bDisableJumpFatigue && PlayerController->MyFortPawn->HasCharacterMovement())
	{
		auto MovementCompAthena = (UFortMovementComp_CharacterAthena*)(PlayerController->MyFortPawn->GetCharacterMovement());
		if (MovementCompAthena->HasJumpPenaltyResetTime())
			MovementCompAthena->JumpPenaltyResetTime = 0.0f;
	}
}

void AFortPlayerControllerAthena::ServerExecuteInventoryItem_(UObject* Context, FFrame& Stack)
{
	FGuid ItemGuid;
	Stack.StepCompiledIn(&ItemGuid);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemGuid == ItemGuid;
		}, FFortItemEntry::Size());

	if (!entry)
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*)entry->ItemDefinition;

	if (auto Gadget = ItemDefinition->Cast<UFortGadgetItemDefinition>())
		ItemDefinition = Gadget->GetWeaponItemDefinition();
	else if (ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass()))
	{
		PlayerController->MyFortPawn->PickUpActor(nullptr, ItemDefinition);
		((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ItemEntryGuid = ItemGuid;

		static auto ContextTrapClass = FindClass("FortDecoTool_ContextTrap");
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

	if (auto DecoTool = Weapon->Cast<AFortDecoTool>())
	{
		DecoTool->ItemDefinition = ItemDefinition;

		if (auto ContextTrapTool = Weapon->Cast<AFortDecoTool_ContextTrap>())
			ContextTrapTool->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)ItemDefinition;
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

	if (ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass()))
	{
		auto DecoTool = (AFortDecoTool*)Weapon;
		DecoTool->ItemDefinition = ItemDefinition;

		if (auto ContextTrapTool = Weapon->Cast<AFortDecoTool_ContextTrap>())
			ContextTrapTool->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)ItemDefinition;
	}
}

bool CanBePlacedByPlayer(TSubclassOf<AActor> BuildClass)
{
	return ((ABuildingSMActor*)BuildClass->GetDefaultObj())->bIsPlayerBuildable;
	/*auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	static auto HasAllPlayerBuildableClasses = GameState->HasAllPlayerBuildableClasses();
	return HasAllPlayerBuildableClasses ? GameState->AllPlayerBuildableClasses.Search([BuildClass](TSubclassOf<AActor> Class)
		{ return Class == BuildClass; }) != 0 : true;*/
}

uint64_t CantBuild_ = 0;
uint64_t CanAffordToPlaceBuildableClass_;
uint64_t PayBuildableClassPlacementCost_;
uint64_t CanPlaceBuildableClassInStructuralGrid_;
void AFortPlayerControllerAthena::ServerCreateBuildingActor(UObject* Context, FFrame& Stack)
{
	TSubclassOf<AActor> BuildingClass;
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

	FBuildingClassData BuildingClassData;
	if (VersionInfo.FortniteVersion >= 8.30)
	{
		struct FCreateBuildingActorData { uint32_t BuildingClassHandle; _Pad_0xC BuildLoc; _Pad_0xC BuildRot; bool bMirrored; uint8_t Pad_1[0x3]; float SyncKey; uint8 Pad_2[0x4]; FBuildingClassData BuildingClassData; };
		struct FCreateBuildingActorData_New { uint32_t BuildingClassHandle; uint8_t Pad_1[0x4]; _Pad_0x18 BuildLoc; _Pad_0x18 BuildRot; bool bMirrored; uint8_t Pad_2[0x3]; float SyncKey; FBuildingClassData BuildingClassData; };

		if (VersionInfo.FortniteVersion >= 20.00)
		{
			FCreateBuildingActorData_New CreateBuildingData;
			Stack.StepCompiledIn(&CreateBuildingData);

			BuildLoc = *(FVector*)&CreateBuildingData.BuildLoc;
			BuildRot = *(FRotator*)&CreateBuildingData.BuildRot;
			bMirrored = CreateBuildingData.bMirrored;
			BuildingClassData = CreateBuildingData.BuildingClassData;

			BuildingClass = AFortGameStateAthena::BuildingClassMap[CreateBuildingData.BuildingClassHandle];
			if (!BuildingClass)
			{
				Stack.IncrementCode();
				return;
			}
		}
		else
		{
			FCreateBuildingActorData CreateBuildingData;
			Stack.StepCompiledIn(&CreateBuildingData);

			BuildLoc = *(FVector*)&CreateBuildingData.BuildLoc;
			BuildRot = *(FRotator*)&CreateBuildingData.BuildRot;
			bMirrored = CreateBuildingData.bMirrored;
			BuildingClassData = CreateBuildingData.BuildingClassData;

			BuildingClass = AFortGameStateAthena::BuildingClassMap[CreateBuildingData.BuildingClassHandle];
			if (!BuildingClass)
			{
				Stack.IncrementCode();
				return;
			}
		}
		BuildingClassData.BuildingClass = BuildingClass;
	}
	else
	{
		Stack.StepCompiledIn(&BuildingClassData);
		Stack.StepCompiledIn(&BuildLoc);
		Stack.StepCompiledIn(&BuildRot);
		Stack.StepCompiledIn(&bMirrored);

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
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

	TArray<ABuildingSMActor*> RemoveBuildings;
	if (VersionInfo.FortniteVersion >= 27)
	{
		char _Unk_OutVar1;
		auto CantBuild = (__int64 (*)(UWorld*, TSubclassOf<AActor>&, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;

		if (CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildLoc, *(_Pad_0x18*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1))
			return;
	}
	else
	{
		char _Unk_OutVar1;
		auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
		auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;

		if (VersionInfo.FortniteVersion >= 20.00 ? CantBuildNew(UWorld::GetWorld(), BuildingClass, *(_Pad_0x18*)&BuildLoc, *(_Pad_0x18*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1) : CantBuild(UWorld::GetWorld(), BuildingClass, *(_Pad_0xC*)&BuildLoc, *(_Pad_0xC*)&BuildRot, bMirrored, &RemoveBuildings, &_Unk_OutVar1))
			return;
	}	

	for (auto& RemoveBuilding : RemoveBuildings)
		RemoveBuilding->K2_DestroyActor();
	RemoveBuildings.Free();

	static auto K2_SpawnBuildingActor = ABuildingSMActor::GetDefaultObj()->GetFunction("K2_SpawnBuildingActor");

	ABuildingSMActor* Building = nullptr;
	/*if (K2_SpawnBuildingActor)
	{
		FTransform SpawnTransform(BuildLoc, BuildRot);
		Building = ABuildingSMActor::K2_SpawnBuildingActor(PlayerController, BuildingClass, SpawnTransform, PlayerController, nullptr, false, false);
	}
	else*/
		Building = UWorld::SpawnActorUnfinished<ABuildingSMActor>(BuildingClass, BuildLoc, BuildRot, PlayerController);

	if (!Building)
		return;

	 static auto UpgradeLevelOffset = FBuildingClassData::StaticStruct()->GetOffset("UpgradeLevel");
	Building->CurrentBuildingLevel = VersionInfo.EngineVersion >= 5.3 ? *(uint8*)(__int64(&BuildingClassData) + UpgradeLevelOffset) : *(uint32*)(__int64(&BuildingClassData) + UpgradeLevelOffset);
	Building->OnRep_CurrentBuildingLevel();

	Building->SetMirrored(bMirrored);

	Building->bPlayerPlaced = true;

	if (((AFortPlayerStateAthena*)PlayerController->PlayerState)->HasTeamIndex())
		Building->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;

	if (Building->HasTeamIndex())
		Building->TeamIndex = Building->Team;

	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr, false);
	UWorld::FinishSpawnActor(Building, BuildLoc, BuildRot);

	if (!PlayerController->bBuildFree && !FConfiguration::bInfiniteMats)
	{
		auto PayBuildableClassPlacementCost = (int(*)(AFortPlayerControllerAthena*, FBuildingClassData)) PayBuildableClassPlacementCost_;

		PayBuildableClassPlacementCost(PlayerController, BuildingClassData);
	}
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
					_this->OnRep_EditingPlayer();
					return;
				}
		}
		else
		{
			if (!NewEditingPlayer)
			{
				_this->EditingPlayer = NewEditingPlayer;
				_this->OnRep_EditingPlayer();
				return;
			}

			auto Handle = NewEditingPlayer->Owner;

			if (auto PlayerController = Handle->Cast<AFortPlayerControllerAthena>())
			{
				_this->EditingPlayer = NewEditingPlayer;
				_this->OnRep_EditingPlayer();
			}
		}
	}
}

void AFortPlayerControllerAthena::ServerBeginEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building/* || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex*/)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
	if (!PlayerState)
		return;
	
	SetEditingPlayer(Building, PlayerState);
	
	if (!PlayerController->MyFortPawn->CurrentWeapon->IsA<AFortWeap_EditingTool>())
	{
		auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
			}, FFortItemEntry::Size());

		PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);
	}

	if (auto EditTool = PlayerController->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
	{
		EditTool->EditActor = Building;
		EditTool->ForceNetUpdate();
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

	if (!PlayerController || !Building || !NewClass || !Building->IsA<ABuildingSMActor>() || !CanBePlacedByPlayer(NewClass) || Building->EditingPlayer != PlayerController->PlayerState || Building->bDestroyed)
	{
		return;
	}

	SetEditingPlayer(Building, nullptr);

	auto ReplaceBuildingActor = (ABuildingSMActor * (*&)(ABuildingSMActor*, unsigned int, TSubclassOf<AActor>, unsigned int, int, bool, AFortPlayerControllerAthena*)) ReplaceBuildingActor_;
	auto ReplaceBuildingActor__New = (ABuildingSMActor * (*&)(ABuildingSMActor*, unsigned int, TSubclassOf<AActor>&, unsigned int, int, bool, AFortPlayerControllerAthena*)) ReplaceBuildingActor_;

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
}

void AFortPlayerControllerAthena::ServerEndEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != PlayerController->PlayerState/* || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex*/ || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	/*if (VersionInfo.EngineVersion >= 4.24)
	{
		auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{
				return entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
			}, FFortItemEntry::Size());

		PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);
	}*/

	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
		}, FFortItemEntry::Size());

	if (!EditToolEntry)
		return;

	auto EditToolPtr = PlayerController->Pawn->CurrentWeaponList.Search([&](AActor* Weapon__Uncasted)
		{ return ((AFortWeapon*)Weapon__Uncasted)->ItemEntryGuid == EditToolEntry->ItemGuid; });

	if (!EditToolPtr)
		return;

	if (auto EditTool = *(AFortWeap_EditingTool**)EditToolPtr)
	{
		EditTool->EditActor = nullptr;
		EditTool->ForceNetUpdate();
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
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([res](FFortItemEntry& entry)
		{
			return entry.ItemDefinition == res;
		}, FFortItemEntry::Size());
	if (!ItemP)
		return;
	auto Item = *ItemP;
	if ((itemEntry->Count - Price) < 0)
		return;

	itemEntry->Count -= Price;
	if (itemEntry->Count <= 0)
		PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
	else
	{
		Item->ItemEntry.Count = itemEntry->Count;
		PlayerController->WorldInventory->UpdateEntry(*itemEntry);
		Item->ItemEntry.bIsDirty = true;
	}

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
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == Guid; }, FFortItemEntry::Size());
	if (!ItemP)
		return;
	auto Item = *ItemP;

	itemEntry->Count -= Count;

	FVector FinalLoc = PlayerController->Pawn->K2_GetActorLocation();

	static auto WID_Launcher_Petrol = FindObject<UFortWorldItemDefinition>(L"/Game/Athena/Items/Weapons/Prototype/WID_Launcher_Petrol.WID_Launcher_Petrol");

	if (itemEntry->ItemDefinition == WID_Launcher_Petrol)
	{
		static auto BGA_Petrol_PickupClass = FindObject<UClass>(L"/Game/Athena/Items/Weapons/Prototype/PetrolPump/BGA_Petrol_Pickup.BGA_Petrol_Pickup_C");

		AActor* PetrolPickup = UWorld::SpawnActor<AActor>(BGA_Petrol_PickupClass, FinalLoc);

		PlayerController->WorldInventory->Remove(Guid);
		PlayerController->WorldInventory->Update(itemEntry);

		return;
	}

	FVector ForwardVector = PlayerController->Pawn->GetActorForwardVector();
	//ForwardVector.Z = 0.0f;
	//ForwardVector.Normalize();

	FinalLoc = FinalLoc + ForwardVector * 450.f;
	FinalLoc.Z += 50.f;

	const float RandomAngleVariation = ((float)rand() * 0.00109866634f) - 18.f;
	const float FinalAngle = (RandomAngleVariation + 360.f / ((float)rand() * 0.00015259254737998596f)) * 0.017453292519943295f;

	FinalLoc.X += cos(FinalAngle) * 100.f;
	FinalLoc.Y += sin(FinalAngle) * 100.f;

	AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *itemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn, Count, true, true, true, nullptr, FinalLoc);
	if (itemEntry->Count <= 0 || Count < 0)
		PlayerController->WorldInventory->Remove(Guid);
	else
	{
		Item->ItemEntry.Count = itemEntry->Count;
		PlayerController->WorldInventory->UpdateEntry(*itemEntry);
		Item->ItemEntry.bIsDirty = true;
	}
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

	auto AbilitySystemComponent = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->AbilitySystemComponent;

	if (auto CharacterVehicle = PlayerController->Pawn->Cast<AFortCharacterVehicle>())
		AbilitySystemComponent = CharacterVehicle->OverrideAbilitySystemComponent;
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

		auto CustomAbility = DanceAsset->HasCustomDanceAbility() ? DanceAsset->CustomDanceAbility.Get() : nullptr;

		if (CustomAbility)
			AbilityToUse = CustomAbility->GetDefaultObj();
		else
		{
			static auto EmoteAbilityClass = FindObject<UClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
			AbilityToUse = EmoteAbilityClass->GetDefaultObj();
		}
	}

	if (AbilityToUse)
	{
		auto Spec = (FGameplayAbilitySpec*)malloc(FGameplayAbilitySpec::Size());
		memset(PBYTE(Spec), 0, FGameplayAbilitySpec::Size());

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

		free(Spec);

		if (PlayerController->MyFortPawn->HasLastReplicatedEmoteExecuted())
		{
			auto OldEmote = PlayerController->MyFortPawn->LastReplicatedEmoteExecuted;
			PlayerController->MyFortPawn->LastReplicatedEmoteExecuted = Asset;
			PlayerController->MyFortPawn->OnRep_LastReplicatedEmoteExecuted(OldEmote);
		}
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
	auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

	if (PlayerController->WorldInventory && PlayerController->Pawn && ((PlayerController->Pawn->HasbShouldDropItemsOnDeath() ? PlayerController->Pawn->bShouldDropItemsOnDeath : true) && !FConfiguration::bKeepInventory))
	{
		bool bHasMats = false;
		for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (entry.ItemDefinition->CanBeDropped())
				AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation(), entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetPlayerElimination(), PlayerController->MyFortPawn);
		}
	}

	auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
	auto KillerPlayerController = KillerPlayerState ? (AFortPlayerControllerAthena*)KillerPlayerState->Owner : nullptr;

	if (VersionInfo.FortniteVersion > 1.8 || VersionInfo.EngineVersion >= 4.19)
	{
		if (PlayerState->HasPawnDeathLocation())
			PlayerState->PawnDeathLocation = PlayerController->Pawn ? PlayerController->Pawn->K2_GetActorLocation() : FVector();

		if (PlayerState->HasDeathInfo())
		{
			memset(&PlayerState->DeathInfo, 0, FDeathInfo::Size());
			PlayerState->DeathInfo.bDBNO = PlayerController->Pawn ? PlayerController->Pawn->IsDBNO() : false;
			if (FDeathInfo::HasKiller())
				PlayerState->DeathInfo.Killer = KillerPlayerState;
			if (FDeathInfo::HasDeathLocation())
				PlayerState->DeathInfo.DeathLocation = PlayerState->HasPawnDeathLocation() ? PlayerState->PawnDeathLocation : (PlayerController->Pawn ? PlayerController->Pawn->K2_GetActorLocation() : FVector());
			if (FDeathInfo::HasDeathTags())
				PlayerState->DeathInfo.DeathTags = /*DeathReport.Tags*/ PlayerController->Pawn ? *(FGameplayTagContainer*)(__int64(&PlayerController->Pawn->MoveSoundStimulusBroadcastInterval) + (VersionInfo.FortniteVersion >= 11 && VersionInfo.FortniteVersion < 18 ? 0x18 : 0x10)) : FGameplayTagContainer();
			if (FDeathInfo::HasDeathClassSlot())
				PlayerState->DeathInfo.DeathClassSlot = -1;
			PlayerState->DeathInfo.DeathCause = ToDeathCause(PlayerController->Pawn, DeathReport.Tags, PlayerState->DeathInfo.bDBNO);
			//PlayerState->DeathInfo.Downer = KillerPlayerState;
			if (FDeathInfo::HasFinisherOrDowner())
				PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
			if (FDeathInfo::HasFinisherOrDownerTags())
				PlayerState->DeathInfo.FinisherOrDownerTags = KillerPawn ? KillerPawn->GameplayTags : PlayerController->Pawn->GameplayTags;
			if (FDeathInfo::HasVictimTags())
				PlayerState->DeathInfo.VictimTags = PlayerController->Pawn->GameplayTags;
			if (FDeathInfo::HasDistance())
				PlayerState->DeathInfo.Distance = PlayerController->Pawn ? (PlayerState->DeathInfo.DeathCause != /*EDeathCause::FallDamage*/ 1 ? (KillerPawn ? KillerPawn->GetDistanceTo(PlayerController->Pawn) : 0) : (PlayerController->MyFortPawn->HasLastFallDistance() ? PlayerController->MyFortPawn->LastFallDistance : 0)) : 0;
			if (FDeathInfo::HasbInitialized())
				PlayerState->DeathInfo.bInitialized = true;
			PlayerState->OnRep_DeathInfo();
		}

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

			for (auto& Damager : PlayerController->Pawn->Damagers)
			{
				if (Damager.DamageCauser != KillerPlayerController && Damager.DamageCauser->IsA<AFortPlayerControllerAthena>())
				{
					FGameplayTagContainer TargetTags{};
					auto DamagerController = (AFortPlayerControllerAthena*)Damager.DamageCauser;

					auto Interface = (IGameplayTagAssetInterface*)PlayerController->Pawn->GetInterface(IGameplayTagAssetInterface::StaticClass());
					if (Interface)
					{
						auto GetOwnedGameplayTags = (void(*)(IGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->Vft[0x2];
						GetOwnedGameplayTags(Interface, &TargetTags);
						//Interface->GetOwnedGameplayTags(&TargetTags);
					}

					DamagerController->GetQuestManager(1)->SendStatEvent(DamagerController, EFortQuestObjectiveStatEvent::GetKillContribution(), 1, PlayerController->Pawn, TargetTags);

					TargetTags.GameplayTags.Free();
					TargetTags.ParentTags.Free();
				}
			}

			FGameplayTagContainer TargetTags{};

			auto Interface = (IGameplayTagAssetInterface*)PlayerController->Pawn->GetInterface(IGameplayTagAssetInterface::StaticClass());
			if (Interface)
			{
				auto GetOwnedGameplayTags = (void(*)(IGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->Vft[0x2];
				GetOwnedGameplayTags(Interface, &TargetTags);
				//Interface->GetOwnedGameplayTags(&TargetTags);
			}

			KillerPlayerController->GetQuestManager(1)->SendStatEvent(KillerPlayerController, EFortQuestObjectiveStatEvent::GetKill(), 1, PlayerController->Pawn, TargetTags);

			TargetTags.GameplayTags.Free();
			TargetTags.ParentTags.Free();
		}

		static auto IsRespawningAllowedFunc = GameState->GetFunction("IsRespawningAllowed");

		bool bRespawnAllowed = false;

		if (!IsRespawningAllowedFunc)
		{
			auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;

			// respawn except storm needs to be fixed
			bRespawnAllowed = Playlist ? Playlist->RespawnType > 0 : false;
		}
		else
			bRespawnAllowed = GameState->Call<bool>(IsRespawningAllowedFunc, PlayerState);

		if (!bRespawnAllowed && (PlayerController->Pawn ? !PlayerController->Pawn->IsDBNO() : true) && PlayerState->HasPlace())
		{
			PlayerState->Place = GameState->PlayersLeft;
			PlayerState->OnRep_Place();

			AFortWeapon* DamageCauser = nullptr;
			static auto ProjectileBaseClass = FindClass("FortProjectileBase");
			if (DeathReport.DamageCauser ? DeathReport.DamageCauser->IsA(ProjectileBaseClass) : false)
			{
				auto Owner = DeathReport.DamageCauser->Owner;

				if (Owner->Cast<AFortWeapon>())
					DamageCauser = (AFortWeapon*)Owner;
				else if (auto Controller = Owner->Cast<AFortPlayerControllerAthena>())
					DamageCauser = (AFortWeapon*)Controller->Pawn->CurrentWeapon;
				else if (auto Pawn = Owner->Cast<AFortPlayerPawnAthena>())
					DamageCauser = (AFortWeapon*)Pawn->CurrentWeapon;
			}
			else if (auto Weapon = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortWeapon>() : nullptr)
				DamageCauser = Weapon;
			if (RemoveFromAlivePlayers_)
			{
				((void (*)(AFortGameMode*, AFortPlayerControllerAthena*, AFortPlayerStateAthena*, AFortPlayerPawnAthena*, UFortItemDefinition*, uint8, char))RemoveFromAlivePlayers_)(GameMode, PlayerController, KillerPlayerState == PlayerState ? nullptr : KillerPlayerState, KillerPawn, DamageCauser->IsA<AFortWeapon>() ? DamageCauser->WeaponData : nullptr, PlayerState->HasDeathInfo() ? PlayerState->DeathInfo.DeathCause : 0, 0);
			}

			if (VersionInfo.FortniteVersion >= 15)
			{
				//static auto SpectatingName = FName(L"Spectating");
				//PlayerController->StateName = SpectatingName;
				//PlayerController->ClientGotoState(SpectatingName);
				PlayerController->Pawn->CharacterMovement->ProcessEvent(PlayerController->Pawn->CharacterMovement->GetFunction("DisableMovement"), nullptr);
			}

			if (PlayerController->Pawn && KillerPlayerState && KillerPlayerState != PlayerState && KillerPlayerState->Place == 1)
			{
				/*if (PlayerState->Place == 1)
				{
					KillerPlayerState = PlayerState;
					KillerPawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
				}*/

				auto KillerWeapon = DamageCauser ? DamageCauser->WeaponData : nullptr;


				if (VersionInfo.FortniteVersion >= 16)
				{
					KillerPlayerController->PlayWinEffects(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause, false);
					KillerPlayerController->ClientNotifyWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
					KillerPlayerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
				}

				if (KillerPlayerState != PlayerState && VersionInfo.FortniteVersion >= 19)
				{
					auto Crown = FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");

					TArray<FFortItemEntryStateValue> StateValues{};
					auto Value = (FFortItemEntryStateValue*)malloc(FFortItemEntryStateValue::Size());
					memset((PBYTE)Value, 0, FFortItemEntryStateValue::Size());

					Value->IntValue = 1;
					Value->StateType = 2;
					StateValues.Add(*Value, FFortItemEntryStateValue::Size());

					free(Value);
					KillerPlayerController->WorldInventory->GiveItem(Crown, 1, 0, 0, true, true, 0, StateValues);
					StateValues.Free();
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

		if (FConfiguration::SiphonAmount > 0 && PlayerController->Pawn && KillerPlayerState && KillerPlayerState->AbilitySystemComponent && KillerPawn && KillerPawn->Controller != PlayerController)
		{
			auto Handle = KillerPlayerState->AbilitySystemComponent->MakeEffectContext();
			FGameplayTag Tag;
			static auto Cue = FName(L"GameplayCue.Shield.PotionConsumed");
			Tag.TagName = Cue;
			auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
			memset((PBYTE)PredictionKey, 0, FPredictionKey::Size());
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
	}

	if (VersionInfo.FortniteVersion < 6)
	{
		if (GameState->GamePhase > 2)
		{
			if (GameMode->bAllowSpectateAfterDeath)
			{
				PlayerController->PlayerToSpectateOnDeath = KillerPawn ? KillerPawn : (GameMode->AlivePlayers.Num() > 0 ? ((AFortPlayerControllerAthena*)GameMode->AlivePlayers[rand() % GameMode->AlivePlayers.Num()])->Pawn : nullptr);
				
				UKismetSystemLibrary::K2_SetTimer(PlayerController, FString(L"SpectateOnDeath"), 5.f, false);
			}
		}
	}

	return ClientOnPawnDiedOG(PlayerController, DeathReport);
}

void AFortPlayerControllerAthena::ServerClientIsReadyToRespawn(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
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


		if (FConfiguration::bDisableJumpFatigue && NewPawn->HasCharacterMovement())
		{
			auto MovementCompAthena = (UFortMovementComp_CharacterAthena*)(NewPawn->GetCharacterMovement());
			if (MovementCompAthena->HasJumpPenaltyResetTime())
				MovementCompAthena->JumpPenaltyResetTime = 0.0f;
		}

		auto Interface = PlayerController->PlayerState->GetInterface(IFortAbilitySystemInterface::StaticClass());
		if (InitializePlayerGameplayAbilities_ && Interface)
		{
			auto InitializePlayerGameplayAbilities = (void(*&)(const IInterface*))InitializePlayerGameplayAbilities_;

			InitializePlayerGameplayAbilities(Interface);
		}
		else
			for (auto& AbilitySet : AFortGameMode::AbilitySets)
				PlayerController->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);
	}

	PlayerState->RespawnData.bClientIsReady = true;
}

struct FCustomCharacterParts
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FCustomCharacterParts);
};

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
	FVector FinalLoc = Pawn ? Pawn->K2_GetActorLocation() : FVector();

	FVector ForwardVector = Pawn ? Pawn->GetActorForwardVector() : FVector();
	ForwardVector.Z = 0.0f;
	ForwardVector.Normalize();

	FinalLoc = FinalLoc + ForwardVector * 450.f;
	FinalLoc.Z += 50.f;

	const float RandomAngleVariation = ((float)rand() * 0.00109866634f) - 18.f;
	const float FinalAngle = RandomAngleVariation * 0.017453292519943295f;

	FinalLoc.X += cos(FinalAngle) * 100.f;
	FinalLoc.Y += sin(FinalAngle) * 100.f;

	auto GiveOrSwap = [&]()
		{
			if (ItemCount >= 5 && AFortInventory::IsPrimaryQuickbar(PickupEntry->ItemDefinition))
			{
				if (!MyFortPawn || !MyFortPawn->CurrentWeapon)
					return;
				
				if (AFortInventory::IsPrimaryQuickbar(((AFortWeapon*)MyFortPawn->CurrentWeapon)->WeaponData))
				{
					auto itemEntry = WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
						{ return entry.ItemGuid == ((AFortWeapon*)MyFortPawn->CurrentWeapon)->ItemEntryGuid; }, FFortItemEntry::Size());


					AFortInventory::SpawnPickup(Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *itemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, -1, true, true, true, nullptr, FinalLoc);
					WorldInventory->Remove(((AFortWeapon*)MyFortPawn->CurrentWeapon)->ItemEntryGuid);
					auto Item = WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
					ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
					if (VersionInfo.FortniteVersion < 3)
					{
						auto& QuickBar = (AFortInventory::IsPrimaryQuickbar(Item->ItemEntry.ItemDefinition) || Item->ItemEntry.ItemDefinition->ItemType == EFortItemType::GetWeaponHarvest()) ? QuickBars->PrimaryQuickBar : QuickBars->SecondaryQuickBar;
						int i = 0;
						for (i = 0; i < QuickBar.Slots.Num(); i++)
						{
							auto& Slot = QuickBar.Slots.Get(i, FQuickBarSlot::Size());

							for (auto& SlotItem : Slot.Items)
								if (SlotItem == Item->ItemEntry.ItemGuid)
								{
									QuickBars->ServerActivateSlotInternal(!(AFortInventory::IsPrimaryQuickbar(Item->ItemEntry.ItemDefinition) || Item->ItemEntry.ItemDefinition->ItemType == EFortItemType::GetWeaponHarvest()), i, 0.f, true);
									break;
								}
						}
					}
					else
						ClientEquipItem(Item->ItemEntry.ItemGuid, true);
				}
				else
					AFortInventory::SpawnPickup(Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, -1, true, true, true, nullptr, FinalLoc);
			}
			else
				WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
		};

	auto GiveOrSwapStack = [&](int32 OriginalCount)
		{
			if (PickupEntry->ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
				WorldInventory->GiveItem(*PickupEntry, OriginalCount - MaxStack, true);
			else
				AFortInventory::SpawnPickup(Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, OriginalCount - MaxStack, true, true, true, nullptr, FinalLoc);
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

			if ((itemEntry->Count += PickupEntry->Count) > MaxStack)
			{
				auto OriginalCount = itemEntry->Count;
				itemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			// full proper
			for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;
				
				StateValue.IntValue = 1;
				bFound = true;
				break;
			}

			if (!bFound)
			{
				auto Value = (FFortItemEntryStateValue*)malloc(FFortItemEntryStateValue::Size());
				memset((PBYTE)Value, 0, FFortItemEntryStateValue::Size());

				Value->IntValue = 1;
				Value->StateType = 2;
				itemEntry->StateValues.Add(*Value, FFortItemEntryStateValue::Size());

				free(Value);
			}



			(*item)->ItemEntry.Count = itemEntry->Count;
			WorldInventory->UpdateEntry(*itemEntry);
		}
		else
		{
			auto itemEntry2 = WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
				{ return entry.ItemDefinition == PickupEntry->ItemDefinition && entry.Count >= MaxStack; }, FFortItemEntry::Size());

			if (!itemEntry && itemEntry2 && !PickupEntry->ItemDefinition->bAllowMultipleStacks)
			{
				AFortInventory::SpawnPickup(Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, PickupEntry->Count, true, true, true, nullptr, FinalLoc);
				return;
			}

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

std::map<std::string, std::vector<FVector>> Waypoints;

extern uint64_t ApplyCharacterCustomization;
extern uint64_t NotifyGameMemberAdded_;

int32 PlayerBotID = 0;
void AFortPlayerControllerAthena::ServerCheat(UObject* Context, FFrame& Stack)
{
	FString Msg;
	Stack.StepCompiledIn(&Msg);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;

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
    cheat resumesafezone - Resumes the storm
    cheat pausesafezone - Pauses the storm
    cheat skipsafezone - Skips to the next safe zone
    cheat startshrinksafezone - Starts shrinking the safe zone
    cheat infiniteammo - Toggles infinite ammo
    cheat infinitemats - Toggles infinite materials
    cheat gravity <Scale> - Sets the gravity scale
    cheat changename <Name> - Changes your player name
    cheat keepinventory - Toggles keeping inventory on death
    cheat spawnactor <class/path> - Spawns an actor near your location
    cheat sethealth <amount> - Sets your pawn's health (0-100)
    cheat setshield <amount> - Sets your pawn's shield (0-100)
    cheat demospeed <Speed> - Sets the speed of the server
    cheat god - Toggles god mode
    cheat speed <Speed> - Sets the player's movement speed
    cheat timeofday <Hour> - Sets the time of day (0-23)
	cheat pausetimeofday - Pauses/Unpauses the time of day
    cheat spawnbot - Spawns a player bot at your location (WIP)
    cheat startevent - Starts the event for the current version
    cheat tp <X> <Y> <Z> - Teleports to a location
    cheat launch <X> <Y> <Z> - Launches the player
    cheat savewaypoint - Saves your current location as a waypoint
    cheat waypoint <Name> - Loads a saved waypoint
    cheat skydive - Toggles skydiving
    cheat giveitem <WID/path> <Count = 1> - Gives you an item
    cheat spawnpickup <WID/path> <Count = 1> - Spawns a pickup at your player's location
    cheat spawnactor <class/path> - Spawns an actor at your location + 5 meters)"), FName(), 1);
	}
	else
	{
		auto& command = args[0];
		std::transform(command.begin(), command.end(), command.begin(), tolower);

		if (command == "startaircraft")
		{
			if (UFortGameStateComponent_BattleRoyaleGamePhaseLogic::GetDefaultObj())
			{
				auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

				GamePhaseLogic->StartAircraftPhase();
				PlayerController->ClientMessage(FString(L"Started the aircraft!"), FName(), 1.f);
			}
			else
			{
				UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startaircraft"), nullptr);
				PlayerController->ClientMessage(FString(L"Started the aircraft!"), FName(), 1.f);
			}
		}
		else if (command == "resumesafezone")
		{
			UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bPausedZone = false;
			if (GameMode->HasbSafeZonePaused())
				GameMode->bSafeZonePaused = false;
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startsafezone"), nullptr);
			PlayerController->ClientMessage(FString(L"Resumed the safe zone."), FName(), 1.f);
		}
		else if (command == "pausesafezone")
		{
			UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bPausedZone = true;
			if (GameMode->HasbSafeZonePaused())
				GameMode->bSafeZonePaused = true;
			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"pausesafezone"), nullptr);
			PlayerController->ClientMessage(FString(L"Paused the safe zone."), FName(), 1.f);
		}
		else if (command == "skipsafezone")
		{
			if (GameMode->HasSafeZoneIndicator())
			{
				if (GameMode->SafeZoneIndicator)
				{
					GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
					GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.05f;
				}
			}
			else
			{
				auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

				if (GamePhaseLogic->SafeZoneIndicator)
				{
					GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
					GamePhaseLogic->SafeZoneIndicator->SafeZoneFinishShrinkTime = GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.05f;
				}
			}

			PlayerController->ClientMessage(FString(L"Currently skipping the zone."), FName(), 1.f);
			//UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"skipsafezone"), nullptr);
		}
		else if (command == "startshrinksafezone")
		{
			auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
			if (GameMode->HasSafeZoneIndicator())
			{
				if (GameMode->SafeZoneIndicator)
					GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			}
			else
			{
				auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

				if (GamePhaseLogic->SafeZoneIndicator)
					GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			}

			PlayerController->ClientMessage(FString(L"Started shrinking the zone."), FName(), 1.f);

			//UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"startshrinksafezone"), nullptr);
		}
		else if (command == "infiniteammo")
			FConfiguration::bInfiniteAmmo ^= 1;
		else if (command == "infinitemats")
			FConfiguration::bInfiniteMats ^= 1;
		else if (command == "demospeed")
		{
			if (args.size() != 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;
			}

			auto ws = L"demospeed " + UEAllocatedWString(args[1].begin(), args[1].end());

			UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(ws.c_str()), nullptr);
			PlayerController->ClientMessage(FString(L"Modified the server's demospeed!"), FName(), 1.f);
		}
		else if (command == "god")
		{
			bool bUseMin = false;

			if (args.size() > 1)
			{
				std::string FullCommand = args[1].c_str();
				std::transform(FullCommand.begin(), FullCommand.end(), FullCommand.begin(), tolower);

				if (FullCommand == "min" || FullCommand == "minimum")
					bUseMin = true;
			}

			float MinValue = bUseMin ? 1.f : 100.f;
			auto& Health = PlayerController->MyFortPawn->HealthSet->Health;

			if (VersionInfo.FortniteVersion >= 21)
			{
				PlayerController->Pawn->bCanBeDamaged ^= 1;
				PlayerController->ClientMessage(FString(L"Toggled god mode!"), FName(), 1.f);
				return;
			}
			else if (Health.Minimum != MinValue)
			{
				Health.Minimum = MinValue;
				PlayerController->MyFortPawn->HealthSet->OnRep_Health(Health);

				if (bUseMin)
					PlayerController->ClientMessage(FString(L"Minimum Health God mode enabled!"), FName(), 1);
				else
					PlayerController->ClientMessage(FString(L"God mode enabled!"), FName(), 1);
			}
			else
			{
				Health.Minimum = 0.f;
				PlayerController->MyFortPawn->HealthSet->OnRep_Health(Health);

				PlayerController->ClientMessage(FString(L"God mode disabled!"), FName(), 1);
			}
		}
		else if (command == "speed")
		{
			float Speed = 1.0f;

			if (args.size() > 1)
			{
				try { Speed = std::stof(std::string(args[1])); }
				catch (...)
				{
					PlayerController->ClientMessage(FString(L"Invalid speed value"), FName(), 1.f);
					return;
				}
			}

			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
			{
				PlayerController->ClientMessage(FString(L"No pawn to set speed"), FName(), 1.f);
				return;
			}

			static auto SetMovementSpeedFn = Pawn->GetFunction("SetMovementSpeed");

			if (!SetMovementSpeedFn)
				SetMovementSpeedFn = Pawn->GetFunction("SetMovementSpeedMultiplier");

			if (!SetMovementSpeedFn)
				return;

			Pawn->ProcessEvent(SetMovementSpeedFn, &Speed);
			PlayerController->ClientMessage(FString(L"Set player speed!"), FName(), 1.f);
		}
		else if (command == "gravity")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Usage: gravity <multiplier>"), FName(), 1.f);
				return;
			}

			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
				return;

			float Multiplier = 1.0f;

			try
			{
				std::string argStr(args[1].begin(), args[1].end());
				Multiplier = std::stof(argStr);
			}
			catch (...)
			{
				PlayerController->ClientMessage(FString(L"Invalid multiplier!"), FName(), 1.f);
				return;
			}

			Pawn->SetGravityMultiplier(Multiplier);

			PlayerController->ClientMessage(FString(L"Gravity multiplier set!"), FName(), 1.f);
		}
		else if (command == "changename" || command == "name")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Please include a phrase/name you'd want to change to!"), FName(), 1);
				return;
			}

			std::string nameStr;

			for (size_t i = 1; i < args.size(); i++)
			{
				nameStr += std::string(args[i].begin(), args[i].end());
				if (i + 1 < args.size())
					nameStr += " ";
			}

			if (nameStr.empty())
			{
				PlayerController->ClientMessage(FString(L"Invalid name!"), FName(), 1);
				return;
			}

			std::wstring nameW(nameStr.begin(), nameStr.end());
			FString NewName(nameW.c_str());

			if (!PlayerController)
				return;

			PlayerController->ServerChangeName(NewName);

			PlayerController->ClientMessage(FString(L"Changed the player's name!"), FName(), 1.f);
		}
		else if (command == "pausetimeofday" || command == "pausetime" || command == "pt")
		{
			static bool bIsPaused = false;

			float Speed = bIsPaused ? 1.f : 0.f;

			UFortKismetLibrary::SetTimeOfDaySpeed(UWorld::GetWorld(), Speed);

			if (bIsPaused)
				PlayerController->ClientMessage(FString(L"Unpaused time of day!"), FName(), 1.f);
			else
				PlayerController->ClientMessage(FString(L"Paused time of day!"), FName(), 1.f);

			bIsPaused ^= 1;
		}
		else if (command == "sethealth")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Please choose an amount to set your health to!"), FName(), 1.f);
				return;
			}

			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
			{
				PlayerController->ClientMessage(FString(L"No pawn!"), FName(), 1.f);
				return;
			}

			float Health = 100.f;

			try { Health = std::stof(std::string(args[1])); }
			catch (...) {}

			Pawn->SetHealth(Health);
			PlayerController->ClientMessage(FString(L"Set pawn health!"), FName(), 1.f);
		}
		else if (command == "setshield")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Please choose an amount to set your shield to!"), FName(), 1.f);
				return;
			}

			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
			{
				PlayerController->ClientMessage(FString(L"No pawn!"), FName(), 1.f);
				return;
			}

			float Shield = 100.f;

			try { Shield = std::stof(std::string(args[1])); }
			catch (...) {}

			Pawn->SetShield(Shield);
			PlayerController->ClientMessage(FString(L"Set pawn shield!"), FName(), 1.f);
		}
		else if (command == "keepinv" || command == "keepinventory")
		{
			FConfiguration::bKeepInventory ^= 1;
			PlayerController->ClientMessage(FString(L"Toggled keep inventory!"), FName(), 1.f);
		}
		else if (command == "timeofday" || command == "time" || command == "t")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;

			}

			float NewTOD = 0.f;
			try { NewTOD = std::stof(std::string(args[1])); }
			catch (...) {}

			UFortKismetLibrary::SetTimeOfDay(UWorld::GetWorld(), NewTOD);
			PlayerController->ClientMessage(FString(L"Set time of day!"), FName(), 1.f);
		}
		else if (command == "spawnbot")
		{
			if (!PlayerController->Pawn)
				return;

			auto CallerController = PlayerController;
			int Count = 1;

			if (args.size() >= 2)
			{
				try { Count = std::stoi(args[1].c_str(), nullptr); }
				catch (...) {}
			}

			for (int i = 0; i < Count; i++)
			{
				auto Transform = PlayerController->Pawn->GetTransform();

				auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
				auto GameState = GameMode->GameState;
				//auto PlayerController = (AFortPlayerControllerAthena*)UWorld::SpawnActor(GameMode->PlayerControllerClass, FVector{});
				auto Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(GameMode->DefaultPawnClass, Transform);
				auto PlayerController = (AFortPlayerControllerAthena*)UWorld::SpawnActor(FindObject<UClass>(L"/Game/Athena/Athena_PlayerController.Athena_PlayerController_C"), FVector{});
				//auto PlayerState = PlayerController->PlayerState;

				if (!PlayerController || !Pawn)
					continue;

				PlayerController->Possess(Pawn);
				PlayerController->MyFortPawn = Pawn; // dont't ask, crashes on 27+

				auto PlayerState = (AFortPlayerStateAthena*)UWorld::SpawnActor(AFortPlayerStateAthena::StaticClass(), FVector{});

				PlayerState->SetOwner(PlayerController);

				PlayerController->PlayerState = PlayerState;
				PlayerController->OnRep_PlayerState();

				Pawn->PlayerState = PlayerState;
				Pawn->OnRep_PlayerState();

				Pawn->SetMaxHealth(100.f);
				//Pawn->SetHealth(100.f);

				PlayerState->TeamIndex = AFortGameMode::PickTeam(GameMode, 0, PlayerController);
				if (PlayerState->HasSquadId())
					PlayerState->SquadId = PlayerState->TeamIndex - 3;
				if (PlayerState->HasbIsABot())
					PlayerState->bIsABot = true;


				if (GameState->HasGameMemberInfoArray())
				{
					auto Member = (FGameMemberInfo*)malloc(FGameMemberInfo::Size());
					memset((PBYTE)Member, 0, FGameMemberInfo::Size());

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

				for (auto& AbilitySet : AFortGameMode::AbilitySets)
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


				static auto CharacterPartsOffset = PlayerState->GetOffset("CharacterParts", 0x100000);

				if (CharacterPartsOffset == -1)
				{
					static auto CharacterPartsOff = PlayerState->GetOffset("CharacterParts");
					if (CharacterPartsOff == -1)
						CharacterPartsOff = PlayerState->GetOffset("LocalCharacterParts");
					auto& CharacterParts = GetFromOffset<const UObject * [0x6]>(PlayerState, CharacterPartsOff);

					static auto Head = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
					static auto Body = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");
					static auto Backpack = FindObject<UObject>(L"/Game/Characters/CharacterParts/Backpacks/NoBackpack.NoBackpack");

					CharacterParts[0] = Head;
					CharacterParts[1] = Body;
					CharacterParts[3] = Backpack;
				}
				else
				{
					static auto CharacterPartsOff = PlayerState->GetOffset("CharacterParts");
					auto& CustomCharacterParts = GetFromOffset<FCustomCharacterParts>(PlayerState, CharacterPartsOff);
					static auto PartsOffset = FCustomCharacterParts::StaticStruct()->GetOffset("Parts");
					auto& CharacterParts = GetFromOffset<const UObject * [0x6]>(&CustomCharacterParts, PartsOffset);

					static auto Head = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
					static auto Body = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");
					static auto Backpack = FindObject<UObject>(L"/Game/Characters/CharacterParts/Backpacks/NoBackpack.NoBackpack");

					CharacterParts[0] = Head;
					CharacterParts[1] = Body;
					CharacterParts[3] = Backpack;
				}
				
				if (ApplyCharacterCustomization)
					((void (*)(AActor*, AFortPlayerPawnAthena*)) ApplyCharacterCustomization)(PlayerState, Pawn);

				PlayerBotID++;
				std::string Name = "Erbium Bot (#" + std::to_string(PlayerBotID) + ")";

				std::wstring WideName(Name.begin(), Name.end());

				FString BotName = FString(WideName.c_str());

				if (std::floor(VersionInfo.FortniteVersion) < 9)
				{
					PlayerController->ServerChangeName(BotName);
				}
				else
				{
					GameMode->ChangeName(PlayerController, BotName, true);
				}

				PlayerState->OnRep_PlayerName();

				/*static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

				PlayerController->WorldInventory->GiveItem(DefaultPickaxe);

				static auto SmartItem	DefClass = FindClass("FortSmartBuildingItemDefinition");

				for (int i = 0; i < GameMode->StartingItems.Num(); i++)
				{
					auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

					if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
						PlayerController->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
				}*/

				//CallerController->ClientMessage(FString(L"Spawned a player bot!"), FName(), 1.f); // todo: fix
			}
		}
		else if (command == "startevent")
		{
			Events::StartEvent();
			PlayerController->ClientMessage(FString(L"Event started!"), FName(), 1);
		}
		else if (command == "bugitgo" || command == "tp")
		{
			if (args.size() != 4)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;
			}

			double X = 0., Y = 0., Z = 0.;

			X = strtod(args[1].c_str(), nullptr);
			Y = strtod(args[2].c_str(), nullptr);
			Z = strtod(args[3].c_str(), nullptr);

			if (PlayerController->Pawn)
			{
				PlayerController->Pawn->K2_SetActorLocation(FVector(X, Y, Z), false, nullptr, true);
				PlayerController->ClientMessage(FString(L"Teleported to location!"), FName(), 1.f);
			}
		}
		else if (command == "launch" || command == "launchpawn")
		{
			if (args.size() != 4)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;
			}

			double X = 0., Y = 0., Z = 0.;

			X = strtod(args[1].c_str(), nullptr);
			Y = strtod(args[2].c_str(), nullptr);
			Z = strtod(args[3].c_str(), nullptr);

			if (PlayerController->Pawn)
			{
				PlayerController->Pawn->LaunchCharacterJump(FVector(X, Y, Z), false, nullptr, true);
				PlayerController->ClientMessage(FString(L"Launched player!"), FName(), 1.f);
			}
		}
		else if (command == "savewaypoint" || command == "s")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Please provide a phrase to save the waypoint to."), FName(), 1.f);
				return;
			}

			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
			{
				PlayerController->ClientMessage(FString(L"Couldn't find a pawn!"), FName(), 1.f);
				return;
			}

			FVector PawnLocation(Pawn->K2_GetActorLocation().X, Pawn->K2_GetActorLocation().Y, Pawn->K2_GetActorLocation().Z);

			if (PawnLocation.X == 0.0f && PawnLocation.Y == 0.0f && PawnLocation.Z == 0.0f)
			{
				PlayerController->ClientMessage(FString(L"Failed to save a waypoint."), FName(), 1.f);
				return;
			}

			std::string Phrase = args[1].c_str();

			auto It = Waypoints.find(Phrase);

			if (It != Waypoints.end())
			{
				if (args.size() >= 3 && (args[2] == "override" || args[2] == "o"))
				{
					It->second.clear();
					It->second.push_back(PawnLocation);

					PlayerController->ClientMessage(FString(L"Waypoint overridden successfully!"), FName(), 1.f);
				}
				else
				{
					PlayerController->ClientMessage(FString(L"A waypoint with this phrase already exists! Use 'waypoint {phrase} override' to override it."), FName(), 1);
				}
			}
			else
			{
				std::vector<FVector> Locations;
				Locations.push_back(PawnLocation);
				Waypoints[Phrase] = Locations;

				PlayerController->ClientMessage(FString(L"Waypoint saved! Use \" cheat waypoint (phrase) \" to teleport to that location!"), FName(), 1);
			}
		}
		else if (command == "waypoint" || command == "w")
		{
			if (args.size() < 2)
			{
				PlayerController->ClientMessage(FString(L"Please provide a waypoint phrase to teleport to."), FName(), 1.f);
				return;
			}

			std::string Phrase = args[1].c_str();

			auto It = Waypoints.find(Phrase);

			if (It == Waypoints.end() || It->second.empty())
			{
				PlayerController->ClientMessage(FString(L"A saved waypoint with this phrase was not found!"), FName(), 1.f);
				return;
			}

			const auto& WaypointList = It->second;

			if (args.size() >= 3 && (args[2] == "previous" || args[2] == "p"))
			{
				if (WaypointList.size() < 2)
				{
					PlayerController->ClientMessage(FString(L"No previous waypoint available for this phrase!"), FName(), 1.f);
					return;
				}

				FVector Destination = Waypoints[Phrase][Waypoints[Phrase].size() - 2];

				auto Pawn = PlayerController->Pawn;

				if (Pawn)
				{
					Pawn->K2_TeleportTo(Destination, Pawn->K2_GetActorRotation(), false, true);
					Pawn->LaunchCharacterJump(FVector(0.0f, 0.0f, -10000000.0f), false, nullptr, true);
					PlayerController->ClientMessage(FString(L"Teleported to previous waypoint!"), FName(), 1.f);
				}
				else
				{
					PlayerController->ClientMessage(FString(L"Couldn't find a pawn to teleport!"), FName(), 1.f);
				}
			}
			else
			{
				FVector Destination = WaypointList.back();

				if (Destination.X == 0.0f && Destination.Y == 0.0f && Destination.Z == 0.0f)
				{
					PlayerController->ClientMessage(FString(L"Waypoint is invalid (0, 0, 0)! Aborting teleport."), FName(), 1.f);
					return;
				}

				auto Pawn = PlayerController->Pawn;

				if (Pawn)
				{
					Pawn->K2_TeleportTo(Destination, Pawn->K2_GetActorRotation(), false, true);
					Pawn->LaunchCharacterJump(FVector(0.0f, 0.0f, -10000000.0f), false, nullptr, true);
					PlayerController->ClientMessage(FString(L"Teleported to waypoint!"), FName(), 1.f);
				}
				else
				{
					PlayerController->ClientMessage(FString(L"Couldn't find a pawn to teleport!"), FName(), 1.f);
				}
			}
		}
		else if (command == "giveitem")
		{
			if (args.size() != 2 && args.size() != 3)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;
			}

			auto ItemDefinition = FindObject<UFortItemDefinition>(UEAllocatedWString(args[1].begin(), args[1].end()));
			if (!ItemDefinition)
				ItemDefinition = TUObjectArray::FindObject<UFortItemDefinition>(args[1].c_str());

			if (!ItemDefinition)
				return PlayerController->ClientMessage(FString(L"Failed to find item! Try passing it as a path or check your spelling & casing"), FName(), 1);

			int32 Count = 1;

			if (args.size() == 3)
			{
				Count = strtol(args[2].c_str(), nullptr, 10);
				if (Count <= 0)
					Count = 1;
			}

			//auto ItemEntry = AFortInventory::MakeItemEntry(ItemDefinition, Count, 0);
			//PlayerController->InternalPickup(ItemEntry);
			//free(ItemEntry);
			auto Pawn = PlayerController->Pawn;

			if (!Pawn)
				return;

			FVector FinalLoc = Pawn ? Pawn->K2_GetActorLocation() : FVector();

			FVector ForwardVector = Pawn ? Pawn->GetActorForwardVector() : FVector();
			ForwardVector.Z = 0.0f;
			ForwardVector.Normalize();

			FinalLoc = FinalLoc + ForwardVector * 450.f;
			FinalLoc.Z += 50.f;

			const float RandomAngleVariation = ((float)rand() * 0.00109866634f) - 18.f;
			const float FinalAngle = RandomAngleVariation * 0.017453292519943295f;

			FinalLoc.X += cos(FinalAngle) * 100.f;
			FinalLoc.Y += sin(FinalAngle) * 100.f;

			auto Pickup = AFortInventory::SpawnPickup(FinalLoc, ItemDefinition, Count, 0, EFortPickupSourceTypeFlag::GetOther(), EFortPickupSpawnSource::GetUnset(), Pawn);

			Pawn->ServerHandlePickup(Pickup, Pickup->PickupLocationData.FlyTime, FVector(), true);
			PlayerController->ClientMessage(FString(L"Gave item!"), FName(), 1.f);
			//PlayerController->WorldInventory->GiveItem(ItemDefinition, Count);
		}
		else if (command == "spawnpickup")
		{
			if (args.size() != 2 && args.size() != 3)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
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
			{
				AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation(), ItemDefinition, Count, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), PlayerController->Pawn);
				PlayerController->ClientMessage(FString(L"Spawned pickup!"), FName(), 1.f);
			}
		}
		else if (command == "spawnactor" || command == "summon")
		{
			if (args.size() != 2)
			{
				PlayerController->ClientMessage(FString(L"Wrong number of arguments!"), FName(), 1.f);
				return;
			}

			auto Loc = PlayerController->Pawn->K2_GetActorLocation();
			Loc.Z += 200.f;

			auto Class = FindObject<UClass>(UEAllocatedWString(args[1].begin(), args[1].end()).c_str());

			if (!Class)
				Class = FindClass(args[1].c_str());

			if (Class)
			{
				UWorld::SpawnActor(Class, Loc);
				PlayerController->ClientMessage(FString(L"Spawned actor!"), FName(), 1.f);
			}
			else
			{
				return PlayerController->ClientMessage(FString(L"Failed to find class! Try passing it as a path or check your spelling & casing"), FName(), 1);
			}
		}
		else if (command == "skydive")
		{
			auto Pawn = PlayerController->Pawn;
			if (!Pawn)
				return;

			static bool bInVortex = false;
			bInVortex ^= 1;

			Pawn->SetInVortex(bInVortex);
		}
		else if (command == "resetbuilds" || command == "reset")
		{
			TArray<ABuildingSMActor*> Builds;
			Utils::GetAll<ABuildingSMActor>(Builds);
			
			for (auto& Build : Builds)
				if (Build->bPlayerPlaced)
					Build->K2_DestroyActor();

			Builds.Free();
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

	auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;

	auto sendStat = [&]()
	{
		if (!ReceivingActor)
			return;

		FGameplayTagContainer TargetTags{};
		
		auto Interface = (IGameplayTagAssetInterface*)ReceivingActor->GetInterface(IGameplayTagAssetInterface::StaticClass());
		if (Interface)
		{
			auto GetOwnedGameplayTags = (void(*)(IGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->Vft[0x2];
			GetOwnedGameplayTags(Interface, &TargetTags);
			//Interface->GetOwnedGameplayTags(&TargetTags);
		}

		PlayerController->GetQuestManager(1)->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetInteract(), 1, ReceivingActor, TargetTags);

		//TargetTags.GameplayTags.Free();
		//TargetTags.ParentTags.Free();
	};

	if (auto Container = bDidntFind ? ReceivingActor->Cast<ABuildingContainer>() : nullptr)
		UFortLootPackage::SpawnLootHook(Container);
	else if (auto Vehicle = ReceivingActor->Cast<AFortAthenaVehicle>())
	{
		ServerAttemptInteract_OG(Context, Stack);
		sendStat();

		UFortVehicleSeatWeaponComponent* SeatWeaponComponent = nullptr;

		if (Vehicle)
			SeatWeaponComponent = (UFortVehicleSeatWeaponComponent*)Vehicle->GetComponentByClass(UFortVehicleSeatWeaponComponent::StaticClass());
		else if (auto CharacterVehicle = Pawn->Cast<AFortCharacterVehicle>())
			SeatWeaponComponent = (UFortVehicleSeatWeaponComponent*)CharacterVehicle->GetComponentByClass(UFortVehicleSeatWeaponComponent::StaticClass());


		if (SeatWeaponComponent)
		{
			UFortVehicleSeatComponent* SeatComponent = nullptr;

			if (Vehicle)
				SeatComponent = (UFortVehicleSeatComponent*)Vehicle->GetComponentByClass(UFortVehicleSeatComponent::StaticClass());
			else if (auto CharacterVehicle = Pawn->Cast<AFortCharacterVehicle>())
				SeatComponent = (UFortVehicleSeatComponent*)CharacterVehicle->GetComponentByClass(UFortVehicleSeatComponent::StaticClass());

			auto SeatIdx = SeatComponent->FindSeatIndex(Pawn);
			UFortWeaponItemDefinition* Weapon = nullptr;

			for (int i = 0; i < SeatWeaponComponent->WeaponSeatDefinitions.Num(); i++)
			{
				auto& WeaponDefinition = SeatWeaponComponent->WeaponSeatDefinitions.Get(i, FWeaponSeatDefinition::Size());

				if (WeaponDefinition.SeatIndex != SeatIdx)
					continue;

				Weapon = WeaponDefinition.VehicleWeapon;
				break;
			}

			if (Weapon)
			{
				printf("Weapon: %s\n", Weapon->Name.ToString().c_str());
				auto Item = PlayerController->WorldInventory->GiveItem(Weapon, 1, AFortInventory::GetStats(Weapon)->ClipSize);
				auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
					{ return entry.ItemDefinition == Weapon; }, FFortItemEntry::Size());

				auto OldWeapon = Pawn->CurrentWeapon;

				PlayerController->ServerExecuteInventoryItem(ItemEntry->ItemGuid);
				PlayerController->ClientEquipItem(ItemEntry->ItemGuid, true);
				if (Pawn->HasPreviousWeapon())
					Pawn->PreviousWeapon = OldWeapon;

				auto Weapon = (AFortWeapon*)Pawn->CurrentWeapon;

				if (Weapon)
				{
					auto RepWeaponInfo = (FMountedWeaponInfoRepped*)malloc(FMountedWeaponInfoRepped::Size());

					if (RepWeaponInfo->HasHostVehicleCached())
					{
						RepWeaponInfo->HostVehicleCached.ObjectPointer = Vehicle;
						RepWeaponInfo->HostVehicleCached.InterfacePointer = Vehicle->GetInterface(IFortVehicleInterface::StaticClass());
					}
					else
						RepWeaponInfo->HostVehicleCachedActor = Vehicle;
					RepWeaponInfo->HostVehicleSeatIndexCached = SeatIdx;

					static auto DualClass = FindClass("FortWeaponRangedDualForVehicle");

					if (Weapon->IsA(DualClass))
					{
						static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
						static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
						*(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
						Weapon->Call(OnRep_MountedWeaponInfoRepped);
					}
					else
					{
						static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
						static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
						*(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
						Weapon->Call(OnRep_MountedWeaponInfoRepped);
					}

					free(RepWeaponInfo);
				}
			}
		}
		return;
	}
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
			ServerAttemptInteract_OG(Context, Stack);
			sendStat();
			return;
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
				CollectorActor->Call(CollectorActor->GetFunction("BlueprintOnInteract"), Pawn);
				//CollectorActor->PlayVendFailFX();
				ServerAttemptInteract_OG(Context, Stack);
				sendStat();
				return;
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

			itemEntry->Count -= (int)Cost;
			if (itemEntry->Count <= 0)
				PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
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

				Item->ItemEntry.Count = itemEntry->Count;
				PlayerController->WorldInventory->UpdateEntry(*itemEntry);
				Item->ItemEntry.bIsDirty = true;
			}
		}

		CollectorActor->ClientPausedActiveInputItem = CollectorActor->ActiveInputItem;
		CollectorActor->bCurrentInteractionSuccess = true;
		CollectorActor->Call(CollectorActor->GetFunction("BlueprintOnInteract"), Pawn);
		//CollectorActor->PlayVendFX();
	}
	else if (auto LockDevice = ReceivingActor->Cast<ABuildingProp_LockDevice>())
	{
		printf("yo %s\n", LockDevice->LockableObject->Name.ToString().c_str());
		LockDevice->CurrentLockState = 1;
		LockDevice->OnRep_CurrentLockState();
	}

	ServerAttemptInteract_OG(Context, Stack);
	sendStat();
	//return bIsComp ? (void)callOG(((UFortControllerComponent_Interaction*)Context), Stack.GetCurrentNativeFunction(), ServerAttemptInteract, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID) : callOG(PlayerController, Stack.GetCurrentNativeFunction(), ServerAttemptInteract, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID);
}
 
void AFortPlayerControllerAthena::ServerDropAllItems(UObject* Context, FFrame& Stack)
{
	UFortItemDefinition* IgnoreItemDef;

	Stack.StepCompiledIn(&IgnoreItemDef);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	printf("ServerDropAllItems[Ignore %s]\n", IgnoreItemDef ? IgnoreItemDef->Name.ToString().c_str() : nullptr);

	auto Loc = PlayerController->MyFortPawn->K2_GetActorLocation();
	for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

		if (Entry.ItemDefinition != IgnoreItemDef && Entry.ItemDefinition->CanBeDropped())
		{
			AFortInventory::SpawnPickup(Loc, Entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
			PlayerController->WorldInventory->Remove(Entry.ItemGuid);
		}
	}
}


void (*OnUnEquipOG)(AFortWeapon*);
void OnUnEquip(AFortWeapon* Weapon)
{
	AFortInventory::RemoveWeaponAbilities(Weapon);

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
	DEFINE_PROP(OnHeldObjectPickedUp, TMulticastInlineDelegate<void()>);
	DEFINE_PROP(OnHeldObjectDropped, TMulticastInlineDelegate<void()>);

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
	if (HeldObjectComponent->HasOnHeldObjectOwningPawnChanged())
		HeldObjectComponent->OnHeldObjectOwningPawnChanged.Process();
	if (Pawn->HasOnHeldObjectPickedUp())
	{
		if (Pawn)
			Pawn->OnHeldObjectPickedUp.Process(HeldObject);
		else
			PreviousPawn->OnHeldObjectDropped.Process(HeldObject);
	}
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
	HeldObjectComponent->OnHeldObjectPickedUp.Process();
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

	AFortPlayerPawnAthena* OwningPawn = nullptr;

	if (HeldObjectComponent->HasOwningPawn())
		OwningPawn = HeldObjectComponent->OwningPawn;
	else
	{
		// its a weakobjectptr on older builds
		static auto OwningPawnOff = HeldObjectComponent->GetOffset("OwningPawn", 0x8000000);
		OwningPawn = (*(TWeakObjectPtr<AFortPlayerPawnAthena>*)(__int64(HeldObjectComponent) + OwningPawnOff)).Get();
	}
	if (!OwningPawn)
		return;


	auto PlayerController = (AFortPlayerControllerAthena*)OwningPawn->Controller;

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
	HeldObjectComponent->OnHeldObjectDropped.Process();
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

	if (!FConfiguration::bKeepInventory && PlayerController->WorldInventory)
	{
		UEAllocatedVector<FGuid> GuidsToRemove;
		for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (Entry.ItemDefinition->CanBeDropped())
			{
				//NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Remove(i, FFortItemEntry::Size());
				//i--;
				GuidsToRemove.push_back(Entry.ItemGuid);
			}
			if (VersionInfo.FortniteVersion < 3 && Entry.ItemDefinition->ItemType == EFortItemType::GetWeaponHarvest())
			{
				//PlayerController->ServerExecuteInventoryItem(Entry.ItemGuid);
				//PlayerController->QuickBars->ServerActivateSlotInternal(0, 0, 0.f, true);
			}
		}

		for (auto& Guid : GuidsToRemove)
			PlayerController->WorldInventory->Remove(Guid);
	}

	return EnterAircraftOG(Object, Aircraft);
}

class AFortPlayerStartCreative : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortPlayerStartCreative);

	DEFINE_PROP(PlayerStartTags, FGameplayTagContainer);
};
void AFortPlayerControllerAthena::ServerTeleportToPlaygroundLobbyIsland(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	static auto CreativePhone = FindObject<UFortWeaponItemDefinition>(L"/Game/Athena/Items/Weapons/Prototype/WID_CreativeTool.WID_CreativeTool");

	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition == CreativePhone;
		}, FFortItemEntry::Size());
	if (ItemEntry)
		PlayerController->WorldInventory->Remove(ItemEntry->ItemGuid);

	if (PlayerController->HasbIsCreativeQuickbarEnabled())
	{
		auto OldbIsCreativeQuickbarEnabled = PlayerController->bIsCreativeQuickbarEnabled;
		PlayerController->bIsCreativeQuickbarEnabled = false;
		PlayerController->OnRep_IsCreativeQuickbarEnabled(OldbIsCreativeQuickbarEnabled);
	}
	if (PlayerController->HasbIsCreativeQuickmenuEnabled())
		PlayerController->bIsCreativeQuickmenuEnabled = false;
	if (PlayerController->HasbIsCreativeModeEnabled())
	{
		PlayerController->bIsCreativeModeEnabled = false;
		PlayerController->OnRep_IsCreativeModeEnabled();
	}

	AActor* Actor = GameMode->ChoosePlayerStart(PlayerController);
	PlayerController->Pawn->K2_TeleportTo(Actor->K2_GetActorLocation(), Actor->K2_GetActorRotation());

	PlayerController->CreativePlotLinkedVolume = PlayerController->GetCurrentVolume();
	PlayerController->OnRep_CreativePlotLinkedVolume();
}

inline std::string CleanupString(std::string& s)
{
	if (s.rfind("Schematic:", 0) == 0)
	{
		s.erase(0, 10);
	}
	return s;
}

void AFortPlayerControllerAthena::ServerCraftSchematic(UObject* Context, FFrame& Stack)
{
	FString ItemId;
	int32 PostCraftSlot;
	bool bIsQuickCrafted;
	Stack.StepCompiledIn(&ItemId);
	Stack.StepCompiledIn(&PostCraftSlot);
	Stack.StepCompiledIn(&bIsQuickCrafted);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	auto SchematicStr = ItemId.ToString();
	std::string SchematicStdStr = SchematicStr.c_str();

	printf("CraftShit: ItemId='%s'\n", SchematicStdStr.c_str());

	auto CleanedSchematic = CleanupString(SchematicStdStr);

	printf("clean schematic broo: '%s'\n", CleanedSchematic.c_str());

	auto Schematic = TUObjectArray::FindObject<UFortSchematicItemDefinition>(CleanedSchematic.c_str());
	if (Schematic)
	{
		printf("schematic found : %s\n", Schematic->Name.ToString().c_str());

		auto ResultItemDef = Schematic->GetResultWorldItemDefinition();

		printf("item: %s\n", ResultItemDef->Name.ToString().c_str());

		if (ResultItemDef) {

			auto subbed = Schematic->MaxLevel - Schematic->MinLevel;

			if (subbed <= -1)
				subbed = 0;
			else
			{
				auto calc = (int)(((float)rand() / 32767) * (float)(subbed + 1));
				if (calc <= subbed)
					subbed = calc;
			}

			auto NewEntry = AFortInventory::MakeItemEntry(ResultItemDef, Schematic->GetQuantityProduced(), subbed + Schematic->MinLevel);

			PlayerController->InternalPickup(NewEntry);
			free(NewEntry);

			if (Schematic->CraftingRecipe.DataTable)
			{
				auto FoundRecipe = Schematic->CraftingRecipe.DataTable->RowMap.Search([&](FName& Key, uint8_t*& Value)
					{ return Key == Schematic->CraftingRecipe.RowName; });

				if (!FoundRecipe)
				{
					printf("[Crafting] Failed to find recipe!\n");
					return;
				}

				auto Recipe = *(FRecipe**)FoundRecipe;
				auto CostCount = Recipe->RecipeCosts.Num();

				printf("num costs: %d\n", CostCount);

				for (int i = 0; i < Recipe->RecipeCosts.Num(); i++)
				{
					auto& Cost = Recipe->RecipeCosts.Get(i, FFortItemQuantityPair::Size());

					auto ItemDef = Cost.ItemDefinition.Get();

					auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
						{ return Entry.ItemDefinition == ItemDef; }, FFortItemEntry::Size());
					auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem*& Item)
						{ return Item->ItemEntry.ItemDefinition == ItemDef; });

					if (itemEntry)
					{
						auto Item = *ItemP;

						itemEntry->Count -= Cost.Quantity;

						if (itemEntry->Count <= 0)
							PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
						else
						{
							Item->ItemEntry.Count = itemEntry->Count;
							PlayerController->WorldInventory->UpdateEntry(*itemEntry);
							Item->ItemEntry.bIsDirty = true;
						}
					}
				}
			}

			if (Schematic->HasCraftingRequirements() && Schematic->CraftingRequirements.DataTable)
			{
				auto FoundRequirements = Schematic->CraftingRequirements.DataTable->RowMap.Search([&](FName& Key, uint8_t*& Value)
					{ return Key == Schematic->CraftingRequirements.RowName; });

				if (!FoundRequirements)
				{
					printf("[Crafting] Failed to find requirements!\n");
					return;
				}

				auto Reqirements = *(FSchematicRequirements**)FoundRequirements;
				auto CostCount = Reqirements->Requirements.Num();

				printf("num requirements: %d\n", CostCount);

				for (int i = 0; i < Reqirements->Requirements.Num(); i++)
				{
					auto& Requirement = Reqirements->Requirements.Get(i, FSchematicRequirement::Size());
					auto ItemDef = Requirement.ItemDefinition;

					auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& Entry)
						{ return Entry.ItemDefinition == ItemDef; }, FFortItemEntry::Size());
					auto ItemP = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem*& Item)
						{ return Item->ItemEntry.ItemDefinition == ItemDef; });

					if (itemEntry)
					{
						auto Item = *ItemP;

						itemEntry->Count -= Requirement.Count;

						if (itemEntry->Count <= 0)
							PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
						else
						{
							Item->ItemEntry.Count = itemEntry->Count;
							PlayerController->WorldInventory->UpdateEntry(*itemEntry);
							Item->ItemEntry.bIsDirty = true;
						}
					}
				}
			}
		}
	}
}

void AFortPlayerControllerAthena::ServerGiveCreativeItem(UObject* Context, FFrame& Stack)
{
	auto CreativeItem = (FFortItemEntry*)malloc(FFortItemEntry::Size());
	memset(CreativeItem, 0, FFortItemEntry::Size());
	FGuid ItemToRemoveGuid{};
	Stack.StepCompiledIn(CreativeItem);
	Stack.StepCompiledIn(&ItemToRemoveGuid);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	if (auto WeaponDef = CreativeItem->ItemDefinition->Cast<UFortWeaponItemDefinition>())
		CreativeItem->LoadedAmmo = AFortInventory::GetStats(WeaponDef)->ClipSize;

	PlayerController->InternalPickup(CreativeItem);
	free(CreativeItem);
}

void AFortPlayerControllerAthena::ServerRequestSeatChange_(UObject* Context, FFrame& Stack)
{
	int TargetSeatIndex;

	Stack.StepCompiledIn(&TargetSeatIndex);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto Pawn = PlayerController->Pawn;

	static auto GetVehicleFunc = Pawn->GetFunction("GetVehicleActor");
	if (!GetVehicleFunc)
		GetVehicleFunc = Pawn->GetFunction("GetVehicle");
	auto Vehicle = Pawn->Call<AActor*>(GetVehicleFunc);

	if (!Vehicle && Pawn->IsA<AFortCharacterVehicle>())
		Vehicle = Pawn;

	UFortVehicleSeatWeaponComponent* SeatWeaponComponent = (UFortVehicleSeatWeaponComponent*)Vehicle->GetComponentByClass(UFortVehicleSeatWeaponComponent::StaticClass());
	if (!SeatWeaponComponent)
		return callOG(PlayerController, Stack.GetCurrentNativeFunction(), ServerRequestSeatChange, TargetSeatIndex);

	UFortVehicleSeatComponent* SeatComponent = (UFortVehicleSeatComponent*)Vehicle->GetComponentByClass(UFortVehicleSeatComponent::StaticClass());

	auto SeatIdx = SeatComponent->FindSeatIndex(PlayerController->MyFortPawn);

	UFortWeaponItemDefinition* OldWeapon = nullptr;
	UFortWeaponItemDefinition* NewWeapon = nullptr;
	if (SeatWeaponComponent)
	{
		for (int i = 0; i < SeatWeaponComponent->WeaponSeatDefinitions.Num(); i++)
		{
			auto& WeaponDefinition = SeatWeaponComponent->WeaponSeatDefinitions.Get(i, FWeaponSeatDefinition::Size());

			if (WeaponDefinition.SeatIndex != SeatIdx)
				continue;

			OldWeapon = WeaponDefinition.VehicleWeapon;
			break;
		}

		if (OldWeapon)
		{
			auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
				{ return entry.ItemDefinition == OldWeapon; }, FFortItemEntry::Size());

			if (ItemEntry)
				PlayerController->WorldInventory->Remove(ItemEntry->ItemGuid);
		}

		for (int i = 0; i < SeatWeaponComponent->WeaponSeatDefinitions.Num(); i++)
		{
			auto& WeaponDefinition = SeatWeaponComponent->WeaponSeatDefinitions.Get(i, FWeaponSeatDefinition::Size());

			if (WeaponDefinition.SeatIndex != TargetSeatIndex)
				continue;

			NewWeapon = WeaponDefinition.VehicleWeapon;
			break;
		}
	}

	callOG(PlayerController, Stack.GetCurrentNativeFunction(), ServerRequestSeatChange, TargetSeatIndex);

	if (OldWeapon)
	{
		auto LastItem = Pawn->HasPreviousWeapon() ? (AFortWeapon*)Pawn->PreviousWeapon : nullptr;

		if (LastItem)
		{
			PlayerController->ServerExecuteInventoryItem(LastItem->ItemEntryGuid);
			PlayerController->ClientEquipItem(LastItem->ItemEntryGuid, true);
		}
		else
		{
			auto pickaxeEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([](FFortItemEntry& entry)
				{ return entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>(); }, FFortItemEntry::Size());

			if (pickaxeEntry)
			{
				PlayerController->ServerExecuteInventoryItem(pickaxeEntry->ItemGuid);
				PlayerController->ClientEquipItem(pickaxeEntry->ItemGuid, true);
			}
		}
	}

	if (NewWeapon)
	{
		auto NewItem = PlayerController->WorldInventory->GiveItem(NewWeapon, 1, AFortInventory::GetStats(NewWeapon)->ClipSize);
		auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemDefinition == NewWeapon; }, FFortItemEntry::Size());
		auto CurrentWeapon = Pawn->CurrentWeapon;

		PlayerController->ServerExecuteInventoryItem(ItemEntry->ItemGuid);
		PlayerController->ClientEquipItem(ItemEntry->ItemGuid, true);
		if (Pawn->HasPreviousWeapon())
			Pawn->PreviousWeapon = CurrentWeapon;

		auto Weapon = (AFortWeapon*)Pawn->CurrentWeapon;

		if (Weapon)
		{
			auto RepWeaponInfo = (FMountedWeaponInfoRepped*)malloc(FMountedWeaponInfoRepped::Size());

			if (RepWeaponInfo->HasHostVehicleCached())
			{
				RepWeaponInfo->HostVehicleCached.ObjectPointer = Vehicle;
				RepWeaponInfo->HostVehicleCached.InterfacePointer = Vehicle->GetInterface(IFortVehicleInterface::StaticClass());
			}
			else
				RepWeaponInfo->HostVehicleCachedActor = Vehicle;
			RepWeaponInfo->HostVehicleSeatIndexCached = SeatIdx;

			static auto DualClass = FindClass("FortWeaponRangedDualForVehicle");

			if (Weapon->IsA(DualClass))
			{
				static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
				static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
				*(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
				Weapon->Call(OnRep_MountedWeaponInfoRepped);
			}
			else
			{
				static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
				static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
				*(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
				Weapon->Call(OnRep_MountedWeaponInfoRepped);
			}

			free(RepWeaponInfo);
		}
	}
}

void AFortPlayerControllerAthena::ServerLoadingScreenDropped_(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	callOG(PlayerController, Stack.GetCurrentNativeFunction(), ServerLoadingScreenDropped);
}

void AFortPlayerControllerAthena::ServerCreativeSetFlightSpeedIndex(UObject* Context, FFrame& Stack)
{
	int32 Index_0;

	Stack.StepCompiledIn(&Index_0);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	auto bIsFlyingPossible = PlayerController->Validation_IsFlyingPossible();

	if (PlayerController->Validation_IsFlyingPossible__Ptr && !bIsFlyingPossible)
		return;

	PlayerController->FlyingModifierIndex = Index_0;
	PlayerController->OnRep_FlyingModifierIndex();
}

void AFortPlayerControllerAthena::ServerCreativeSetFlightSprint(UObject* Context, FFrame& Stack)
{
	bool bSprint;

	Stack.StepCompiledIn(&bSprint);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	PlayerController->bIsFlightSprinting = bSprint;
	PlayerController->OnRep_IsFlightSprinting();
}

void AFortPlayerControllerAthena::PostLoadHook()
{
	if (VersionInfo.FortniteVersion >= 27)
	{
		CanPlaceBuildableClassInStructuralGrid_ = FindCanPlaceBuildableClassInStructuralGrid();
		//InitializeBuildingActor_ = FindInitializeBuildingActor();
		//PostInitializeSpawnedBuildingActor_ = FindPostInitializeSpawnedBuildingActor();
	}
	CantBuild_ = FindCantBuild();
	ReplaceBuildingActor_ = FindReplaceBuildingActor(); // pre-cache building offsets
	RemoveFromAlivePlayers_ = FindRemoveFromAlivePlayers();
	GiveAbilityAndActivateOnce = FindGiveAbilityAndActivateOnce();
	CanAffordToPlaceBuildableClass_ = FindCanAffordToPlaceBuildableClass();
	PayBuildableClassPlacementCost_ = FindPayBuildableClassPlacementCost();
	InitializePlayerGameplayAbilities_ = FindInitializePlayerGameplayAbilities();

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
	if (VersionInfo.FortniteVersion < 11)
		ServerAttemptAircraftJumpVft = GetDefaultObj()->GetFunction("ServerAttemptAircraftJump")->GetVTableIndex();
	
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

	//if (VersionInfo.FortniteVersion != 1.72 && VersionInfo.FortniteVersion != 1.8)
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

	if (wcsstr(FConfiguration::Playlist, L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2")) // on 24.20+ u get killed for going into water without this
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerTeleportToPlaygroundLobbyIsland"), ServerTeleportToPlaygroundLobbyIsland);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCraftSchematic"), ServerCraftSchematic);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerGiveCreativeItem"), ServerGiveCreativeItem);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerRequestSeatChange"), ServerRequestSeatChange_, ServerRequestSeatChange_OG);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerLoadingScreenDropped"), ServerLoadingScreenDropped_, ServerLoadingScreenDropped_OG);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCreativeSetFlightSpeedIndex"), ServerCreativeSetFlightSpeedIndex);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCreativeSetFlightSprint"), ServerCreativeSetFlightSprint);
}
