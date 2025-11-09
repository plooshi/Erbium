#pragma once
#include "../../pch.h"

class UFortLevelSaveComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortLevelSaveComponent);

    DEFINE_PROP(RestrictedPlotDefinition, const UObject*);
    DEFINE_PROP(AccountIdOfOwner, FUniqueNetIdRepl);
    DEFINE_PROP(bIsLoaded, bool);
    DEFINE_PROP(bLoadPlaysetFromPlot, bool);
    DEFINE_PROP(bAutoLoadFromRestrictedPlotDefinition, bool);
};

class UFortPlaysetItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlaysetItemDefinition);
};

class UPlaysetLevelStreamComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UPlaysetLevelStreamComponent);

    DEFINE_PROP(CurrentPlayset, const UFortPlaysetItemDefinition*);
    DEFINE_BITFIELD_PROP(bAutoLoadLevel);
    DEFINE_BITFIELD_PROP(bAutoActivate);

    DEFINE_FUNC(SetPlayset, void);
    DEFINE_FUNC(OnRep_CurrentPlayset, void);
};

class AFortVolume : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortVolume);

    DEFINE_BITFIELD_PROP(bNeverAllowSaving);
    DEFINE_PROP(VolumeState, uint8);
    DEFINE_PROP(CurrentPlayset, const UFortPlaysetItemDefinition*);

    DEFINE_FUNC(OnRep_VolumeState, void);
    DEFINE_FUNC(OnRep_CurrentPlayset, void);
};