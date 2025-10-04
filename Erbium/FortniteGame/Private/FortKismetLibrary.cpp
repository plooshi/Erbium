#include "pch.h"
#include "../Public/FortKismetLibrary.h"
#include "../Public/FortInventory.h"
#include "../Public/FortLootPackage.h"

void UFortKismetLibrary::K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
	class UObject* WorldContextObject;
	class UFortItemDefinition* ItemDefinition;
	int32 NumberToSpawn;
	FVector Position;
	FVector Direction;
	int32 OverrideMaxStackCount;
	bool bToss;
	bool bRandomRotation;
	bool bBlockedFromAutoPickup;
	int32 PickupInstigatorHandle;
	uint8 SourceType;
	uint8 Source;
	class AFortPlayerControllerAthena* OptionalOwnerPC;
	bool bPickupOnlyRelevantToOwner;
	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&ItemDefinition);
	Stack.StepCompiledIn(&NumberToSpawn);
	Stack.StepCompiledIn(&Position);
	Stack.StepCompiledIn(&Direction);
	Stack.StepCompiledIn(&OverrideMaxStackCount);
	Stack.StepCompiledIn(&bToss);
	Stack.StepCompiledIn(&bRandomRotation);
	Stack.StepCompiledIn(&bBlockedFromAutoPickup);
	Stack.StepCompiledIn(&PickupInstigatorHandle);
	Stack.StepCompiledIn(&SourceType);
	Stack.StepCompiledIn(&Source);
	Stack.StepCompiledIn(&OptionalOwnerPC);
	Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
	Stack.IncrementCode();

	*Ret = AFortInventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, 0, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
}


bool bHasItemVariantGuid2 = false;
bool bHasItemLevel = false;
bool bHasPickupInstigatorHandle = false;
bool bHasbUseItemPickupAnalyticEvent = false;
void UFortKismetLibrary::GiveItemToInventoryOwner(UObject* Object, FFrame& Stack)
{
	TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
	UFortItemDefinition* ItemDefinition;
	FGuid ItemVariantGuid;
	int32 NumberToGive;
	bool bNotifyPlayer;
	int32 ItemLevel = -1;
	int32 PickupInstigatorHandle = 0;
	bool bUseItemPickupAnalyticEvent = false;
	Stack.StepCompiledIn(&InventoryOwner);
	Stack.StepCompiledIn(&ItemDefinition);
	if (bHasItemVariantGuid2)
		Stack.StepCompiledIn(&ItemVariantGuid);
	Stack.StepCompiledIn(&NumberToGive);
	Stack.StepCompiledIn(&bNotifyPlayer);
	if (bHasItemLevel)
		Stack.StepCompiledIn(&ItemLevel);
	if (bHasPickupInstigatorHandle)
		Stack.StepCompiledIn(&PickupInstigatorHandle);
	if (bHasbUseItemPickupAnalyticEvent)
		Stack.StepCompiledIn(&bUseItemPickupAnalyticEvent);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)InventoryOwner.ObjectPointer;
	auto ItemEntry = AFortInventory::MakeItemEntry(ItemDefinition, NumberToGive, ItemLevel);
	PlayerController->InternalPickup(ItemEntry);
	free(ItemEntry);
}


bool bHasItemVariantGuid = false;
void UFortKismetLibrary::K2_RemoveItemFromPlayer(UObject* Context, FFrame& Stack, int32* Ret)
{
	AFortPlayerControllerAthena* PlayerController;
	UFortItemDefinition* ItemDefinition;
	FGuid ItemVariantGuid{};
	int32 AmountToRemove;
	bool bForceRemoval;
	Stack.StepCompiledIn(&PlayerController);
	Stack.StepCompiledIn(&ItemDefinition);
	if (bHasItemVariantGuid)
		Stack.StepCompiledIn(&ItemVariantGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	if (!PlayerController)
	{
		*Ret = 0;
		return;
	}

	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemDefinition == ItemDefinition; }, FFortItemEntry::Size());
	if (!itemEntry)
	{
		*Ret = 0;
		return;
	}

	auto RemoveCount = AmountToRemove;
	itemEntry->Count -= AmountToRemove;
	static auto GadgetClass = FindClass("FortGadgetItemDefinition");
	if (itemEntry->Count <= 0 || ItemDefinition->IsA(GadgetClass))
	{
		RemoveCount += itemEntry->Count;
		PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
	}
	else
		PlayerController->WorldInventory->UpdateEntry(*itemEntry);

	*Ret = RemoveCount;
}

void UFortKismetLibrary::K2_RemoveItemFromPlayerByGuid(UObject* Context, FFrame& Stack, int32* Ret)
{
	class AFortPlayerControllerAthena* PlayerController;
	struct FGuid ItemGuid;
	int32 AmountToRemove;
	bool bForceRemoval;
	Stack.StepCompiledIn(&PlayerController);
	Stack.StepCompiledIn(&ItemGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == ItemGuid; }, FFortItemEntry::Size());
	if (!ItemEntry)
	{
		*Ret = 0;
		return;
	}

	auto RemoveCount = AmountToRemove;
	ItemEntry->Count -= AmountToRemove;

	static auto GadgetClass = FindClass("FortGadgetItemDefinition");
	if (ItemEntry->Count <= 0 || ItemEntry->ItemDefinition->IsA(GadgetClass)) {
		RemoveCount += ItemEntry->Count;
		PlayerController->WorldInventory->Remove(ItemEntry->ItemGuid);
	}
	else
		PlayerController->WorldInventory->UpdateEntry(*ItemEntry);

	*Ret = RemoveCount;
	return;
}

void UFortKismetLibrary::SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
	UObject* WorldContextObject;
	FSpawnItemVariantParams Params;
	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&Params);
	Stack.IncrementCode();

	*Ret = AFortInventory::SpawnPickup(Params.Position, Params.WorldItemDefinition, Params.NumberToSpawn, 0, Params.SourceType, Params.Source, nullptr, Params.bToss, Params.bRandomRotation);
}


void UFortKismetLibrary::PickLootDrops(UObject* Object, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	FName TierGroupName;
	int32 WorldLevel;
	int32 ForcedLootTier;
	FGameplayTagContainer OptionalLootTags;

	Stack.StepCompiledIn(&WorldContextObject);
	auto& OutLootToDrop = Stack.StepCompiledInRef<TArray<FFortItemEntry>>();
	Stack.StepCompiledIn(&TierGroupName);
	Stack.StepCompiledIn(&WorldLevel);
	Stack.StepCompiledIn(&ForcedLootTier);
	Stack.StepCompiledIn(&OptionalLootTags);
	Stack.IncrementCode();

	auto LootDrops = UFortLootPackage::ChooseLootForContainer(TierGroupName, ForcedLootTier, WorldLevel);

	for (auto& LootDrop : LootDrops)
	{
		OutLootToDrop.Add(*LootDrop, FFortItemEntry::Size());
		free(LootDrop);
	}

	*Ret = LootDrops.Num() > 0;
}

void UFortKismetLibrary::Hook()
{
	auto GiveItemToInventoryOwnerFn = GetDefaultObj()->GetFunction("GiveItemToInventoryOwner");
	if (GiveItemToInventoryOwnerFn)
		for (auto& Param : GiveItemToInventoryOwnerFn->GetParams().NameOffsetMap)
		{
			if (Param.Name == "ItemVariantGuid")
				bHasItemVariantGuid2 = true;
			else if (Param.Name == "ItemLevel")
				bHasItemLevel = true;
			else if (Param.Name == "PickupInstigatorHandle")
				bHasPickupInstigatorHandle = true;
			else if (Param.Name == "bUseItemPickupAnalyticEvent")
				bHasbUseItemPickupAnalyticEvent = true;
		}
	Utils::ExecHook(GiveItemToInventoryOwnerFn, GiveItemToInventoryOwner);
	auto K2_RemoveItemFromPlayerFn = GetDefaultObj()->GetFunction("K2_RemoveItemFromPlayer");
	if (K2_RemoveItemFromPlayerFn)
		for (auto& Param : K2_RemoveItemFromPlayerFn->GetParams().NameOffsetMap)
		{
			if (Param.Name == "ItemVariantGuid")
			{
				bHasItemVariantGuid = true;
				break;
			}
		}
	Utils::ExecHook(K2_RemoveItemFromPlayerFn, K2_RemoveItemFromPlayer);
	Utils::ExecHook(GetDefaultObj()->GetFunction("K2_RemoveItemFromPlayerByGuid"), K2_RemoveItemFromPlayerByGuid);
	Utils::ExecHook(GetDefaultObj()->GetFunction("K2_SpawnPickupInWorld"), K2_SpawnPickupInWorld);
	Utils::ExecHook(GetDefaultObj()->GetFunction("SpawnItemVariantPickupInWorld"), SpawnItemVariantPickupInWorld);
	Utils::ExecHook(GetDefaultObj()->GetFunction("PickLootDrops"), PickLootDrops);
}