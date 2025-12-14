#include "pch.h"
#include "../Public/LateGame.h"
#include "../Public/Utils.h"
#include "../../FortniteGame/Public/FortInventory.h"

FLateGameItem LateGame::GetShotgun()
{
    static UEAllocatedVector<FLateGameItem> Shotguns
    {
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")), // pump
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")), // pump

        /*
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_UR_Boss.WID_Shotgun_Pump_Paprika_Athena_UR_Boss")), // Hammer pump Boss
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_R.WID_Shotgun_Pump_Paprika_Athena_R")), // rare
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_VR.WID_Shotgun_Pump_Paprika_Athena_VR")), // Epic
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_SR.WID_Shotgun_Pump_Paprika_Athena_SR")), // Gold

        
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/SunRoseWeaponsGameplay/Items/Weapons/CerberusSG/WID_Shotgun_Break_Cerberus_Athena_UR.WID_Shotgun_Break_Cerberus_Athena_UR")), // gatekeeper boss
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/SunRoseWeaponsGameplay/Items/Weapons/CerberusSG/WID_Shotgun_Break_Cerberus_Athena_SR.WID_Shotgun_Break_Cerberus_Athena_SR")), // gatekeeper epic
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/SunRoseWeaponsGameplay/Items/Weapons/CerberusSG/WID_Shotgun_Break_Cerberus_Athena_R.WID_Shotgun_Break_Cerberus_Athena_R")), // gatekeeper rare
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/SunRoseWeaponsGameplay/Items/Weapons/CerberusSG/WID_Shotgun_Break_Cerberus_Athena_SR.WID_Shotgun_Break_Cerberus_Athena_SR")), // gatekeeper gold
        /**/
    };

    return Shotguns[rand() % Shotguns.size()];
}

FLateGameItem LateGame::GetAssaultRifle()
{
    static UEAllocatedVector<FLateGameItem> AssaultRifles
    {
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")), // scar 
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")), // scar

        /*

        // enforcer ARs
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Infantry/HitscanWIDs/WID_Assault_Paprika_Infantry_Athena_HS_UR_Boss.WID_Assault_Paprika_Infantry_Athena_HS_UR_Boss")), // Mythic enforcer AR
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Infantry/HitscanWIDs/WID_Assault_Paprika_Infantry_Athena_HS_SR.WID_Assault_Paprika_Infantry_Athena_HS_SR")), // gold
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Infantry/HitscanWIDs/WID_Assault_Paprika_Infantry_Athena_HS_VR.WID_Assault_Paprika_Infantry_Athena_HS_VR")), // epic
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Infantry/HitscanWIDs/WID_Assault_Paprika_Infantry_Athena_HS_R.WID_Assault_Paprika_Infantry_Athena_HS_R")), // blue


        // Ch5 Scar
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_DPS/HitscanWIDs/WID_Assault_Paprika_DPS_Athena_HS_UR_Boss.WID_Assault_Paprika_DPS_Athena_HS_UR_Boss")), // Boss / mythic
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_DPS/HitscanWIDs/WID_Assault_Paprika_DPS_Athena_HS_VR.WID_Assault_Paprika_DPS_Athena_HS_VR")),// gold
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_DPS/HitscanWIDs/WID_Assault_Paprika_DPS_Athena_HS_SR.WID_Assault_Paprika_DPS_Athena_HS_SR")), // epic
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_DPS/HitscanWIDs/WID_Assault_Paprika_DPS_Athena_HS_R.WID_Assault_Paprika_DPS_Athena_HS_R")), // blue

        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Heavy/HitscanWIDs/WID_Assault_Paprika_Heavy_Athena_HS_UR_Boss.WID_Assault_Paprika_Heavy_Athena_HS_UR_Boss")), // boss / mythic
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Heavy/HitscanWIDs/WID_Assault_Paprika_Heavy_Athena_HS_VR.WID_Assault_Paprika_Heavy_Athena_HS_VR")), // gold
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Heavy/HitscanWIDs/WID_Assault_Paprika_Heavy_Athena_HS_SR.WID_Assault_Paprika_Heavy_Athena_HS_SR")), // epic
        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_Heavy/HitscanWIDs/WID_Assault_Paprika_Heavy_Athena_HS_R.WID_Assault_Paprika_Heavy_Athena_HS_R")), // blue

        FLateGameItem(1,FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaAR_DPS/WID_Assault_Paprika_HITSCAN.WID_Assault_Paprika_HITSCAN")), // IDK
        /**/
    };

    return AssaultRifles[rand() % AssaultRifles.size()];
}


FLateGameItem LateGame::GetSniper()
{
    static UEAllocatedVector<FLateGameItem> Snipers
    {
        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03")), // bolt


        FLateGameItem(1, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_VR_Ore_T03.WID_Pistol_Scavenger_Athena_VR_Ore_T03")), // tac smg

  
        /*
		FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_DPS/HitscanWIDs/WID_SMG_Paprika_DPS_Athena_HS_UR_Boss.WID_SMG_Paprika_DPS_Athena_HS_UR_Boss")), // hyper smg mythic
		FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_DPS/HitscanWIDs/WID_SMG_Paprika_DPS_Athena_HS_SR.WID_SMG_Paprika_DPS_Athena_HS_SR")), // hyper smg gold
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_DPS/HitscanWIDs/WID_SMG_Paprika_DPS_Athena_HS_VR.WID_SMG_Paprika_DPS_Athena_HS_VR")), // hyper smg epic
			   FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_DPS/HitscanWIDs/WID_SMG_Paprika_DPS_Athena_HS_R.WID_SMG_Paprika_DPS_Athena_HS_R")), // hyper smg blue

        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_Burst/HitscanWIDs/WID_SMG_Paprika_Burst_Athena_HS_SR.WID_SMG_Paprika_Burst_Athena_HS_SR")), // thunder busrt gold
		FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_Burst/HitscanWIDs/WID_SMG_Paprika_Burst_Athena_HS_VR.WID_SMG_Paprika_Burst_Athena_HS_VR")), // thunder busrt epic
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaSMG_Burst/HitscanWIDs/WID_SMG_Paprika_Burst_Athena_HS_R.WID_SMG_Paprika_Burst_Athena_HS_R")) // thunder busrt blue
		/**/
    };

    return Snipers[rand() % Snipers.size()];
}

FLateGameItem LateGame::GetHeal()
{
    static UEAllocatedVector<FLateGameItem> Heals
    {
        FLateGameItem(3, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")), // big pots
        FLateGameItem(6, FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")), // minis

        /*
        FLateGameItem(6, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco")),
        FLateGameItem(2, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff")),
        FLateGameItem(3, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/Flopper/WID_Athena_Flopper.WID_Athena_Flopper")),
        FLateGameItem(6, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")),
        FLateGameItem(3, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")),
        FLateGameItem(2, FindObject<UFortWeaponRangedItemDefinition>(L"/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff")),
        FLateGameItem(1, FindObject<UFortWeaponRangedItemDefinition>(L"/FlipperGameplay/Items/HealSpray/WID_Athena_HealSpray.WID_Athena_HealSpray"))
        /**/
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
        FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData"),


    };

    return Resources[(uint8)ResourceType];
}
