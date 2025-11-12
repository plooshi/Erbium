#include "pch.h"
#include "../Public/BuildingSMActor.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/FortKismetLibrary.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../../Erbium/Public/Configuration.h"


void ABuildingSMActor::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, __int64 HitInfo, AActor* InstigatedBy, AActor* DamageCauser, __int64 EffectContext)
{
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);

	if (!InstigatedBy || !Actor->IsA<ABuildingSMActor>() || Actor->bPlayerPlaced || Actor->GetHealth() == 1 || (Actor->HasbAllowResourceDrop() && !Actor->bAllowResourceDrop))
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	if (!DamageCauser || !DamageCauser->IsA<AFortWeapon>() || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
	if (!Resource)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	auto MaxMat = Resource->GetMaxStackSize();

	static auto Playlist = FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);
	static auto GameData = Playlist ? Playlist->ResourceRates.Get() : nullptr;
	if (!GameData)
		GameData = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

	int ResCount = 0;
	if (Actor->HasBuildingResourceAmountOverride())
	{
		FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out;
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
			float Out;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

			float RC = Out / (Actor->GetMaxHealth() / Damage);

			ResCount = (int)round(RC);
		}
	}

	auto Controller = (AFortPlayerControllerAthena*)InstigatedBy;
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

			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
				break;
			}*/


			Item->ItemEntry.Count = itemEntry->Count;
			Controller->WorldInventory->UpdateEntry(*itemEntry);
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
		Controller->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType::None : Actor->ResourceType, ResCount, Actor->GetHealth() - Damage <= 0, Damage == 100.f);

	Actor->ForceNetUpdate();
	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

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

		auto NewTrap = UWorld::SpawnActor<ABuildingSMActor>(ItemDefinition->BlueprintClass.Get(), Location, Rotation, AttachedActor);
		AttachedActor->AttachBuildingActorToMe(NewTrap, true);
		AttachedActor->bHiddenDueToTrapPlacement = ItemDefinition->bReplacesBuildingWhenPlaced;
		if (ItemDefinition->bReplacesBuildingWhenPlaced)
			AttachedActor->bActorEnableCollision = false;
		AttachedActor->ForceNetUpdate();

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
		}

		if (NewTrap->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex)
		{
			NewTrap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			NewTrap->Team = NewTrap->TeamIndex;
		}
	}
	callOG(DecoTool, Stack.GetCurrentNativeFunction(), ServerSpawnDeco, Location, Rotation, AttachedActor, InBuildingAttachmentType);

	if (VersionInfo.FortniteVersion < 18)
	{
		static auto TrapClass = FindClass("BuildingTrap");
		auto trapPtr = AttachedActor->AttachedBuildingActors.Search([&](ABuildingSMActor*& actor) {
			return actor->IsA(TrapClass) && actor->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			});

		auto trap = trapPtr ? *trapPtr : nullptr;
		if (trap) {
			trap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			trap->Team = trap->TeamIndex;
		}
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

		auto NewTrap = UWorld::SpawnActor<ABuildingSMActor>(ItemDefinition->BlueprintClass.Get(), Location, Rotation, AttachedActor);
		AttachedActor->AttachBuildingActorToMe(NewTrap, true);
		AttachedActor->bHiddenDueToTrapPlacement = ItemDefinition->bReplacesBuildingWhenPlaced;
		if (ItemDefinition->bReplacesBuildingWhenPlaced)
			AttachedActor->bActorEnableCollision = false;
		AttachedActor->ForceNetUpdate();

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
		}

		if (NewTrap->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex)
		{
			NewTrap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			NewTrap->Team = NewTrap->TeamIndex;
		}
	}

	ServerSpawnDeco_ImplementationOG(Context, Stack);

	if (VersionInfo.FortniteVersion < 18)
	{
		static auto TrapClass = FindClass("BuildingTrap");
		auto trapPtr = AttachedActor->AttachedBuildingActors.Search([&](ABuildingSMActor*& actor) {
			return actor->IsA(TrapClass) && actor->TeamIndex != ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			});

		auto trap = trapPtr ? *trapPtr : nullptr;
		if (trap) {
			trap->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
			trap->Team = trap->TeamIndex;
		}
	}
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

	auto OnDamageServerAddr = FindFunctionCall(L"OnDamageServer", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x4C, 0x89, 0x4C } : VersionInfo.EngineVersion == 4.19 || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x40, 0x55 });

	Utils::Hook(OnDamageServerAddr, OnDamageServer, OnDamageServerOG);

	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool.ServerSpawnDeco", AFortDecoTool::ServerSpawnDeco_, AFortDecoTool::ServerSpawnDeco_OG);
	if (AFortDecoTool_ContextTrap::StaticClass())
	{
		auto Func = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerSpawnDeco_Implementation");

		if (!Func)
			Func = AFortDecoTool_ContextTrap::GetDefaultObj()->GetFunction("ServerSpawnDeco");

		Utils::ExecHook(Func, AFortDecoTool_ContextTrap::ServerSpawnDeco_Implementation, AFortDecoTool_ContextTrap::ServerSpawnDeco_ImplementationOG);
	}
}
