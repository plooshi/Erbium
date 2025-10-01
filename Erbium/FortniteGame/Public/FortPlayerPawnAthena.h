#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

class AFortPlayerPawnAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerPawnAthena);

    DEFINE_PROP(CurrentWeapon, AActor*); // everything breaks if we include FortWeapon.h so
    DEFINE_PROP(Controller, AActor*);
    DEFINE_PROP(IncomingPickups, TArray<AActor*>);

    DEFINE_FUNC(BeginSkydiving, void);
    DEFINE_FUNC(SetHealth, void);
    DEFINE_FUNC(SetShield, void);
    DEFINE_FUNC(EquipWeaponDefinition, AActor*);

    DefUHookOg(ServerHandlePickup_);
    DefUHookOg(ServerHandlePickupInfo);
    DefHookOg(bool, FinishedTargetSpline, void*);

    InitHooks;
};