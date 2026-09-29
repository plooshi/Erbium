#pragma once
#include "../../../pch.h"

class FBotController;

// Phase 3: bot cosmetics.
//
// Kept completely separate from AI: a loadout is chosen and applied once at spawn and never read by
// any behavior.
//
// Where cosmetics come from: the running build's own asset manager. Each slot maps to a primary asset
// type (AthenaCharacter, AthenaBackpack, ...); UKismetSystemLibrary::GetPrimaryAssetIdList lists every
// id the build knows. So the catalogue is automatically correct per version -- a cosmetic that does
// not exist in a build simply is not in its list -- and no hard-coded per-version ID table can go
// stale. Only chosen cosmetics are loaded (on demand, validated, cached).

enum class EBotCosmeticSlot : uint8
{
    Outfit,
    Backpack,
    Pickaxe,
    Glider,
    Contrail,
    Count,
};

enum class EBotCosmeticMode : uint8
{
    Random,   // Any valid cosmetic of this slot.
    Specific, // One of SpecificIds (random among them); falls back to Random if none are valid.
    None,     // Leave the slot empty / game default.
};

struct FBotCosmeticSlotSettings
{
    EBotCosmeticMode Mode = EBotCosmeticMode::Random;

    // Cosmetic ids as used by the game and by sites like fortnite.gg, e.g. "CID_028_Athena_Commando_F",
    // "BID_004_BlackKnight", "Pickaxe_ID_011_Medieval", "Glider_ID_001". Case-insensitive.
    std::vector<std::wstring> SpecificIds;
};

struct FBotCosmeticSettings
{
    static inline bool bEnabled = true;

    // Slot order follows EBotCosmeticSlot.
    static inline FBotCosmeticSlotSettings Slots[(int)EBotCosmeticSlot::Count] = {
        { EBotCosmeticMode::Random, {} }, // Outfit
        { EBotCosmeticMode::Random, {} }, // Backpack
        { EBotCosmeticMode::Random, {} }, // Pickaxe
        { EBotCosmeticMode::Random, {} }, // Glider
        { EBotCosmeticMode::None, {} },   // Contrail
    };

    // Optional overrides read once from this file in the server's working directory, if it exists.
    // Format, one slot per line ("#" starts a comment):
    //   Outfit=Random
    //   Backpack=None
    //   Pickaxe=Pickaxe_ID_011_Medieval, Pickaxe_ID_012_District
    static inline const wchar_t* SettingsFile = L"BotCosmetics.txt";
};

// What one bot wears. Holds weak references: cosmetics are regular assets the engine may unload.
struct FBotCosmeticLoadout
{
    TWeakObjectPtr<UObject> Items[(int)EBotCosmeticSlot::Count];
    std::string Ids[(int)EBotCosmeticSlot::Count]; // for logs / debug output; empty = none

    UObject* Get(EBotCosmeticSlot Slot) const { return (UObject*)Items[(int)Slot].Get(); }
    std::string Describe() const;
};

namespace BotCosmetics
{
    const char* ToString(EBotCosmeticSlot Slot);

    // Picks a loadout according to FBotCosmeticSettings. Never fails: slots that cannot be filled stay empty.
    FBotCosmeticLoadout ChooseLoadout();

    // Picks a specific cosmetic for one slot (e.g. from a cheat). Returns false if the id is unknown or invalid.
    bool ChooseSpecific(FBotCosmeticLoadout& Loadout, EBotCosmeticSlot Slot, const std::wstring& Id);

    // Applies the loadout to the bot's pawn (visible parts) and, for player-controller bots, to the
    // controller's cosmetic loadout (pickaxe granted by GiveStartingItems, glider used when skydiving).
    // Returns false if the outfit could not be applied (the caller then uses the default appearance).
    bool ApplyLoadout(FBotController& Bot, const FBotCosmeticLoadout& Loadout);

    // Number of known cosmetics for a slot on this build (builds the catalogue on first use).
    int GetAvailableCount(EBotCosmeticSlot Slot);

    // Re-reads the settings file and clears the catalogue (e.g. after editing BotCosmetics.txt).
    // Game thread only.
    void Reload();

    // Any thread (GUI): performed on the game thread before the next loadout is chosen.
    void RequestReload();

    // Why the last bot got the default appearance, or "none". Safe from the GUI thread.
    const char* GetLastProblem();

    // Catalogue size as last built on the game thread (0 before the first bot). Read-only, for the GUI.
    int GetKnownCount(EBotCosmeticSlot Slot);
}
