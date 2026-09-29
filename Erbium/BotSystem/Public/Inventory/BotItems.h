#pragma once
#include "../../../pch.h"

class UFortItemDefinition;

// Item categories the AI reasons about (spec 2.5).
enum class EBotItemCategory : uint8
{
    Weapon,   // ranged gun that uses separate ammo
    Melee,    // pickaxe
    Ammo,
    Healing,  // restores health
    Shield,   // restores shield
    Utility,  // other usable items (grenades, gadgets, ...)
    Resource, // building materials
    Tool,     // building pieces / edit tool (never dropped or scored)
    Other,
};

// Rough engagement range of a weapon, used to pick the right gun for the distance.
enum class EBotWeaponRange : uint8
{
    Close,  // shotguns, SMGs
    Medium, // rifles, pistols
    Long,   // snipers
};

// Configurable priorities (spec 2.5 "the exact priority system should be configurable").
struct FBotItemPriorities
{
    static inline float Weapon = 100.f;
    static inline float Shield = 60.f;
    static inline float Healing = 55.f;
    static inline float Ammo = 40.f;   // only for guns the bot carries
    static inline float Utility = 25.f;
    static inline float Resource = 15.f;
    static inline float RarityBonus = 12.f; // per rarity tier above common

    // Name fragments (lower case) used to classify consumables and weapon ranges. Item definitions do
    // not expose "heals health" / "is a shotgun" as reflected data across all builds, but their asset
    // names are stable (WID_Athena_Medkit, Athena_Shields, WID_Shotgun_..., WID_Sniper_...).
    static inline std::vector<std::string> HealingTokens = { "bandage", "medkit", "heal", "chug", "apple", "mushroom", "banana", "floppa" };
    static inline std::vector<std::string> ShieldTokens = { "shield", "slurp", "chug", "flopper", "splash" };
    static inline std::vector<std::string> CloseRangeTokens = { "shotgun", "smg", "submachine", "pistol_auto", "minigun" };
    static inline std::vector<std::string> LongRangeTokens = { "sniper", "dmr", "huntingrifle", "hunting" };
};

// What a consumable does, for bots whose (virtual) inventory cannot use items through the game's
// item abilities. Values are the standard Battle Royale ones; matched by name token, first match wins.
// Native-inventory bots never use this: the game applies the real item effect.
struct FBotConsumableEffect
{
    const char* Token;
    float Health;
    float HealthCap;
    float Shield;
    float ShieldCap;
    float UseTime;
};

struct FBotConsumableEffects
{
    static inline std::vector<FBotConsumableEffect> Table = {
        { "bandage", 15.f, 75.f, 0.f, 0.f, 3.5f },
        { "medkit", 100.f, 100.f, 0.f, 0.f, 10.f },
        { "shieldsmall", 0.f, 0.f, 25.f, 50.f, 2.f },
        { "shield", 0.f, 0.f, 50.f, 100.f, 5.f },
        { "slurp", 75.f, 100.f, 75.f, 100.f, 2.f },
        { "chug", 100.f, 100.f, 100.f, 100.f, 15.f },
    };
};

namespace BotItems
{
    // nullptr if the item is not in FBotConsumableEffects::Table.
    const FBotConsumableEffect* GetConsumableEffect(const UFortItemDefinition* Definition);

    EBotItemCategory Classify(const UFortItemDefinition* Definition);
    EBotWeaponRange GetWeaponRange(const UFortItemDefinition* Definition);

    // The ammo item a gun uses (nullptr for anything that does not use separate ammo).
    const UFortItemDefinition* GetAmmoDefinition(const UFortItemDefinition* Definition);

    // Clip size from the weapon's stats (0 if unknown).
    int GetClipSize(const UFortItemDefinition* Definition);

    // 0 = common ... higher is better. Uses the reflected Rarity (or Tier) property.
    int GetRarity(const UFortItemDefinition* Definition);

    // Whether the item takes one of the five primary quickbar slots (same rule as Erbium's inventory).
    bool UsesPrimarySlot(const UFortItemDefinition* Definition);

    const char* ToString(EBotItemCategory Category);
    std::string GetName(const UFortItemDefinition* Definition);
}
