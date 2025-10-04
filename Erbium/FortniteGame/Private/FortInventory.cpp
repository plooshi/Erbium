#include "pch.h"
#include "../Public/FortInventory.h"
#include "../Public/FortPlayerPawnAthena.h"
#include "../Public/FortPlayerControllerAthena.h"

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

UFortWorldItem* AFortInventory::GiveItem(const UFortItemDefinition* Def, int Count, int LoadedAmmo, int Level, bool ShowPickupNoti, bool updateInventory, int PhantomReserveAmmo)
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
    if (OnItemInstanceAddedVft && VersionInfo.FortniteVersion >= 4)
    {
        static auto InterfaceClass = FindClass("FortInventoryOwnerInterface");
        ((bool(*)(const UFortItemDefinition*, const IInterface*, UFortWorldItem*, uint8)) Def->Vft[OnItemInstanceAddedVft])(Def, Owner->GetInterface(InterfaceClass), Item, 1);
    }

    if (updateInventory)
        Update(&Item->ItemEntry);
    return Item;
}

UFortWorldItem* AFortInventory::GiveItem(FFortItemEntry& entry, int Count, bool ShowPickupNoti, bool updateInventory)
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
    if (ItemInstanceIdx != -1 && OnItemInstanceRemovedVft && VersionInfo.FortniteVersion >= 4)
    {
        static auto InterfaceClass = FindClass("FortInventoryOwnerInterface");
        ((bool(*)(const UFortItemDefinition*, const IInterface*, UFortWorldItem*)) Instance->ItemEntry.ItemDefinition->Vft[OnItemInstanceRemovedVft])(Instance->ItemEntry.ItemDefinition, Owner->GetInterface(InterfaceClass), Instance);
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


FFortItemEntry* AFortInventory::MakeItemEntry(const UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
{
    auto ItemEntry = (FFortItemEntry*) malloc(FFortItemEntry::Size());
    __stosb((PBYTE)ItemEntry, 0, FFortItemEntry::Size());

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

    if (NewPickup->HasbRandomRotation())
        NewPickup->bRandomRotation = RandomRotation;
    if (Entry.Level != -1)
        NewPickup->PrimaryPickupItemEntry.Level = Entry.Level;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    static auto HasPhantomReserveAmmo = Entry.HasPhantomReserveAmmo();
    if (HasPhantomReserveAmmo)
        NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
    NewPickup->OnRep_PrimaryPickupItemEntry();
    NewPickup->PawnWhoDroppedPickup = Pawn;

    NewPickup->TossPickup(Loc, Pawn, -1, Toss, true, (uint8) SourceTypeFlag, (uint8) SpawnSource);
    if (SpawnSource != -1)
        NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::GetChest() || SpawnSource == EFortPickupSpawnSource::GetAmmoBox();
    if (NewPickup->bTossedFromContainer)
        NewPickup->OnRep_TossedFromContainer();

    if (VersionInfo.FortniteVersion < 6)
    {
        static auto ProjectileMovementClass = FindClass("ProjectileMovementComponent");
        NewPickup->MovementComponent = UGameplayStatics::SpawnObject(ProjectileMovementClass, NewPickup);
    }

    return NewPickup;
}

AFortPickupAthena* AFortInventory::SpawnPickup(FVector Loc, const UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo, long long SourceTypeFlag, long long SpawnSource, AFortPlayerPawnAthena* Pawn, bool Toss, bool bRandomRotation)
{
    auto ItemEntry = MakeItemEntry(ItemDefinition, Count, -1);
    auto Pickup = SpawnPickup(Loc, *ItemEntry, SourceTypeFlag, SpawnSource, Pawn, -1, Toss, true, bRandomRotation);
    free(ItemEntry);
    return Pickup;
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

    if (NewPickup->HasbRandomRotation())
        NewPickup->bRandomRotation = true;
    NewPickup->PrimaryPickupItemEntry.Level = Entry.Level;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    static auto HasPhantomReserveAmmo = Entry.HasPhantomReserveAmmo();
    if (HasPhantomReserveAmmo)
        NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
    NewPickup->OnRep_PrimaryPickupItemEntry();
    NewPickup->PawnWhoDroppedPickup = Pawn;
    NewPickup->TossPickup(Loc, Pawn, -1, true, true, EFortPickupSourceTypeFlag::GetContainer(), EFortPickupSpawnSource::GetChest());
    //auto bFloorLoot = Container->IsA<ATiered_Athena_FloorLoot_01_C>() || Container->IsA<ATiered_Athena_FloorLoot_Warmup_C>();
    //UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, NewPickup, 1, 0, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, false);
    NewPickup->bTossedFromContainer = true;
    NewPickup->OnRep_TossedFromContainer();

    if (VersionInfo.FortniteVersion < 6)
    {
        static auto ProjectileMovementClass = FindClass("ProjectileMovementComponent");
        NewPickup->MovementComponent = UGameplayStatics::SpawnObject(ProjectileMovementClass, NewPickup);
    }

    return NewPickup;
}


bool AFortInventory::IsPrimaryQuickbar(const UFortItemDefinition* ItemDefinition)
{
    static auto MeleeClass = FindClass("FortWeaponMeleeItemDefinition");
    static auto ResourceClass = FindClass("FortResourceItemDefinition");
    static auto AmmoClass = FindClass("FortAmmoItemDefinition");
    static auto TrapClass = FindClass("FortTrapItemDefinition");
    static auto BuildingClass = FindClass("FortBuildingItemDefinition");
    static auto EditToolClass = FindClass("FortEditToolItemDefinition");

    return ItemDefinition->IsA(MeleeClass) || ItemDefinition->IsA(ResourceClass) || ItemDefinition->IsA(AmmoClass) || ItemDefinition->IsA(TrapClass) || ItemDefinition->IsA(BuildingClass) || ItemDefinition->IsA(EditToolClass) || (ItemDefinition->HasbForceIntoOverflow() && ItemDefinition->bForceIntoOverflow) ? false : true;
}


void AFortInventory::UpdateEntry(FFortItemEntry& Entry)
{
    if (!this)
        return; // wtf 3.5

    auto repEnt = Inventory.ReplicatedEntries.Search([&](FFortItemEntry& item)
        { return item.ItemGuid == Entry.ItemGuid; }, FFortItemEntry::Size());
    if (repEnt)
        *repEnt = Entry;
        //__movsb((PBYTE)repEnt, (const PBYTE)&Entry, FFortItemEntry::Size());

    auto ent = Inventory.ItemInstances.Search([&](UFortWorldItem* item)
        { return item->ItemEntry.ItemGuid == Entry.ItemGuid; });
    if (ent)
        (*ent)->ItemEntry = Entry;
        //__movsb((PBYTE)&(*ent)->ItemEntry, (const PBYTE)&Entry, FFortItemEntry::Size());

    Update(&Entry);
}

bool RemoveInventoryItem(IInterface* Interface, FGuid& ItemGuid, int Count, bool bForceRemoval)
{
    static auto InterfaceOffset = FindClass("FortPlayerController")->GetSuper()->GetPropertiesSize() + (VersionInfo.EngineVersion >= 4.27 ? 16 : 8);
    auto PlayerController = (AFortPlayerControllerAthena*)(__int64(Interface) - InterfaceOffset);

    auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
        { return entry.ItemGuid == ItemGuid; }, FFortItemEntry::Size());

    if (ItemEntry)
    {
        ItemEntry->Count -= Count;
        if (ItemEntry->Count <= 0 || bForceRemoval)
            PlayerController->WorldInventory->Remove(ItemGuid);
        else
            PlayerController->WorldInventory->UpdateEntry(*ItemEntry);

        return true;
    }

    return false;
}

void SetLoadedAmmo(UFortWorldItem* Item, int LoadedAmmo)
{
    Item->ItemEntry.LoadedAmmo = LoadedAmmo;

    auto PlayerController = (AFortPlayerControllerAthena*)Item->GetOwningController();
    PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
}

void SetPhantomReserveAmmo(UFortWorldItem* Item, unsigned int PhantomReserveAmmo)
{
    Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;

    auto PlayerController = (AFortPlayerControllerAthena*)Item->GetOwningController();
    PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
}

void AFortInventory::Hook()
{
    Utils::Hook(FindRemoveInventoryItem(), RemoveInventoryItem);

    auto SetOwningInventory = Memcury::Scanner::FindPattern("48 85 D2 74 ? 80 BA ? ? ? ? ? 75 ? 48 89 91").Get(); // finds on 1.8, 4.1, 10.40, 14.40, 19.10, 21.00

    if (SetOwningInventory)
    {
        auto WorldItemVft = UFortWorldItem::GetDefaultObj()->Vft;
        int SetOwningInventoryIdx = 0;

        for (int i = 0; i < 0x200; i++)
        {
            if (WorldItemVft[i] == (void*)SetOwningInventory)
            {
                SetOwningInventoryIdx = i;
                break;
            }
        }

        if (SetOwningInventoryIdx)
        {
            auto HasPhantomReserveAmmo = FFortItemEntry::HasPhantomReserveAmmo();

            Utils::Hook<UFortWorldItem>(uint32(SetOwningInventoryIdx - (HasPhantomReserveAmmo ? (VersionInfo.EngineVersion < 4.27 ? 2 : 3) : 1)), SetLoadedAmmo);
            if (HasPhantomReserveAmmo)
                Utils::Hook<UFortWorldItem>(uint32(SetOwningInventoryIdx - (VersionInfo.EngineVersion < 4.27 ? 1 : 2)), SetPhantomReserveAmmo);
        }
    }
}