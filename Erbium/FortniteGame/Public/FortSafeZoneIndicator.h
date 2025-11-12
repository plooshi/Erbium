#pragma once
#include "../../pch.h"

struct FFortSafeZoneDamageInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortSafeZoneDamageInfo);

    DEFINE_STRUCT_PROP(Damage, float);
    DEFINE_STRUCT_PROP(bPercentageBasedDamage, bool);
};

struct FFortSafeZonePhaseInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortSafeZonePhaseInfo);

    uint8_t Padding[0x60];
    DEFINE_STRUCT_PROP(Radius, float);
    DEFINE_STRUCT_PROP(WaitTime, float);
    DEFINE_STRUCT_PROP(ShrinkTime, float);
    DEFINE_STRUCT_PROP(PlayerCap, int);
    DEFINE_STRUCT_PROP(TimeBetweenStormCapDamage, float);
    DEFINE_STRUCT_PROP(StormCapDamagePerTick, float);
    DEFINE_STRUCT_PROP(StormCampingIncrementTimeAfterDelay, float);
    DEFINE_STRUCT_PROP(StormCampingInitialDelayTime, float);
    DEFINE_STRUCT_PROP(MegaStormGridCellThickness, int);
    DEFINE_STRUCT_PROP(UsePOIStormCenter, bool);
    DEFINE_STRUCT_PROP(Center, FVector);
    DEFINE_STRUCT_PROP(DamageInfo, FFortSafeZoneDamageInfo);
};

class AFortSafeZoneIndicatorFuture : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortSafeZoneIndicatorFuture);

    DEFINE_PROP(NextNextCenter, FVector);
    DEFINE_PROP(NextNextRadius, float);
};

class AFortSafeZoneIndicator : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortSafeZoneIndicator);

    DEFINE_PROP(SafeZoneStartShrinkTime, float);
    DEFINE_PROP(SafeZoneFinishShrinkTime, float);
    DEFINE_PROP(CurrentPhase, int);
    DEFINE_PROP(PhaseCount, int);
    DEFINE_PROP(SafeZonePhases, TArray<FFortSafeZonePhaseInfo>);
    DEFINE_PROP(LastCenter, FVector); // old builds
    DEFINE_PROP(LastRadius, float);
    DEFINE_PROP(PreviousCenter, FVector);
    DEFINE_PROP(PreviousRadius, float);
    DEFINE_PROP(NextCenter, FVector);
    DEFINE_PROP(NextRadius, float);
    DEFINE_PROP(NextMegaStormGridCellThickness, int);
    DEFINE_PROP(NextNextCenter, FVector);
    DEFINE_PROP(NextNextRadius, float);
    DEFINE_PROP(NextNextMegaStormGridCellThickness, int);
    DEFINE_PROP(CurrentDamageInfo, FFortSafeZoneDamageInfo);
    DEFINE_PROP(OnSafeZonePhaseChanged, TMulticastInlineDelegate<void()>);
    DEFINE_PROP(FutureReplicator, AFortSafeZoneIndicatorFuture*);
    DEFINE_PROP(SafezoneStateChangedDelegate, TMulticastInlineDelegate<void(AFortSafeZoneIndicator*, uint8_t)>);

    DEFINE_FUNC(GetSafeZoneCenter, FVector);
    DEFINE_FUNC(OnRep_CurrentPhase, void);
    DEFINE_FUNC(OnRep_PhaseCount, void);
    DEFINE_FUNC(OnRep_CurrentDamageInfo, void);
    DEFINE_FUNC(OnSafeZoneStateChange, void);
};