#include "pch.h"
#include "../Public/LateGame.h"
#include "../Public/Utils.h"
#include "../../FortniteGame/Public/FortInventory.h"

FLateGameItem LateGame::GetShotgun()
{
    static UEAllocatedVector<FLateGameItem> Shotguns
    {
    };

    if (Shotguns.size() == 0)
    {
        if (VersionInfo.FortniteVersion < 6.31)
        {
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_UC_Ore_T03.WID_Shotgun_Standard_Athena_UC_Ore_T03")));
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_C_Ore_T03.WID_Shotgun_Standard_Athena_C_Ore_T03")));
        }
        else if (VersionInfo.FortniteVersion >= 19.00 && VersionInfo.FortniteVersion <= 22.40)
        {
            if (VersionInfo.FortniteVersion >= 21.30)
            {
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/DaisyWeaponGameplay/Items/Weapons/Shotguns/OverLoadShotgun/WID_Shotgun_OverLoad_Athena_SR.WID_Shotgun_OverLoad_Athena_SR")));
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/DaisyWeaponGameplay/Items/Weapons/Shotguns/OverLoadShotgun/WID_Shotgun_OverLoad_Athena_VR.WID_Shotgun_OverLoad_Athena_VR")));
            }
            if (VersionInfo.FortniteVersion <= 22)
            {
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/FlipperGameplay/Items/Weapons/BurstShotgun/WID_Shotgun_CoreBurst_Athena_SR.WID_Shotgun_CoreBurst_Athena_SR")));
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/FlipperGameplay/Items/Weapons/BurstShotgun/WID_Shotgun_CoreBurst_Athena_VR.WID_Shotgun_CoreBurst_Athena_VR")));
            }
            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")));
            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")));
        }
        else if (VersionInfo.FortniteVersion > 22.40 && VersionInfo.FortniteVersion <= 26.30)
        {
            if (VersionInfo.FortniteVersion < 24)
            {
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterPumpShotgun/WID_Shotgun_MusterPump_Athena_UC.WID_Shotgun_MusterPump_Athena_UC")));
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterPumpShotgun/WID_Shotgun_MusterPump_Athena_VR.WID_Shotgun_MusterPump_Athena_VR")));
            }
            else if (VersionInfo.FortniteVersion >= 25.11)
            {
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/ChronoWeaponGameplay/Items/ChronoShotgun/WID_Shotgun_Chrono_Athena_SR.WID_Shotgun_Chrono_Athena_SR")));
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/ChronoWeaponGameplay/Items/ChronoShotgun/WID_Shotgun_Chrono_Athena_VR.WID_Shotgun_Chrono_Athena_VR")));
            }

            if (VersionInfo.FortniteVersion >= 24 && VersionInfo.FortniteVersion < 26.00)
            {
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/RadicalWeaponsGameplay/Weapons/RadicalShotgunPump/WID_Shotgun_RadicalPump_Athena_SR.WID_Shotgun_RadicalPump_Athena_SR")));
                Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/RadicalWeaponsGameplay/Weapons/RadicalShotgunPump/WID_Shotgun_RadicalPump_Athena_VR.WID_Shotgun_RadicalPump_Athena_VR")));
            }
            
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/Exotics/WID_Shotgun_Breach_Athena_X.WID_Shotgun_Breach_Athena_X")));

            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")));
            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")));
        }
        else if (VersionInfo.FortniteVersion >= 28.00)
        {
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_SR.WID_Shotgun_Pump_Paprika_Athena_SR")));
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_VR.WID_Shotgun_Pump_Paprika_Athena_VR")));

            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")));
            //Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")));
        }
        else
        {
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03"))); 
            Shotguns.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03"))); 
        }
    }

    return Shotguns[rand() % Shotguns.size()];
}

FLateGameItem LateGame::GetAssaultRifle()
{
    static UEAllocatedVector<FLateGameItem> AssaultRifles
    {
    };

    if (AssaultRifles.size() == 0)
    {
        if (VersionInfo.FortniteVersion > 22.40 && VersionInfo.FortniteVersion <= 26.30)
        {
            if (VersionInfo.FortniteVersion < 25.00)
            {
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterScopedAR/WID_Assault_MusterScoped_Athena_SR.WID_Assault_MusterScoped_Athena_SR")));
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterScopedAR/WID_Assault_MusterScoped_Athena_VR.WID_Assault_MusterScoped_Athena_VR")));
            }
            else if (VersionInfo.FortniteVersion >= 26.00)
            {
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/HopscotchWeaponsGameplay/Items/FlipmagAR/WID_Assault_FlipMag_Athena_SR.WID_Assault_FlipMag_Athena_SR")));
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/HopscotchWeaponsGameplay/Items/FlipmagAR/WID_Assault_FlipMag_Athena_VR.WID_Assault_FlipMag_Athena_VR")));
            }
            else if (VersionInfo.FortniteVersion >= 25.00)
            {
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/ChronoWeaponGameplay/Items/PanRifle/WID_Assault_Chrono_Pan_Rifle_Athena_SR.WID_Assault_Chrono_Pan_Rifle_Athena_SR")));
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/ChronoWeaponGameplay/Items/PanRifle/WID_Assault_Chrono_Pan_Rifle_Athena_VR.WID_Assault_Chrono_Pan_Rifle_Athena_VR")));
            }

            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")));
            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")));
        }
        else if (VersionInfo.FortniteVersion >= 19.00 && VersionInfo.FortniteVersion <= 22.40)
        {
            if (VersionInfo.FortniteVersion < 21 && VersionInfo.FortniteVersion != 20.00)
            {
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterScopedAR/WID_Assault_RedDotAR_Athena_SR.WID_Assault_RedDotAR_Athena_SR")));
                AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/MusterCoreWeapons/Items/Weapons/MusterScopedAR/WID_Assault_RedDotAR_Athena_VR.WID_Assault_RedDotAR_Athena_VR")));
            }

            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreAR/WID_Assault_CoreAR_Athena_SR.WID_Assault_CoreAR_Athena_SR")));
            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/FlipperGameplay/Items/Weapons/CoreAR/WID_Assault_CoreAR_Athena_VR.WID_Assault_CoreAR_Athena_VR")));
        }
        else
        {
            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")));
            AssaultRifles.push_back(FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")));
        }
    }

    return AssaultRifles[rand() % AssaultRifles.size()];
}


FLateGameItem LateGame::GetUtility()
{
    static UEAllocatedVector<FLateGameItem> Utilities
    {
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03")), // bolt
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_VR_Ore_T03.WID_Pistol_Scavenger_Athena_VR_Ore_T03")) // tac smg
    };

    static bool bAdded = false;
    if (!bAdded)
    {
        bAdded = true;

        if (VersionInfo.FortniteVersion >= 24.20 && VersionInfo.FortniteVersion < 25)
        {
            auto ODMGear = FLateGameItem(3, FindObject<UFortWeaponRangedItemDefinition>(L"/DryBox/Items/NyxGlass/AGID_NyxGlass.AGID_NyxGlass"));
            Utilities.push_back(ODMGear);
        }
    }

    return Utilities[rand() % Utilities.size()];
}

FLateGameItem LateGame::GetHeal()
{
    static UEAllocatedVector<FLateGameItem> Heals
    {
        FLateGameItem(3, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")), // big pots
        FLateGameItem(6, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")) // minis
    };

    static bool bAdded = false;
    if (!bAdded)
    {
        bAdded = true;

        auto ChugSplash = FLateGameItem(6, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco"));
        auto SlurpJuice = FLateGameItem(2, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff"));

        if (ChugSplash.Item)
            Heals.push_back(ChugSplash);

        if (SlurpJuice.Item)
            Heals.push_back(SlurpJuice);

        if (VersionInfo.FortniteVersion >= 19 && VersionInfo.FortniteVersion < 27)
        {
            auto MedMist = FLateGameItem(2, FindObject<UFortItemDefinition>(L"/FlipperGameplay/Items/HealSpray/WID_Athena_HealSpray.WID_Athena_HealSpray"));

            Heals.push_back(MedMist);
        }
    }

    return Heals[rand() % Heals.size()];
}

const UFortItemDefinition* LateGame::GetAmmo(EAmmoType AmmoType)
{
    static UEAllocatedVector<const UFortItemDefinition*> Ammos
    {
        FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
        FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells"),
        FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
        FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets"),
        FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy")
    };

    return Ammos[(uint8)AmmoType];
}

const UFortItemDefinition* LateGame::GetResource(EFortResourceType ResourceType)
{
    static UEAllocatedVector<const UFortItemDefinition*> Resources
    {
        FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData"),
        FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData"),
        FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData")
    };

    return Resources[(uint8)ResourceType];
}
