#pragma once
#include "../../pch.h"

struct FCreativeLoadedLinkData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FCreativeLoadedLinkData);
    
    DEFINE_STRUCT_PROP(CreatorName, FString);
    DEFINE_STRUCT_PROP(SupportCode, FString);
    DEFINE_STRUCT_PROP(Version, int32);
};

class UFortLevelSaveComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortLevelSaveComponent);

    DEFINE_PROP(RestrictedPlotDefinition, const UObject*);
    DEFINE_PROP(AccountIdOfOwner, FUniqueNetIdRepl);
    DEFINE_PROP(bIsLoaded, bool);
    DEFINE_PROP(bLoadPlaysetFromPlot, bool);
    DEFINE_PROP(bAutoLoadFromRestrictedPlotDefinition, bool);
    DEFINE_PROP(LoadedLinkData, FCreativeLoadedLinkData);
};

class UFortPlaysetItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlaysetItemDefinition);
};

class UFortCreativeRealEstatePlotItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortCreativeRealEstatePlotItemDefinition);

    DEFINE_PROP(BasePlayset, TSoftObjectPtr<UFortPlaysetItemDefinition>);
};

struct FFortPlaysetStreamingData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortPlaysetStreamingData);

    DEFINE_STRUCT_BITFIELD_PROP(bValid);
};

class UPlaysetLevelStreamComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UPlaysetLevelStreamComponent);

    DEFINE_PROP(CurrentPlayset, const UFortPlaysetItemDefinition*);
    DEFINE_BITFIELD_PROP(bAutoLoadLevel);
    DEFINE_BITFIELD_PROP(bAutoActivate);
    DEFINE_PROP(ClientPlaysetData, FFortPlaysetStreamingData);

    DEFINE_FUNC(SetPlayset, void);
    DEFINE_FUNC(OnRep_CurrentPlayset, void);
    DEFINE_FUNC(OnRep_ClientPlaysetData, void);
};

class AFortVolume : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortVolume);

    DEFINE_BITFIELD_PROP(bNeverAllowSaving);
    DEFINE_PROP(VolumeState, uint8);
    DEFINE_PROP(CurrentPlayset, const UFortPlaysetItemDefinition*);
    DEFINE_PROP(LinkedPortals, TArray<AActor*>);

    DEFINE_FUNC(OnRep_VolumeState, void);
    DEFINE_FUNC(OnRep_CurrentPlayset, void);
};