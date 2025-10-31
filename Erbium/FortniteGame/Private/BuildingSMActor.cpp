#include "pch.h"
#include "../Public/BuildingSMActor.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/FortKismetLibrary.h"


void ABuildingSMActor::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, __int64 HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, __int64 EffectContext)
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
		/*auto ClassData = Actor->GetClassData();
		FCurveTableRowHandle& BuildingResourceAmountOverride = ClassData->BuildingResourceAmountOverride;

		if (BuildingResourceAmountOverride.CurveTable && BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(BuildingResourceAmountOverride.CurveTable, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

			float RC = Out / (Actor->GetMaxHealth() / Damage);

			ResCount = (int)round(RC);
		}*/


		auto Out = 11.f + (float)rand() / (32767.f / 22.f);
		float RC = Out / (Actor->GetMaxHealth() / Damage);

		ResCount = (int)round(RC);
	}

	if (ResCount > 0)
	{
		auto ItemP = InstigatedBy->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
			{
				return entry->ItemEntry.ItemDefinition == Resource;
			});
		auto itemEntry = InstigatedBy->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition == Resource;
		}, FFortItemEntry::Size());

		if (ItemP)
		{
			auto Item = *ItemP;

			for (int i = 0; i < Item->ItemEntry.StateValues.Num(); i++)
			{
				auto& StateValue = Item->ItemEntry.StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
			}


			itemEntry->Count += ResCount;
			if (itemEntry->Count > MaxMat)
			{
				AFortInventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), Item->ItemEntry.ItemDefinition, Item->ItemEntry.Count - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), InstigatedBy->MyFortPawn);
				Item->ItemEntry.Count = MaxMat;
			}

			Item->ItemEntry = *itemEntry;
			Item->ItemEntry.bIsReplicatedCopy = false;

			for (int i = 0; i < Item->ItemEntry.StateValues.Num(); i++)
			{
				auto& StateValue = Item->ItemEntry.StateValues.Get(i, FFortItemEntryStateValue::Size());

				if (StateValue.StateType != 2)
					continue;

				StateValue.IntValue = 0;
			}

			InstigatedBy->WorldInventory->UpdateEntry(*itemEntry);
		}
		else
		{
			if (ResCount > MaxMat)
			{
				AFortInventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), Resource, ResCount - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), InstigatedBy->MyFortPawn);
				ResCount = MaxMat;
			}

			InstigatedBy->WorldInventory->GiveItem(Resource, ResCount, 0, 0, false);
		}
	}


	if (ResCount > 0)
		InstigatedBy->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType::None : Actor->ResourceType, ResCount, Actor->GetHealth() - Damage <= 0, Damage == 100.f);

	Actor->ForceNetUpdate();
	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

void ABuildingSMActor::PostLoadHook()
{
	if (VersionInfo.FortniteVersion >= 28)
	{
		GetSparseClassData_ = Memcury::Scanner::FindPattern("48 83 EC ? 48 8B 81 ? ? ? ? 45 33 C0 4C 8B C9").Get();
	}

	auto OnDamageServerAddr = FindFunctionCall(L"OnDamageServer", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x4C, 0x89, 0x4C } : VersionInfo.EngineVersion == 4.19 || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x40, 0x55 });

	Utils::Hook(OnDamageServerAddr, OnDamageServer, OnDamageServerOG);
}
