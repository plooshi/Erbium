#include "pch.h"
#include "../Public/BuildingSMActor.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../Public/FortGameStateAthena.h"
#include "../Public/FortWeapon.h"
#include "../Public/FortKismetLibrary.h"


void ABuildingSMActor::OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, __int64 HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, __int64 EffectContext) {
	auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
	static auto VMClass = FindClass("B_Athena_VendingMachine_C");
	static int HasLlamaClass = -1;
	static auto MeleeClass = FindClass("FortWeaponMeleeItemDefinition");
	if (HasLlamaClass == -1 && GameState->MapInfo)
	{
		HasLlamaClass = GameState->MapInfo->HasLlamaClass();
	}
	if (!InstigatedBy || Actor->bPlayerPlaced || Actor->GetHealth() == 1 || Actor->IsA(VMClass) || (HasLlamaClass == 1 && Actor->IsA(GameState->MapInfo->LlamaClass))) 
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	if (!DamageCauser || !DamageCauser->IsA<AFortWeapon>() || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(MeleeClass)) 
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	static auto PickaxeTag = UKismetStringLibrary::Conv_StringToName(FString(L"Weapon.Melee.Impact.Pickaxe"));
	auto entry = DamageTags.GameplayTags.Search([](FGameplayTag& entry) {
		return entry.TagName.ComparisonIndex == PickaxeTag.ComparisonIndex;
		}, FGameplayTag::Size());
	if (!entry)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
	if (!Resource)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	auto MaxMat = Resource->GetMaxStackSize();

	FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
	int ResCount = 0;

	if (Actor->BuildingResourceAmountOverride.CurveTable && Actor->BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
	{
		float Out;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Actor->BuildingResourceAmountOverride.CurveTable, Actor->BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

		float RC = Out / (Actor->GetMaxHealth() / Damage);

		ResCount = (int)round(RC);
	}

	if (ResCount > 0)
	{
		auto itemEntry = InstigatedBy->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
			return entry.ItemDefinition == Resource;
			}, FFortItemEntry::Size());

		if (itemEntry)
		{
			itemEntry->Count += ResCount;
			if (itemEntry->Count > MaxMat)
			{
				AFortInventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), itemEntry->ItemDefinition, itemEntry->Count - MaxMat, 0, EFortPickupSourceTypeFlag::GetTossed(), EFortPickupSpawnSource::GetUnset(), InstigatedBy->MyFortPawn);
				itemEntry->Count = MaxMat;
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

	InstigatedBy->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType::None : Actor->ResourceType, ResCount, false, Damage == 100.f);

	Actor->ForceNetUpdate();
	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

void ABuildingSMActor::Hook()
{
	auto OnDamageServerAddr = FindFunctionCall(L"OnDamageServer", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x4C, 0x89, 0x4C } : VersionInfo.EngineVersion == 4.19 || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x40, 0x55 });

	Utils::Hook(OnDamageServerAddr, OnDamageServer, OnDamageServerOG);
}
