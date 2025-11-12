#pragma once
#include "../../pch.h"
#include "../../Engine/Public/CurveTable.h"
#include "FortInventory.h"
#include "FortGameMode.h"

struct FItemsToGive final
{
public:
    UFortWorldItemDefinition* ItemToDrop;
    FScalableFloat NumberToGive;
};

class AFortAthenaMutator : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaMutator);

    DEFINE_PROP(CachedGameMode, AFortGameMode*);
};

class AFortAthenaMutator_GiveItemsAtGamePhaseStep : public AFortAthenaMutator
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaMutator_GiveItemsAtGamePhaseStep);

    DEFINE_PROP(PhaseToGiveItems, uint8);
    DEFINE_PROP(ItemsToGive, TArray<FItemsToGive>);

    DefUHookOg(OnGamePhaseStepChanged);

    InitPostLoadHooks;
};

class AFortAthenaMutator_GiveItemsAtGamePhase : public AFortAthenaMutator
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaMutator_GiveItemsAtGamePhase);

    DEFINE_PROP(PhaseToGiveItems, uint8);
    DEFINE_PROP(ItemsToGive, TArray<FItemsToGive>);

    DefUHookOg(OnGamePhaseChanged);

    InitPostLoadHooks;
};