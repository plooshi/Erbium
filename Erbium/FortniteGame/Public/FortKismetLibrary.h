#pragma once
#include "../../pch.h"
#include "FortInventory.h"
#include "BuildingSMActor.h"

struct FSpawnItemVariantParams
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FSpawnItemVariantParams);

	DEFINE_STRUCT_PROP(Position, FVector);
	DEFINE_STRUCT_PROP(position, FVector); // WHY
	DEFINE_STRUCT_PROP(WorldItemDefinition, UFortItemDefinition*);
	DEFINE_STRUCT_PROP(NumberToSpawn, int32);
	DEFINE_STRUCT_PROP(SourceType, uint8);
	DEFINE_STRUCT_PROP(Source, uint8);
	DEFINE_STRUCT_PROP(bToss, bool);
	DEFINE_STRUCT_PROP(bRandomRotation, bool);
};

class UFortKismetLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortKismetLibrary);

    DEFINE_STATIC_FUNC(UpdatePlayerCustomCharacterPartsVisualization, void);
	DEFINE_STATIC_FUNC(TossPickupFromContainer, void);
	DEFINE_STATIC_FUNC(EquipFortAbilitySet, void);
	DEFINE_STATIC_FUNC(FindGroundLocationAt, FVector);
    //DEFINE_STATIC_FUNC(K2_GetResourceItemDefinition, UFortItemDefinition*);

	static const UFortItemDefinition* K2_GetResourceItemDefinition(EFortResourceType Type)
	{
		// exec func doesnt exist on rlly old builds
		static auto WoodItemData = Utils::FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
		static auto StoneItemData = Utils::FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
		static auto MetalItemData = Utils::FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

		if (Type == EFortResourceType::Wood)
			return WoodItemData;
		else if (Type == EFortResourceType::Stone)
			return StoneItemData;
		else if (Type == EFortResourceType::Metal)
			return MetalItemData;

		return nullptr;
	}

	static void K2_SpawnPickupInWorld(UObject*, FFrame&, AFortPickupAthena**);
	static void GiveItemToInventoryOwner(UObject*, FFrame&);
	static void K2_RemoveItemFromPlayer(UObject*, FFrame&, int32*);
	static void K2_RemoveItemFromPlayerByGuid(UObject*, FFrame&, int32*);
	static void SpawnItemVariantPickupInWorld(UObject*, FFrame&, AFortPickupAthena**);
	static void PickLootDrops(UObject*, FFrame&, bool*);

	InitHooks;
};