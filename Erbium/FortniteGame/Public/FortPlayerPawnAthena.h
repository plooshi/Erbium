#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

class AFortPlayerPawnAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerPawnAthena);

    DEFINE_FUNC(BeginSkydiving, void);
    DEFINE_FUNC(SetHealth, void);
    DEFINE_FUNC(SetShield, void);
    DEFINE_FUNC(EquipWeaponDefinition, AActor*);
};