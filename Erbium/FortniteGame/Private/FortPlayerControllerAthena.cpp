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

	if (VersionInfo.FortniteVersion >= 11.00)
	{
		static auto ControllerCompClass = FindClass("FortControllerComponent_Aircraft");
		if (Context->IsA(ControllerCompClass))
			PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
		else
			PlayerController = (AFortPlayerControllerAthena*)Context;

		GameMode->RestartPlayer(PlayerController);
		PlayerController->ClientSetRotation(Rotation, true);

		if (PlayerController->MyFortPawn)
		{
			PlayerController->MyFortPawn->BeginSkydiving(true);
			PlayerController->MyFortPawn->SetHealth(100.f);
		}
	}
	else
	{
		static auto ServerAttemptAircraftJumpOG = (void(*)(AFortPlayerControllerAthena*, FRotator&)) ((AFortPlayerControllerAthena*)Context)->Vft[((AFortPlayerControllerAthena*)Context)->GetFunction("ServerAttemptAircraftJump")->GetVTableIndex()];

		ServerAttemptAircraftJumpOG((AFortPlayerControllerAthena*)Context, Rotation);
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

	static auto GadgetClass = FindClass("FortGadgetItemDefinition");
	static auto DecoClass = FindClass("FortDecoItemDefinition");
	static auto ContextTrapClass = FindClass("FortDecoTool_ContextTrap");

	if (ItemDefinition->IsA(GadgetClass))
		ItemDefinition = ItemDefinition->GetWeaponItemDefinition();
	else if (ItemDefinition->IsA(DecoClass))
	{
		PlayerController->MyFortPawn->PickUpActor(nullptr, ItemDefinition);
		((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ItemEntryGuid = ItemGuid;

		if (PlayerController->MyFortPawn->CurrentWeapon->IsA(ContextTrapClass))
			((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = ItemDefinition;

		return;
	}

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
	static auto CantBuild = (__int64 (*)(UWorld*, const UClass*, _Pad_0xC, _Pad_0xC, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
	static auto CantBuildNew = (__int64 (*)(UWorld*, const UClass*, _Pad_0x18, _Pad_0x18, bool, TArray<ABuildingSMActor*> *, char*))CantBuild_;
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
	return HasAllPlayerBuildableClasses ? GameState->AllPlayerBuildableClasses.Search([BuildClass](TSubclassOf<AActor> Class) { return Class == BuildClass; }) : true;
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

	static auto EditToolClass = FindClass("FortEditToolItemDefinition");
	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition->IsA(EditToolClass);
		}, FFortItemEntry::Size());


	PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);

	static auto EditToolWeaponClass = FindClass("FortWeap_EditingTool");
	if (PlayerController->MyFortPawn->CurrentWeapon->IsA(EditToolWeaponClass))
	{
		static auto EditActorOffset = PlayerController->MyFortPawn->CurrentWeapon->GetOffset("EditActor");
		static auto OnRep_EditActor = PlayerController->MyFortPawn->CurrentWeapon->GetFunction("OnRep_EditActor");

		GetFromOffset<ABuildingSMActor*>(PlayerController->MyFortPawn->CurrentWeapon, EditActorOffset) = Building;
		PlayerController->MyFortPawn->CurrentWeapon->Call(OnRep_EditActor);
	}
}


uint64_t ReplaceBuildingActor_ = 0;
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

	static auto ReplaceBuildingActor = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, const UClass*, unsigned int, int, bool, AFortPlayerControllerAthena*)) ReplaceBuildingActor_;

	ABuildingSMActor* NewBuild = ReplaceBuildingActor(Building, 1, NewClass.Get(), Building->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);

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
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != PlayerController->PlayerState || Building->Team != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex || Building->bDestroyed)
		return;

	SetEditingPlayer(Building, nullptr);

	static auto EditToolClass = FindClass("FortEditToolItemDefinition");
	auto EditToolEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition->IsA(EditToolClass);
		}, FFortItemEntry::Size());

	PlayerController->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->HasTrackerGuid() ? EditToolEntry->TrackerGuid : FGuid(), false);

	static auto EditToolWeaponClass = FindClass("FortWeap_EditingTool");
	if (PlayerController->MyFortPawn->CurrentWeapon->IsA(EditToolWeaponClass))
	{
		static auto EditActorOffset = PlayerController->MyFortPawn->CurrentWeapon->GetOffset("EditActor");
		static auto OnRep_EditActor = PlayerController->MyFortPawn->CurrentWeapon->GetFunction("OnRep_EditActor");

		GetFromOffset<ABuildingSMActor*>(PlayerController->MyFortPawn->CurrentWeapon, EditActorOffset) = nullptr;
		PlayerController->MyFortPawn->CurrentWeapon->Call(OnRep_EditActor);
	}
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

	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == Guid; }, FFortItemEntry::Size());
	if (!ItemEntry || (ItemEntry->Count - Count) < 0)
		return;

	ItemEntry->Count -= Count;
	AFortInventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *ItemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn, Count);
	if (ItemEntry->Count == 0)
		PlayerController->WorldInventory->Remove(Guid);
	else
		PlayerController->WorldInventory->UpdateEntry(*ItemEntry);
}

class UAthenaDanceItemDefinition : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UAthenaDanceItemDefinition);

	DEFINE_BITFIELD_PROP(bMovingEmote);
	DEFINE_PROP(WalkForwardSpeed, float);
	DEFINE_BITFIELD_PROP(bMoveForwardOnly);
	DEFINE_BITFIELD_PROP(bMoveFollowingOnly);
};

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
		static auto SprayAbilityClass = Utils::FindObject<UClass>(L"/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");
		AbilityToUse = SprayAbilityClass->GetDefaultObj();
	}
	//else if (auto ToyAsset = Asset->Cast<UAthenaToyItemDefinition>()) {
	//	AbilityToUse = ToyAsset->ToySpawnAbility->DefaultObject;
	//}
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

		static auto EmoteAbilityClass = Utils::FindObject<UClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		AbilityToUse = EmoteAbilityClass->GetDefaultObj();
	}

	if (AbilityToUse) 
	{
		static auto ConstructAbilitySpec = FindConstructAbilitySpec();
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
		((void (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec*, void*)) FindGiveAbilityAndActivateOnce())(AbilitySystemComponent, &handle, Spec, nullptr);
	}
}



void AFortPlayerControllerAthena::ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
{
	if (!PlayerController)
		return ClientOnPawnDiedOG(PlayerController, DeathReport);
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

	static auto MeleeClass = FindClass("FortWeaponMeleeItemDefinition");
	static auto ResourceClass = FindClass("FortResourceItemDefinition");
	static auto RangedWeaponClass = FindClass("FortWeaponRangedItemDefinition");
	static auto ConsumableClass = FindClass("FortConsumableItemDefinition");
	static auto AmmoClass = FindClass("FortAmmoItemDefinition");

	if (!GameState->IsRespawningAllowed(PlayerState) && PlayerController->WorldInventory && PlayerController->MyFortPawn)
	{
		bool bHasMats = false;
		for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

			if (!entry.ItemDefinition->IsA(MeleeClass) && (entry.ItemDefinition->IsA(ResourceClass) || entry.ItemDefinition->IsA(RangedWeaponClass) || entry.ItemDefinition->IsA(ConsumableClass) || entry.ItemDefinition->IsA(AmmoClass)))
			{
				AFortInventory::SpawnPickup(PlayerController->MyFortPawn->K2_GetActorLocation(), entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetPlayerElimination(), PlayerController->MyFortPawn);
			}
		}
	}


	auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
	auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;


	/*PlayerState->PawnDeathLocation = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector();
	PlayerState->DeathInfo.bDBNO = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false;
	PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
	PlayerState->DeathInfo.DeathTags = DeathReport.Tags;
	PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(PlayerState->DeathInfo.DeathTags, PlayerState->DeathInfo.bDBNO);
	//PlayerState->DeathInfo.Downer = KillerPlayerState;
	PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
	PlayerState->DeathInfo.Distance = PlayerController->MyFortPawn ? (PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage ? (KillerPawn ? KillerPawn->GetDistanceTo(PlayerController->MyFortPawn) : 0) : PlayerController->MyFortPawn->Cast<AFortPlayerPawnAthena>()->LastFallDistance) : 0;
	PlayerState->DeathInfo.bInitialized = true;
	PlayerState->OnRep_DeathInfo();*/

	return ClientOnPawnDiedOG(PlayerController, DeathReport);
}


void AFortPlayerControllerAthena::InternalPickup(FFortItemEntry* PickupEntry)
{
	auto MaxStack = (int32)PickupEntry->ItemDefinition->GetMaxStackSize();
	int ItemCount = 0;

	for (int i = 0; i < WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Item = WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

		if (AFortInventory::IsPrimaryQuickbar(Item.ItemDefinition))
			ItemCount += Item.ItemDefinition->HasNumberOfSlotsToTake() ? Item.ItemDefinition->NumberOfSlotsToTake : 1;
	}

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
					AFortInventory::SpawnPickup(GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn);
			}
			else
				WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
		};

	auto GiveOrSwapStack = [&](int32 OriginalCount)
		{
			if (PickupEntry->ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
				WorldInventory->GiveItem(*PickupEntry, OriginalCount - MaxStack, true);
			else
				AFortInventory::SpawnPickup(GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), MyFortPawn, OriginalCount - MaxStack);
		};

	if (MaxStack > 1)
	{
		auto itemEntry = WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
			{ return entry.ItemDefinition == PickupEntry->ItemDefinition && entry.Count < MaxStack; }, FFortItemEntry::Size());

		if (itemEntry)
		{
			if ((itemEntry->Count += PickupEntry->Count) > MaxStack)
			{
				auto OriginalCount = itemEntry->Count;
				itemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			WorldInventory->UpdateEntry(*itemEntry);
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


void AFortPlayerControllerAthena::Hook()
{
	CantBuild_ = FindCantBuild();
	ReplaceBuildingActor_ = FindReplaceBuildingActor(); // pre-cache building offsets

	auto DefaultFortPC = DefaultObjImpl("FortPlayerController");

	Utils::Hook(FindGetPlayerViewPoint(), GetPlayerViewPoint, GetPlayerViewPointOG);
	// they only stripped it on athena for some reason
	auto ServerAcknowledgePossessionIdx = GetDefaultObj()->GetFunction("ServerAcknowledgePossession")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerAcknowledgePossessionIdx, DefaultFortPC->Vft[ServerAcknowledgePossessionIdx]);

	auto ServerAttemptAircraftJumpPC = GetDefaultObj()->GetFunction("ServerAttemptAircraftJump");
	if (!ServerAttemptAircraftJumpPC)
		Utils::ExecHook(DefaultObjImpl("FortControllerComponent_Aircraft")->GetFunction("ServerAttemptAircraftJump"), ServerAttemptAircraftJump, ServerAttemptAircraftJumpOG);
	else
		Utils::ExecHook(ServerAttemptAircraftJumpPC, ServerAttemptAircraftJump);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerAcknowledgePossession"), ServerAcknowledgePossession);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryItem"), ServerExecuteInventoryItem);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryWeapon"), ServerExecuteInventoryWeapon); // S9 shenanigans

	// same as serveracknowledgepossession
	auto ServerReturnToMainMenuIdx = GetDefaultObj()->GetFunction("ServerReturnToMainMenu")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerReturnToMainMenuIdx, DefaultFortPC->Vft[ServerReturnToMainMenuIdx]);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerCreateBuildingActor"), ServerCreateBuildingActor);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerBeginEditingBuildingActor"), ServerBeginEditingBuildingActor);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerEditBuildingActor"), ServerEditBuildingActor);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerEndEditingBuildingActor"), ServerEndEditingBuildingActor);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerAttemptInventoryDrop"), ServerAttemptInventoryDrop);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerPlayEmoteItem"), ServerPlayEmoteItem);

	auto ClientOnPawnDiedAddr = FindFunctionCall(L"ClientOnPawnDied", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x48, 0x89, 0x54 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C });
	Utils::Hook(ClientOnPawnDiedAddr, ClientOnPawnDied, ClientOnPawnDiedOG);
}