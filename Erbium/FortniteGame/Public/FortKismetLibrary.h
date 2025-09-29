#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortInventory.h"
#include "BuildingSMActor.h"

class UFortKismetLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortKismetLibrary);

    DEFINE_STATIC_FUNC(UpdatePlayerCustomCharacterPartsVisualization, void);
    //DEFINE_STATIC_FUNC(K2_GetResourceItemDefinition, UFortItemDefinition*);

	static UFortItemDefinition* K2_GetResourceItemDefinition(EFortResourceType Type)
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
};