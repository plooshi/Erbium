#include "pch.h"
#include "../../Public/Inventory/BotInventory.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../Public/Combat/BotWeapons.h"
#include "../../../FortniteGame/Public/FortKismetLibrary.h"
#include "../../../FortniteGame/Public/FortWeapon.h"
#include <objbase.h> // CoCreateGuid (excluded by WIN32_LEAN_AND_MEAN)

// --- Native storage: a real AFortInventory ---------------------------------------------------------

class FBotNativeInventoryStorage final : public IBotInventoryStorage
{
public:
    explicit FBotNativeInventoryStorage(AFortInventory* InInventory)
        : Inventory(InInventory)
    {
    }

    const char* GetName() const override { return "Native"; }

    AFortInventory* GetNativeInventory() const override
    {
        auto Result = Inventory.Get();
        return Result && !Result->bActorIsBeingDestroyed ? Result : nullptr;
    }

    std::vector<FBotItem> GetItems() const override
    {
        std::vector<FBotItem> Items;
        auto Native = GetNativeInventory();
        if (!Native)
            return Items;

        auto& Entries = Native->Inventory.ReplicatedEntries;
        for (int i = 0; i < Entries.Num(); i++)
        {
            auto& Entry = Entries.Get(i, FFortItemEntry::Size());
            if (!Entry.ItemDefinition)
                continue;

            FBotItem Item;
            Item.Definition = Entry.ItemDefinition;
            Item.Count = Entry.Count;
            Item.LoadedAmmo = Entry.LoadedAmmo;
            Item.Guid = Entry.ItemGuid;
            Items.push_back(Item);
        }

        return Items;
    }

    bool Add(const UFortItemDefinition* Definition, int Count, int LoadedAmmo) override
    {
        auto Native = GetNativeInventory();
        if (!Native)
            return false;

        // Stack onto an existing entry where the game would (ammo, materials, consumables).
        auto MaxStack = Definition->GetMaxStackSize();
        if (MaxStack > 1)
            if (auto Entry = FindEntry(Native, [&](FFortItemEntry& E) { return E.ItemDefinition == Definition && E.Count < MaxStack; }))
            {
                SetCount(Native, *Entry, (std::min)(Entry->Count + Count, (int)MaxStack));
                return true;
            }

        return Native->GiveItem(Definition, Count, LoadedAmmo) != nullptr;
    }

    bool Remove(const FGuid& Guid, int Count) override
    {
        auto Native = GetNativeInventory();
        auto Entry = Native ? FindEntry(Native, [&](FFortItemEntry& E) { return SameGuid(E.ItemGuid, Guid); }) : nullptr;
        if (!Entry)
            return false;

        if (Count < 0 || Count >= Entry->Count)
            Native->Remove(Guid);
        else
            SetCount(Native, *Entry, Entry->Count - Count);

        return true;
    }

    void SetLoadedAmmo(const FGuid& Guid, int LoadedAmmo) override
    {
        auto Native = GetNativeInventory();
        if (auto Entry = Native ? FindEntry(Native, [&](FFortItemEntry& E) { return SameGuid(E.ItemGuid, Guid); }) : nullptr)
        {
            Entry->LoadedAmmo = LoadedAmmo;
            if (auto Instance = FindInstance(Native, Guid))
                Instance->ItemEntry.LoadedAmmo = LoadedAmmo;
            Native->UpdateEntry(*Entry);
        }
    }

private:
    template <typename Predicate>
    static FFortItemEntry* FindEntry(AFortInventory* Native, Predicate&& Pred)
    {
        return Native->Inventory.ReplicatedEntries.Search(Pred, FFortItemEntry::Size());
    }

    static UFortWorldItem* FindInstance(AFortInventory* Native, const FGuid& Guid)
    {
        auto Found = Native->Inventory.ItemInstances.Search([&](UFortWorldItem* Item) { return Item && SameGuid(Item->ItemEntry.ItemGuid, Guid); });
        return Found ? *Found : nullptr;
    }

    static void SetCount(AFortInventory* Native, FFortItemEntry& Entry, int Count)
    {
        Entry.Count = Count;
        if (auto Instance = FindInstance(Native, Entry.ItemGuid))
            Instance->ItemEntry.Count = Count;
        Native->UpdateEntry(Entry);
    }

    TWeakObjectPtr<AFortInventory> Inventory;
};

// --- Virtual storage: bookkeeping only ---------------------------------------------------------------

class FBotVirtualInventoryStorage final : public IBotInventoryStorage
{
public:
    const char* GetName() const override { return "Virtual"; }

    std::vector<FBotItem> GetItems() const override { return Items; }

    bool Add(const UFortItemDefinition* Definition, int Count, int LoadedAmmo) override
    {
        auto MaxStack = Definition->GetMaxStackSize();
        if (MaxStack > 1)
            for (auto& Item : Items)
                if (Item.Definition == Definition && Item.Count < MaxStack)
                {
                    Item.Count = (std::min)(Item.Count + Count, (int)MaxStack);
                    return true;
                }

        FBotItem Item;
        Item.Definition = Definition;
        Item.Count = Count;
        Item.LoadedAmmo = LoadedAmmo;
        CoCreateGuid((GUID*)&Item.Guid);
        Items.push_back(Item);
        return true;
    }

    bool Remove(const FGuid& Guid, int Count) override
    {
        for (size_t i = 0; i < Items.size(); i++)
        {
            if (!SameGuid(Items[i].Guid, Guid))
                continue;

            if (Count < 0 || Count >= Items[i].Count)
                Items.erase(Items.begin() + i);
            else
                Items[i].Count -= Count;
            return true;
        }

        return false;
    }

    void SetLoadedAmmo(const FGuid& Guid, int LoadedAmmo) override
    {
        for (auto& Item : Items)
            if (SameGuid(Item.Guid, Guid))
                Item.LoadedAmmo = LoadedAmmo;
    }

private:
    std::vector<FBotItem> Items;
};

// --- FBotInventory -----------------------------------------------------------------------------------

// Value of an item independent of what the bot carries (used to rank what to drop).
static float StaticValue(const FBotItem& Item)
{
    float Base = 0.f;
    switch (Item.Category)
    {
    case EBotItemCategory::Weapon:
        Base = FBotItemPriorities::Weapon;
        break;
    case EBotItemCategory::Shield:
        Base = FBotItemPriorities::Shield;
        break;
    case EBotItemCategory::Healing:
        Base = FBotItemPriorities::Healing;
        break;
    case EBotItemCategory::Utility:
        Base = FBotItemPriorities::Utility;
        break;
    default:
        Base = 1.f;
        break;
    }

    return Base + BotItems::GetRarity(Item.Definition) * FBotItemPriorities::RarityBonus;
}

FBotInventory::FBotInventory(FBotController& InBot)
    : Bot(InBot)
{
}

void FBotInventory::Initialize(AFortInventory* NativeInventory)
{
    if (NativeInventory)
        Storage = std::make_unique<FBotNativeInventoryStorage>(NativeInventory);
    else
        Storage = std::make_unique<FBotVirtualInventoryStorage>();

    BOT_VERBOSE(BOT_CATEGORY_LOOT, "Bot #%u inventory: %s", Bot.GetId(), Storage->GetName());
}

std::vector<FBotItem> FBotInventory::GetItems() const
{
    if (!Storage)
        return {};

    auto Items = Storage->GetItems();
    for (auto& Item : Items)
        Item.Category = BotItems::Classify(Item.Definition);

    return Items;
}

int FBotInventory::CountOf(const UFortItemDefinition* Definition) const
{
    int Count = 0;
    for (auto& Item : GetItems())
        if (Item.Definition == Definition)
            Count += Item.Count;

    return Count;
}

int FBotInventory::PrimarySlotsUsed() const
{
    int Used = 0;
    for (auto& Item : GetItems())
        if (BotItems::UsesPrimarySlot(Item.Definition))
            Used++;

    return Used;
}

int FBotInventory::GetReserveAmmo(const UFortItemDefinition* Weapon) const
{
    auto Ammo = BotItems::GetAmmoDefinition(Weapon);
    return Ammo ? CountOf(Ammo) : 0;
}

float FBotInventory::BaseScore(const UFortItemDefinition* Definition, int Count, const std::vector<FBotItem>& Items) const
{
    auto Category = BotItems::Classify(Definition);
    auto Rarity = BotItems::GetRarity(Definition);

    switch (Category)
    {
    case EBotItemCategory::Weapon:
    {
        float Score = FBotItemPriorities::Weapon + Rarity * FBotItemPriorities::RarityBonus;
        auto Range = BotItems::GetWeaponRange(Definition);
        bool bHasWeapon = false;

        for (auto& Item : Items)
        {
            if (Item.Category != EBotItemCategory::Weapon)
                continue;

            bHasWeapon = true;
            if (BotItems::GetWeaponRange(Item.Definition) == Range && BotItems::GetRarity(Item.Definition) >= Rarity)
                Score *= 0.2f; // already carries an equal or better gun for this range
        }

        return bHasWeapon ? Score : Score * 2.f;
    }
    case EBotItemCategory::Ammo:
    {
        // Only ammo for guns the bot carries; less interesting the more it already has.
        for (auto& Item : Items)
        {
            if (Item.Category != EBotItemCategory::Weapon || BotItems::GetAmmoDefinition(Item.Definition) != Definition)
                continue;

            auto Clip = (std::max)(BotItems::GetClipSize(Item.Definition), 1);
            auto Reserve = CountOf(Definition);
            return FBotItemPriorities::Ammo * (Reserve < Clip * 2 ? 1.5f : (Reserve > Clip * 6 ? 0.2f : 1.f));
        }
        return 0.f;
    }
    case EBotItemCategory::Healing:
    case EBotItemCategory::Shield:
    case EBotItemCategory::Utility:
    {
        auto Held = CountOf(Definition);
        if (Held >= Definition->GetMaxStackSize())
            return 0.f;

        auto Base = Category == EBotItemCategory::Healing ? FBotItemPriorities::Healing : (Category == EBotItemCategory::Shield ? FBotItemPriorities::Shield : FBotItemPriorities::Utility);
        return (Base + Rarity * FBotItemPriorities::RarityBonus * 0.5f) * (Held > 0 ? 0.5f : 1.f);
    }
    case EBotItemCategory::Resource:
        return FBotItemPriorities::Resource;
    default:
        return 0.f;
    }
}

const FBotItem* FBotInventory::WorstPrimaryItem(std::vector<FBotItem>& Items) const
{
    const FBotItem* Worst = nullptr;
    for (auto& Item : Items)
        if (BotItems::UsesPrimarySlot(Item.Definition) && (!Worst || StaticValue(Item) < StaticValue(*Worst)))
            Worst = &Item;

    return Worst;
}

float FBotInventory::ScoreItem(const UFortItemDefinition* Definition, int Count) const
{
    if (!Storage || !Definition)
        return 0.f;

    auto Items = GetItems();
    auto Score = BaseScore(Definition, Count, Items);
    if (Score <= 0.f)
        return 0.f;

    // Needs a new primary slot while all five are taken: only worth it as an upgrade.
    bool bStacks = Definition->GetMaxStackSize() > 1 && CountOf(Definition) > 0;
    if (BotItems::UsesPrimarySlot(Definition) && !bStacks && PrimarySlotsUsed() >= PrimarySlots)
    {
        auto Worst = WorstPrimaryItem(Items);
        FBotItem Candidate;
        Candidate.Definition = Definition;
        Candidate.Category = BotItems::Classify(Definition);

        if (!Worst || StaticValue(Candidate) <= StaticValue(*Worst) * 1.1f)
            return 0.f;

        Score -= StaticValue(*Worst) * 0.5f;
    }

    return (std::max)(Score, 0.f);
}

bool FBotInventory::Give(const UFortItemDefinition* Definition, int Count, int LoadedAmmo)
{
    if (!Storage || !Definition || Count <= 0)
        return false;

    // AI-controller bots keep building materials with their builder (see FBotBuilder).
    if (BotItems::Classify(Definition) == EBotItemCategory::Resource && !Storage->GetNativeInventory())
    {
        for (int Material = 0; Material < (int)EBotBuildMaterial::Count; Material++)
            if (UFortKismetLibrary::K2_GetResourceItemDefinition((EFortResourceType)Material) == Definition)
            {
                Bot.GetBuilder().AddMaterials((EBotBuildMaterial)Material, Count);
                return true;
            }
        return false;
    }

    bool bStacks = Definition->GetMaxStackSize() > 1 && CountOf(Definition) > 0;
    if (BotItems::UsesPrimarySlot(Definition) && !bStacks && PrimarySlotsUsed() >= PrimarySlots)
    {
        auto Items = GetItems();
        auto Worst = WorstPrimaryItem(Items);
        if (!Worst)
            return false;

        BOT_VERBOSE(BOT_CATEGORY_LOOT, "Bot #%u drops %s to make room.", Bot.GetId(), BotItems::GetName(Worst->Definition).c_str());
        Drop(Worst->Guid);
    }

    return Storage->Add(Definition, Count, LoadedAmmo);
}

bool FBotInventory::Consume(const FGuid& Guid, int Count)
{
    return Storage && Storage->Remove(Guid, Count);
}

void FBotInventory::Drop(const FGuid& Guid)
{
    auto Pawn = Bot.GetPawn();
    if (!Storage || !Pawn)
        return;

    for (auto& Item : GetItems())
    {
        if (!SameGuid(Item.Guid, Guid))
            continue;

        if (Item.Definition->CanBeDropped())
            AFortInventory::SpawnPickup(Pawn->K2_GetActorLocation(), Item.Definition, Item.Count, Item.LoadedAmmo, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), Pawn);

        Storage->Remove(Guid, -1);
        return;
    }
}

void FBotInventory::DropAll(const FVector& Location)
{
    if (!Storage)
        return;

    for (auto& Item : GetItems())
    {
        if (!Item.Definition->CanBeDropped() || Item.Category == EBotItemCategory::Tool || Item.Category == EBotItemCategory::Melee)
            continue;

        AFortInventory::SpawnPickup(Location, Item.Definition, Item.Count, Item.LoadedAmmo, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetPlayerElimination(), Bot.GetPawn());
        Storage->Remove(Item.Guid, -1);
    }
}

const FBotItem* FBotInventory::GetBestWeaponFor(float Distance, std::vector<FBotItem>& Items) const
{
    auto Wanted = Distance < 1000.f ? EBotWeaponRange::Close : (Distance < 4500.f ? EBotWeaponRange::Medium : EBotWeaponRange::Long);

    const FBotItem* Best = nullptr;
    float BestScore = -1.f;

    for (auto& Item : Items)
    {
        if (Item.Category != EBotItemCategory::Weapon)
            continue;

        bool bHasAmmo = Item.LoadedAmmo > 0 || GetReserveAmmo(Item.Definition) > 0;
        if (!bHasAmmo)
            continue;

        auto Range = BotItems::GetWeaponRange(Item.Definition);
        float Score = BotItems::GetRarity(Item.Definition) * 10.f + (Range == Wanted ? 50.f : (abs((int)Range - (int)Wanted) == 1 ? 20.f : 0.f)) + (Item.LoadedAmmo > 0 ? 15.f : 0.f);

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = &Item;
        }
    }

    return Best;
}

const FBotItem* FBotInventory::GetBestOf(EBotItemCategory Category, std::vector<FBotItem>& Items) const
{
    const FBotItem* Best = nullptr;
    for (auto& Item : Items)
        if (Item.Category == Category && Item.Count > 0 && (!Best || BotItems::GetRarity(Item.Definition) > BotItems::GetRarity(Best->Definition)))
            Best = &Item;

    return Best;
}

bool FBotInventory::Equip(const FBotItem& Item)
{
    auto Pawn = Bot.GetPawn();
    if (!Pawn || !Item.Definition)
        return false;

    const UFortItemDefinition* WeaponDefinition = Item.Definition;
    if (auto Gadget = Item.Definition->Cast<UFortGadgetItemDefinition>())
        WeaponDefinition = Gadget->GetWeaponItemDefinition();

    if (!WeaponDefinition || !WeaponDefinition->IsA<UFortWeaponItemDefinition>())
        return false;

    // Same call ServerExecuteInventoryItem makes for players.
    FGuid TrackerGuid{};
    bool bDisableEquipAnimation = false;
    auto Weapon = (AFortWeapon*)Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)WeaponDefinition, Item.Guid, TrackerGuid, bDisableEquipAnimation);
    if (!Weapon)
        return false;

    // Virtual storage: the game has no item to read the magazine from, so set it from our record.
    if (!Storage->GetNativeInventory() && BotItems::GetAmmoDefinition(Item.Definition))
        BotWeapons::SetLoadedAmmo(Weapon, Item.LoadedAmmo);

    return true;
}

FGuid FBotInventory::GetEquippedGuid() const
{
    auto Weapon = BotWeapons::GetCurrentWeapon(Bot.GetPawn());
    return Weapon ? Weapon->ItemEntryGuid : FGuid{};
}

void FBotInventory::SyncLoadedAmmo(const FGuid& Guid, int LoadedAmmo)
{
    if (Storage && !Storage->GetNativeInventory())
        Storage->SetLoadedAmmo(Guid, LoadedAmmo);
}

int FBotInventory::ReloadVirtual(const FBotItem& Weapon)
{
    if (!Storage || Storage->GetNativeInventory())
        return -1;

    auto AmmoDefinition = BotItems::GetAmmoDefinition(Weapon.Definition);
    auto Equipped = BotWeapons::GetCurrentWeapon(Bot.GetPawn());
    if (!AmmoDefinition || !Equipped)
        return -1;

    auto Loaded = (std::max)(BotWeapons::GetLoadedAmmo(Equipped), 0);
    auto Needed = BotItems::GetClipSize(Weapon.Definition) - Loaded;
    if (Needed <= 0)
        return Loaded;

    // Take reserve ammo stack by stack.
    for (auto& Item : GetItems())
    {
        if (Needed <= 0)
            break;
        if (Item.Definition != AmmoDefinition)
            continue;

        auto Take = (std::min)(Needed, Item.Count);
        Storage->Remove(Item.Guid, Take);
        Loaded += Take;
        Needed -= Take;
    }

    BotWeapons::SetLoadedAmmo(Equipped, Loaded);
    Storage->SetLoadedAmmo(Weapon.Guid, Loaded);
    return Loaded;
}
