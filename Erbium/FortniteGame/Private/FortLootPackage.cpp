#include "pch.h"
#include "../Public/FortLootPackage.h"
#include <FortniteGame/Public/BuildingContainer.h>
#include <FortniteGame/Public/FortGameModeAthena.h>

void UFortLootPackage::SetupLDSForPackage(TArray<FFortItemEntry*>& LootDrops, SDK::FName Package, int i, FName TierGroup, int WorldLevel)
{
	TArray<FFortLootPackageData*> LPGroups;
	for (auto const& Val : LPGroupsAll)
	{
		if (!Val)
			continue;

		if (Val->LootPackageID != Package)
			continue;
		if (i != -1 && Val->LootPackageCategory != i)
			continue;
		if (WorldLevel >= 0) {
			if (Val->MaxWorldLevel >= 0 && WorldLevel > Val->MaxWorldLevel)
				continue;
			if (Val->MinWorldLevel >= 0 && WorldLevel < Val->MinWorldLevel)
				continue;
		}

		LPGroups.Add(Val);
	}
	if (LPGroups.Num() == 0)
		return;

	auto LootPackage = PickWeighted(LPGroups, [](float Total) { return ((float)rand() / 32767.f) * Total; });
	if (!LootPackage)
		return;

	if (LootPackage->LootPackageCall.Num() > 1)
	{
		for (int i = 0; i < LootPackage->Count; i++)
			SetupLDSForPackage(LootDrops, UKismetStringLibrary::Conv_StringToName(LootPackage->LootPackageCall), 0, TierGroup, WorldLevel);

		return;
	}

	auto _Id = Utils::FindObject<UFortItemDefinition>(LootPackage->ItemDefinition.ObjectID.AssetPathName.ToWString().c_str());
	LootPackage->ItemDefinition.WeakPtr = _Id;
	auto ItemDefinition = _Id;
	if (!ItemDefinition)
		return;

	static auto WeaponRangedItemDefinitionClass = FindClass("FortWeaponRangedItemDefinition");
	auto AmmoDef = _Id->IsA(WeaponRangedItemDefinitionClass) && VersionInfo.FortniteVersion >= 11.00 ? ((UFortWeaponItemDefinition*)_Id)->GetAmmoWorldItemDefinition_BP() : nullptr;

	bool found = false;
	bool foundAmmo = false;
	for (auto& LootDrop : LootDrops)
	{
		if ((!AmmoDef || AmmoDef->DropCount) && LootDrop->ItemDefinition == ItemDefinition)
		{
			LootDrop->Count += LootPackage->Count;

			if (LootDrop->Count > ItemDefinition->GetMaxStackSize()) {
				auto OGCount = LootDrop->Count;
				LootDrop->Count = ItemDefinition->GetMaxStackSize();

				//if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary)
				LootDrops.Add(AFortInventory::MakeItemEntry(ItemDefinition, OGCount - (int32)ItemDefinition->GetMaxStackSize(), /*std::clamp(Inventory::GetLevel(ItemDefinition->LootLevelData), ItemDefinition->MinLevel, ItemDefinition->MaxLevel))*/ ItemDefinition->MaxLevel));
			}

			//if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary)
			found = true;
		}

		if (LootDrop->ItemDefinition == AmmoDef)
		{
			LootDrop->Count += AmmoDef->DropCount;

			if (LootDrop->Count > AmmoDef->GetMaxStackSize()) {
				auto OGCount = LootDrop->Count;
				LootDrop->Count = AmmoDef->GetMaxStackSize();

				//if (!AFortInventory::IsPrimaryQuickbar(LootDrop->ItemDefinition))
				LootDrops.Add(AFortInventory::MakeItemEntry(AmmoDef, OGCount - AmmoDef->GetMaxStackSize(), /*std::clamp(Inventory::GetLevel(AmmoDef->LootLevelData), AmmoDef->MinLevel, AmmoDef->MaxLevel)*/ ItemDefinition->MaxLevel));
			}

			//if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary)
			foundAmmo = true;
		}
	}

	if (!found && LootPackage->Count > 0)
		LootDrops.Add(AFortInventory::MakeItemEntry(ItemDefinition, LootPackage->Count, /*std::clamp(Inventory::GetLevel(ItemDefinition->LootLevelData), ItemDefinition->MinLevel, ItemDefinition->MaxLevel)*/ ItemDefinition->MaxLevel));

	if (AmmoDef && AmmoDef->DropCount > 0 && !foundAmmo && LootPackage->Count > 0)
		LootDrops.Add(AFortInventory::MakeItemEntry(AmmoDef, AmmoDef->DropCount, /*std::clamp(Inventory::GetLevel(AmmoDef->LootLevelData), AmmoDef->MinLevel, AmmoDef->MaxLevel)*/ ItemDefinition->MaxLevel));
}

TArray<FFortItemEntry*> UFortLootPackage::ChooseLootForContainer(FName TierGroup, int LootTier, int WorldLevel)
{
	TArray<FFortLootTierData*> TierDataGroups;

	for (auto const& Val : TierDataAllGroups) {
		if (Val->TierGroup == TierGroup && (LootTier == -1 ? true : LootTier == Val->LootTier))
			TierDataGroups.Add(Val);
	}
	auto LootTierData = PickWeighted(TierDataGroups, [](float Total) { return ((float)rand() / 32767.f) * Total; });
	if (!LootTierData)
		return {};

	if (LootTierData->NumLootPackageDrops < 0)
		return {};

	int DropCount;
	if (LootTierData->NumLootPackageDrops < 1.f)
		DropCount = 1;
	else
	{
		DropCount = (int)((LootTierData->NumLootPackageDrops * 2.f) - .5f) >> 1;

		float RemainderSomething = LootTierData->NumLootPackageDrops - (float)DropCount;

		if (RemainderSomething > 0.0000099999997f)
			DropCount += RemainderSomething >= ((float)rand() / 32767);
	}

	float AmountOfLootDrops = 0;
	float MinLootDrops = 0;

	for (auto& Min : LootTierData->LootPackageCategoryMinArray)
	{
		// fortnite also does some bit arithmetic here
		AmountOfLootDrops += Min;
	}

	int SumWeights = 0;

	for (int i = 0; i < LootTierData->LootPackageCategoryWeightArray.Num(); ++i)
		if (LootTierData->LootPackageCategoryWeightArray[i] > 0 && (LootTierData->LootPackageCategoryMaxArray[i] < 0 || 0 < LootTierData->LootPackageCategoryMaxArray[i]))
			SumWeights += LootTierData->LootPackageCategoryWeightArray[i];

	if (AmountOfLootDrops < DropCount)
		while (SumWeights > 0)
		{
			AmountOfLootDrops++;

			if (AmountOfLootDrops >= DropCount) {
				//AmountOfLootDrops = AmountOfLootDrops;
				break;
			}

			SumWeights--;
		}

	//if (!AmountOfLootDrops)
	//	AmountOfLootDrops = AmountOfLootDrops;

	if (!AmountOfLootDrops)
		return {};

	TArray<FFortItemEntry*> LootDrops;
	LootDrops.Reserve((int)AmountOfLootDrops);

	int SpawnedItems = 0;
	int CurrentCategory = 0;
	while (SpawnedItems < AmountOfLootDrops && CurrentCategory < LootTierData->LootPackageCategoryMinArray.Num())
	{
		for (int j = 0; j < LootTierData->LootPackageCategoryMinArray[CurrentCategory]; j++)
			SetupLDSForPackage(LootDrops, LootTierData->LootPackage, CurrentCategory, TierGroup, WorldLevel);

		CurrentCategory++;
		SpawnedItems += LootTierData->LootPackageCategoryMinArray[CurrentCategory];
	}

	/*std::map<UFortWorldItemDefinition*, int32> AmmoMap;
	for (auto& Item : LootDrops)
		if (Item.ItemDefinition->IsA<UFortWeaponRangedItemDefinition>() && !Item.ItemDefinition->IsStackable() && ((UFortWorldItemDefinition*)Item.ItemDefinition)->GetAmmoWorldItemDefinition_BP())
		{
			auto AmmoDefinition = ((UFortWorldItemDefinition*)Item.ItemDefinition)->GetAmmoWorldItemDefinition_BP();
			int i = 0;
			auto AmmoEntry = LootDrops.Search([&](FFortItemEntry& Entry)
				{
					if (AmmoMap[AmmoDefinition] > 0 && i < AmmoMap[AmmoDefinition])
					{
						i++;
						return false;
					}
					AmmoMap[AmmoDefinition]++;
					return Entry.ItemDefinition == AmmoDefinition;
				});

			if (AmmoEntry)
				continue;

			FFortLootPackageData* Group = nullptr;
			static auto AmmoSmall = FName(L"WorldList.AthenaAmmoSmall");
			for (auto const& Val : LPGroupsAll)
				if (Val->LootPackageID == AmmoSmall && Val->ItemDefinition == AmmoDefinition)
				{
					Group = Val;
					break;
				}

			if (Group)
				LootDrops.Add(*Inventory::MakeItemEntry(AmmoDefinition, Group->Count, 0));
		}*/

	return LootDrops;
}


bool SpawnLootHook(ABuildingContainer* Container)
{
	if (Container->bAlreadySearched)
		return false;

	auto& RealTierGroup = Container->SearchLootTierGroup;
	for (const auto& [OldTierGroup, RedirectedTierGroup] : ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->RedirectAthenaLootTierGroups)
	{
		if (OldTierGroup == Container->SearchLootTierGroup)
		{
			RealTierGroup = RedirectedTierGroup;
			break;
		}
	}

	for (auto& LootDrop : UFortLootPackage::ChooseLootForContainer(RealTierGroup))
		AFortInventory::SpawnPickup(Container, *LootDrop);

	Container->bAlreadySearched = true;
	Container->OnRep_bAlreadySearched();
	Container->SearchBounceData.SearchAnimationCount++;
	Container->BounceContainer();

	return true;
}

void UFortLootPackage::Hook()
{
	if (VersionInfo.FortniteVersion >= 11.00)
		Utils::Hook(FindSpawnLoot(), SpawnLootHook);
}
