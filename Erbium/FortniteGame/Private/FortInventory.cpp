#include "pch.h"
#include "../Public/FortInventory.h"
#include "../Public/FortPlayerPawnAthena.h"

// OnItemInstanceRemoved is always + 1
inline uint32_t FindOnItemInstanceAddedVft()
{
    uint32_t OnItemInstanceAddedVft = 0;

    if (OnItemInstanceAddedVft == 0)
    {
        auto sRef = Memcury::Scanner::FindStringRef("AmmoCountPistol").Get();

        uint64_t AmmoDef__OnItemInstanceAdded = 0;
        for (int i = 0; i < 1000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
            {
                AmmoDef__OnItemInstanceAdded = sRef - i;
                break;
            }
            else if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x53)
            {
                AmmoDef__OnItemInstanceAdded = sRef - i;
                break;
            }
        }

        auto AmmoDefObj = DefaultObjImpl("FortAmmoItemDefinition");


        for (int i = 0; i < 0x100; i++)
            if (uint64_t(AmmoDefObj->Vft[i]) == AmmoDef__OnItemInstanceAdded)
                return OnItemInstanceAddedVft = i;
    }

    return OnItemInstanceAddedVft;
}

UFortWorldItem* AFortInventory::GiveItem(UFortItemDefinition* Def, int Count, int LoadedAmmo, int Level, bool ShowPickupNoti, bool updateInventory, int PhantomReserveAmmo)
{
    if (!this || !Def || !Count)
        return nullptr;
    UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, Level);
    Item->SetOwningControllerForTemporaryItem(Owner);
    Item->ItemEntry.LoadedAmmo = LoadedAmmo;
    if (Item->ItemEntry.HasPhantomReserveAmmo())
        Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;

    this->Inventory.ReplicatedEntries.Add(Item->ItemEntry, FFortItemEntry::Size());
    this->Inventory.ItemInstances.Add(Item);

    static auto OnItemInstanceAddedVft = FindOnItemInstanceAddedVft();
    if (OnItemInstanceAddedVft)
    {
        static auto InterfaceClass = FindClass("FortInventoryOwnerInterface");
        ((bool(*)(UFortItemDefinition*, const IInterface*, UFortWorldItem*, uint8)) Def->Vft[OnItemInstanceAddedVft])(Def, Owner->GetInterface(InterfaceClass), Item, 1);
    }

    if (updateInventory)
        Update(&Item->ItemEntry);
    return Item;
}

UFortWorldItem* AFortInventory::GiveItem(FFortItemEntry entry, int Count, bool ShowPickupNoti, bool updateInventory)
{
    if (Count == -1)
        Count = entry.Count;

    return GiveItem(entry.ItemDefinition, Count, entry.LoadedAmmo, entry.Level, ShowPickupNoti, updateInventory, entry.HasPhantomReserveAmmo() ? entry.PhantomReserveAmmo : 0);
}

void AFortInventory::Update(FFortItemEntry* Entry)
{
    bRequiresLocalUpdate = true;
    HandleInventoryLocalUpdate();

    return Entry ? Inventory.MarkItemDirty(*Entry) : Inventory.MarkArrayDirty();
}


void AFortInventory::Remove(FGuid Guid)
{
    auto ItemEntryIdx = Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& entry) { return entry.ItemGuid == Guid; }, FFortItemEntry::Size());
    if (ItemEntryIdx != -1)
        Inventory.ReplicatedEntries.Remove(ItemEntryIdx, FFortItemEntry::Size());

    auto ItemInstanceIdx = Inventory.ItemInstances.SearchIndex([&](UFortWorldItem* entry) { return entry->ItemEntry.ItemGuid == Guid; });
    auto ItemInstance = Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
        { return entry->ItemEntry.ItemGuid == Guid; });

    auto Instance = ItemInstance ? *ItemInstance : nullptr;
    if (ItemInstanceIdx != -1)
        Inventory.ItemInstances.Remove(ItemInstanceIdx);

    static auto OnItemInstanceRemovedVft = FindOnItemInstanceAddedVft() + 1;
    if (ItemInstanceIdx != -1 && OnItemInstanceRemovedVft)
    {
        static auto InterfaceClass = FindClass("FortInventoryOwnerInterface");
        ((bool(*)(UFortItemDefinition*, const IInterface*, UFortWorldItem*)) Instance->ItemEntry.ItemDefinition->Vft[OnItemInstanceRemovedVft])(Instance->ItemEntry.ItemDefinition, Owner->GetInterface(InterfaceClass), Instance);
    }

    Update(nullptr);
}

FFortRangedWeaponStats* AFortInventory::GetStats(UFortWeaponItemDefinition* Def)
{
    if (!Def || !Def->WeaponStatHandle.DataTable)
        return nullptr;

    auto Val = Def->WeaponStatHandle.DataTable->RowMap.Search([Def](FName& Key, uint8_t* Value) {
        return Def->WeaponStatHandle.RowName == Key && Value;
        });

    return Val ? *(FFortRangedWeaponStats**)Val : nullptr;
}


FFortItemEntry* AFortInventory::MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
{
    FFortItemEntry* ItemEntry = new FFortItemEntry();

    ItemEntry->MostRecentArrayReplicationKey = -1;
    ItemEntry->ReplicationID = -1;
    ItemEntry->ReplicationKey = -1;

    ItemEntry->ItemDefinition = ItemDefinition;
    ItemEntry->Count = Count;
    ItemEntry->Durability = 1.f;
    ItemEntry->GameplayAbilitySpecHandle = FGameplayAbilitySpecHandle(-1);
    ItemEntry->ParentInventory.ObjectIndex = -1;
    ItemEntry->Level = Level;
    if (auto Weapon = ItemDefinition->Cast<UFortWeaponItemDefinition>())
    {
        auto Stats = GetStats(Weapon);
        ItemEntry->LoadedAmmo = Stats->ClipSize;
        if (Weapon->HasbUsesPhantomReserveAmmo() && Weapon->bUsesPhantomReserveAmmo)
            ItemEntry->PhantomReserveAmmo = Stats->InitialClips * Stats->ClipSize;
    }

    return ItemEntry;
}

AFortPickupAthena* AFortInventory::SpawnPickup(FVector Loc, FFortItemEntry& Entry, long long SourceTypeFlag, long long SpawnSource, AFortPlayerPawnAthena* Pawn, int OverrideCount, bool Toss, bool RandomRotation, bool bCombine)
{
    if (!&Entry)
        return nullptr;
    AFortPickupAthena* NewPickup = UWorld::SpawnActor<AFortPickupAthena>(Loc, {});
    if (!NewPickup)
        return nullptr;

    NewPickup->bRandomRotation = RandomRotation;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
    NewPickup->OnRep_PrimaryPickupItemEntry();
    NewPickup->PawnWhoDroppedPickup = Pawn;

    NewPickup->TossPickup(Loc, Pawn, -1, Toss, true, (uint8) SourceTypeFlag, (uint8) SpawnSource);
    NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::GetChest() || SpawnSource == EFortPickupSpawnSource::GetAmmoBox();
    if (NewPickup->bTossedFromContainer)
        NewPickup->OnRep_TossedFromContainer();

    return NewPickup;
}

AFortPickupAthena* AFortInventory::SpawnPickup(FVector Loc, UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo, long long SourceTypeFlag, long long SpawnSource, AFortPlayerPawnAthena* Pawn, bool Toss, bool bRandomRotation)
{
    return SpawnPickup(Loc, *MakeItemEntry(ItemDefinition, Count, 0), SourceTypeFlag, SpawnSource, Pawn, -1, Toss, true, bRandomRotation);
}


AFortPickupAthena* AFortInventory::SpawnPickup(ABuildingContainer* Container, FFortItemEntry& Entry, AFortPlayerPawnAthena* Pawn, int OverrideCount)
{
    if (!&Entry)
        return nullptr;

    auto ContainerLoc = Container->K2_GetActorLocation();
    auto Loc = ContainerLoc + (Container->GetActorForwardVector() * Container->LootSpawnLocation_Athena.X) + (Container->GetActorRightVector() * Container->LootSpawnLocation_Athena.Y) + (Container->GetActorUpVector() * Container->LootSpawnLocation_Athena.Z);
    AFortPickupAthena* NewPickup = UWorld::SpawnActor<AFortPickupAthena>(Loc, {});
    if (!NewPickup)
        return nullptr;

    NewPickup->bRandomRotation = true;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
    NewPickup->OnRep_PrimaryPickupItemEntry();
    NewPickup->PawnWhoDroppedPickup = Pawn;
    NewPickup->TossPickup(Loc, Pawn, -1, true, true, EFortPickupSourceTypeFlag::GetContainer(), EFortPickupSpawnSource::GetChest());
    //auto bFloorLoot = Container->IsA<ATiered_Athena_FloorLoot_01_C>() || Container->IsA<ATiered_Athena_FloorLoot_Warmup_C>();
    //UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, NewPickup, 1, 0, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, false);
    NewPickup->bTossedFromContainer = true;
    NewPickup->OnRep_TossedFromContainer();


    return NewPickup;
}


bool AFortInventory::IsPrimaryQuickbar(UFortItemDefinition* ItemDefinition)
{
    static auto MeleeClass = FindClass("FortWeaponMeleeItemDefinition");
    static auto ResourceClass = FindClass("FortResourceItemDefinition");
    static auto AmmoClass = FindClass("FortAmmoItemDefinition");
    static auto TrapClass = FindClass("FortTrapItemDefinition");
    static auto BuildingClass = FindClass("FortBuildingItemDefinition");
    static auto EditToolClass = FindClass("FortEditToolItemDefinition");

    return ItemDefinition->IsA(MeleeClass) || ItemDefinition->IsA(ResourceClass) || ItemDefinition->IsA(AmmoClass) || ItemDefinition->IsA(TrapClass) || ItemDefinition->IsA(BuildingClass) || ItemDefinition->IsA(EditToolClass) || ItemDefinition->bForceIntoOverflow ? false : true;
}


void AFortInventory::UpdateEntry(FFortItemEntry& Entry)
{
    auto repEnt = Inventory.ReplicatedEntries.Search([&](FFortItemEntry& item)
        { return item.ItemGuid == Entry.ItemGuid; }, FFortItemEntry::Size());
    if (repEnt)
        __movsb((PBYTE)repEnt, (const PBYTE)&Entry, FFortItemEntry::Size());

    auto ent = Inventory.ItemInstances.Search([&](UFortWorldItem* item)
        { return item->ItemEntry.ItemGuid == Entry.ItemGuid; });
    if (ent)
        __movsb((PBYTE)&(*ent)->ItemEntry, (const PBYTE)&Entry, FFortItemEntry::Size());

    Update(&Entry);
}