#pragma once
#include "pch.h"
#include "../../FortniteGame/Public/BuildingSMActor.h"
#include "../../FortniteGame/Public/FortInventory.h"

struct FLateGameItem
{
    uint32 Count;
    const UFortItemDefinition* Item;
};

enum class EAmmoType : uint8
{
    Assault = 0,
    Shotgun = 1,
    Submachine = 2,
    Rocket = 3,
    Sniper = 4
};

class LateGame
{
public:
    static FLateGameItem GetShotgun();
    static FLateGameItem GetAssaultRifle();
    static FLateGameItem GetSniper();
    static FLateGameItem GetHeal();

    static const UFortItemDefinition* GetAmmo(EAmmoType);
    static const UFortItemDefinition* GetResource(EFortResourceType);
};