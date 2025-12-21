#pragma once
#include "../../pch.h"
#include "FortPlayerStateAthena.h"
#include "GameplayTagContainer.h"
#include "../../Engine/Public/CurveTable.h"

enum class EFortResourceType : uint8
{
    Wood = 0,
    Stone = 1,
    Metal = 2,
    Permanite = 3,
    GoldCurrency = 4,
    None = 5
};

class EFortResourceType__Enum
{
public:
    UENUM_COMMON_MEMBERS(EFortResourceType);

    DEFINE_ENUM_PROP(None);
};

struct FTierMeshSets final
{
public:
    int32                                         Tier;                                              // 0x0000(0x0004)(Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
    uint8                                         Pad_4[0x4];                                        // 0x0004(0x0004)(Fixing Size After Last Property [ Dumper-7 ])
    TArray<void*>                                 MeshSets;                                          // 0x0008(0x0010)(Edit, BlueprintVisible, ZeroConstructor, DisableEditOnInstance, NativeAccessSpecifierPublic)
};

inline uint64_t GetSparseClassData_ = 0;

struct FBuildingSMActorClassData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FBuildingSMActorClassData);

    DEFINE_STRUCT_PROP(BuildingResourceAmountOverride, FCurveTableRowHandle);
    DEFINE_STRUCT_PROP(AlternateMeshes, TArray<FTierMeshSets>);
    DEFINE_STRUCT_PROP(EditModePatternData, UObject*);
};

class AFortDecoTool : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortDecoTool);

    DEFINE_PROP(ItemDefinition, UObject*);

    DEFINE_FUNC(ServerSpawnDeco, void);

    DefUHookOg(ServerSpawnDeco_);
    DefUHookOg(ServerCreateBuildingAndSpawnDeco);
};

class UFortContextTrapItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortContextTrapItemDefinition);

    DEFINE_PROP(FloorTrap, UObject*);
    DEFINE_PROP(CeilingTrap, UObject*);
    DEFINE_PROP(WallTrap, UObject*);
    DEFINE_PROP(StairTrap, UObject*);
};

class AFortDecoTool_ContextTrap : public AFortDecoTool
{
public:
    UCLASS_COMMON_MEMBERS(AFortDecoTool_ContextTrap);

    DEFINE_PROP(ContextTrapItemDefinition, UFortContextTrapItemDefinition*);

    DefUHookOg(ServerSpawnDeco_Implementation);
};

class ABuildingProp_LockDevice : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingProp_LockDevice);

    DEFINE_PROP(CurrentLockState, uint8);
    DEFINE_PROP(LockableObject, UObject*);

    DEFINE_FUNC(OnRep_CurrentLockState, void);
};

class ABuildingSMActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingSMActor);

    DEFINE_BITFIELD_PROP(bPlayerPlaced);
    DEFINE_PROP(BuildingResourceAmountOverride, FCurveTableRowHandle);
    DEFINE_PROP(ResourceType, EFortResourceType);
    DEFINE_PROP(Team, uint8);
    DEFINE_PROP(TeamIndex, uint8);
    DEFINE_PROP(EditingPlayer, AFortPlayerStateAthena*);
    DEFINE_BITFIELD_PROP(bDestroyed);
    DEFINE_PROP(CurrentBuildingLevel, int32);
    DEFINE_BITFIELD_PROP(bAllowResourceDrop);
    DEFINE_PROP(AlternateMeshes, TArray<FTierMeshSets>);
    DEFINE_BITFIELD_PROP(bPersistToWorld);
    DEFINE_BITFIELD_PROP(bAutoReleaseCurieContainerOnDestroyed);
    DEFINE_PROP(BuildingReplacementType, uint8_t);
    DEFINE_PROP(ReplacementDestructionReason, uint8_t);
    DEFINE_PROP(OnReplacementDestruction, TMulticastInlineDelegate<void(uint8_t, ABuildingSMActor*)>);
    DEFINE_PROP(AttachedBuildingActors, TArray<ABuildingSMActor*>);
    DEFINE_BITFIELD_PROP(bHiddenDueToTrapPlacement);
    DEFINE_PROP(BuildingType, uint8);
    DEFINE_PROP(EditModePatternData, UObject*);
    DEFINE_BITFIELD_PROP(bIsPlayerBuildable);

    FBuildingSMActorClassData* GetClassData() const
    {
        FBuildingSMActorClassData* (*GetSparseClassDataOG)(UObject*, uint8) = decltype(GetSparseClassDataOG)(GetSparseClassData_);

        return GetSparseClassDataOG(Class, 1);
    }


    DEFINE_FUNC(GetHealth, float);
    DEFINE_FUNC(GetMaxHealth, float);
    DEFINE_FUNC(SetMirrored, void);
    DEFINE_FUNC(InitializeKismetSpawnedBuildingActor, void);
    DEFINE_FUNC(GetHealthPercent, float);
    DEFINE_FUNC(RepairBuilding, void);
    DEFINE_FUNC(SilentDie, void);
    DEFINE_FUNC(OnRep_CurrentBuildingLevel, void);
    DEFINE_STATIC_FUNC(K2_SpawnBuildingActor, ABuildingSMActor*);
    DEFINE_FUNC(AttachBuildingActorToMe, void);
    DEFINE_FUNC(OnRep_EditingPlayer, void);

    DefHookOg(void, OnDamageServer, ABuildingSMActor*, float, FGameplayTagContainer, FVector, __int64, AActor*, AActor*, __int64);
    DefUHookOg(ServerSpawnDeco_Implementation);

    InitPostLoadHooks;
};

class AFortWeap_EditingTool : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortWeap_EditingTool);

    DEFINE_PROP(EditActor, ABuildingSMActor*);

    DEFINE_FUNC(OnRep_EditActor, void);
};