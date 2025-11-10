#include "pch.h"
#include "../Public/BuildingSMActor.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/FortKismetLibrary.h"
#include "../Public/FortPlayerControllerAthena.h"


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

	int ResCount = 0;
	if (Actor->HasBuildingResourceAmountOverride())
	{
		FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.CurveTable && BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(BuildingResourceAmountOverride.CurveTable, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

			float RC = Out / (Actor->GetMaxHealth() / Damage);

			ResCount = (int)round(RC);
		}
	}
	else
	{
		auto ClassData = Actor->GetClassData();
		FCurveTableRowHandle& BuildingResourceAmountOverride = ClassData->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.CurveTable && BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(BuildingResourceAmountOverride.CurveTable, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

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


			Item->ItemEntry.Count += ResCount;
			if (Item->ItemEntry.Count > MaxMat)
			{
				AFortInventory::SpawnPickup(Controller->Pawn->K2_GetActorLocation(), Item->ItemEntry.ItemDefinition, Item->ItemEntry.Count - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), Controller->MyFortPawn);
				Item->ItemEntry.Count = MaxMat;
			}

			/*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
			{
				auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
				break;
			}*/


			//Item->ItemEntry.Count = itemEntry->Count;
			Controller->WorldInventory->UpdateEntry(Item->ItemEntry);
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

void ABuildingSMActor::ServerSpawnDeco(UObject* Context, FFrame& Stack)
{
	ServerSpawnDecoOG(Context, Stack);

	auto AttachedActor = *(ABuildingSMActor**)(Stack.Locals + FVector::Size() + FRotator::Size());

	printf("AttachedActor %s\n", AttachedActor->Name.ToString().c_str());
}

void ABuildingSMActor::ServerSpawnDeco_Implementation(UObject* Context, FFrame& Stack)
{
	ServerSpawnDeco_ImplementationOG(Context, Stack);
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

	Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool.ServerSpawnDeco", ServerSpawnDeco, ServerSpawnDecoOG);
	//Utils::ExecHook(L"/Script/FortniteGame.FortDecoTool_ContextTrap.ServerSpawnDeco_Implementation", ServerSpawnDeco_Implementation, ServerSpawnDeco_ImplementationOG);
}
