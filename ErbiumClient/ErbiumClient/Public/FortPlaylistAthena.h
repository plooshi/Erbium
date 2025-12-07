#pragma once
#include "../../pch.h"
#include "../../../Erbium/Engine/Public/CurveTable.h"

struct FUIExtension final
{
public:
    uint8                                         Slot;                                              // 0x0000(0x0001)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
    uint8                                         Pad_1[0x7];                                        // 0x0001(0x0007)(Fixing Size After Last Property [ Dumper-7 ])
    TSoftClassPtr<class UClass>                   WidgetClass;                                       // 0x0008(0x0028)(Edit, DisableEditOnInstance, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

struct EPlaylistUIExtensionSlot
{
public:
    UENUM_COMMON_MEMBERS(EPlaylistUIExtensionSlot);

    DEFINE_ENUM_PROP(Primary);
};

struct EUIExtensionSlot
{
public:
    UENUM_COMMON_MEMBERS(EUIExtensionSlot);

    DEFINE_ENUM_PROP(Primary);
};

class UFortPlaylistAthena : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlaylistAthena);

    DEFINE_PROP(bRespawnInAir, bool);
    DEFINE_PROP(RespawnHeight, FScalableFloat);
    DEFINE_PROP(RespawnTime, FScalableFloat);
    DEFINE_PROP(RespawnType, uint8);
    DEFINE_PROP(bAllowJoinInProgress, bool);
    DEFINE_PROP(UIExtensions, TArray<FUIExtension>);
    DEFINE_PROP(bForceRespawnLocationInsideOfVolume, bool);
};