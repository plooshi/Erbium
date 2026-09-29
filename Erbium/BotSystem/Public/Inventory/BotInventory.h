#pragma once
#include "../../../pch.h"
#include "../../../FortniteGame/Public/FortInventory.h"
#include "BotItems.h"

class FBotController;

// FGuid::operator== in the SDK takes a non-const reference; this works on const guids.
inline bool SameGuid(const FGuid& A, const FGuid& B)
{
    return A.A == B.A && A.B == B.B && A.C == B.C && A.D == B.D;
}

inline bool IsZeroGuid(const FGuid& Guid)
{
    return Guid.A == 0 && Guid.B == 0 && Guid.C == 0 && Guid.D == 0;
}

struct FBotItem
{
    const UFortItemDefinition* Definition = nullptr;
    int Count = 0;
    int LoadedAmmo = 0;
    FGuid Guid{};
    EBotItemCategory Category = EBotItemCategory::Other;
};

// Where a bot's items live.
//
// Native: a real AFortInventory, changed only through Erbium's own inventory functions (GiveItem,
// Remove, UpdateEntry). Used by player-controller bots (their WorldInventory) and by AI controllers
// that implement Fortnite's inventory-owner interface (e.g. FortAthenaAIBotController).
//
// Virtual: a list kept by the bot system. Used by the plain engine AIController, which has no
// inventory-owner interface -- giving it a native inventory would route item callbacks into code that
// requires one. Weapons are still equipped on the pawn through the game (EquipWeaponDefinition);
// only the bookkeeping (counts, reserve ammo) is local.
class IBotInventoryStorage
{
public:
    virtual ~IBotInventoryStorage() = default;

    virtual const char* GetName() const = 0;
    virtual std::vector<FBotItem> GetItems() const = 0;
    virtual bool Add(const UFortItemDefinition* Definition, int Count, int LoadedAmmo) = 0;
    // Count < 0 removes the whole stack.
    virtual bool Remove(const FGuid& Guid, int Count) = 0;
    virtual void SetLoadedAmmo(const FGuid& Guid, int LoadedAmmo) = 0;
    virtual AFortInventory* GetNativeInventory() const { return nullptr; }
};

// Per-bot inventory: storage + the decisions from spec 2.6 (what is worth picking up, what to drop
// when full, which weapon to hold). Kept separate from combat: combat asks for "best weapon for this
// distance" and "reload", it never edits the inventory itself.
class FBotInventory
{
public:
    static constexpr int PrimarySlots = 5;

    explicit FBotInventory(FBotController& InBot);

    // Picks the storage backend. Native when a usable AFortInventory is given.
    void Initialize(AFortInventory* NativeInventory);
    bool IsInitialized() const { return Storage != nullptr; }
    const char* GetStorageName() const { return Storage ? Storage->GetName() : "None"; }

    std::vector<FBotItem> GetItems() const;
    int CountOf(const UFortItemDefinition* Definition) const;
    int PrimarySlotsUsed() const;

    // Reserve ammo for a gun (not counting what is loaded).
    int GetReserveAmmo(const UFortItemDefinition* Weapon) const;

    // How much the bot wants this item (0 = not interested). Accounts for what it already carries and
    // for full primary slots (then only worth it if it beats the worst primary item).
    float ScoreItem(const UFortItemDefinition* Definition, int Count) const;

    // Adds an item (stacking where the game would). Frees a primary slot first if needed by dropping
    // the lowest-value primary item next to the bot. Returns false if it could not be added.
    bool Give(const UFortItemDefinition* Definition, int Count, int LoadedAmmo);

    bool Consume(const FGuid& Guid, int Count);

    // Drops a stack as a pickup at the bot's feet (or all stacks on death).
    void Drop(const FGuid& Guid);
    void DropAll(const FVector& Location);

    // Best gun for a distance (cm), preferring loaded/reloadable ones. nullptr if none.
    const FBotItem* GetBestWeaponFor(float Distance, std::vector<FBotItem>& Items) const;
    const FBotItem* GetBestOf(EBotItemCategory Category, std::vector<FBotItem>& Items) const;

    // Equips through the pawn, like ServerExecuteInventoryItem does for players.
    bool Equip(const FBotItem& Item);
    // Guid of the item the pawn is holding (zero guid if nothing).
    FGuid GetEquippedGuid() const;

    // Virtual storage only: moves reserve ammo into the equipped gun (the game does this itself for
    // native inventories). Returns the new loaded amount, or -1 if not applicable.
    int ReloadVirtual(const FBotItem& Weapon);
    void SyncLoadedAmmo(const FGuid& Guid, int LoadedAmmo);

    AFortInventory* GetNativeInventory() const { return Storage ? Storage->GetNativeInventory() : nullptr; }

private:
    float BaseScore(const UFortItemDefinition* Definition, int Count, const std::vector<FBotItem>& Items) const;
    const FBotItem* WorstPrimaryItem(std::vector<FBotItem>& Items) const;

    FBotController& Bot;
    std::unique_ptr<IBotInventoryStorage> Storage;
};
