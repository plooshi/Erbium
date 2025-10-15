#include "pch.h"
#include "../Public/FortInventory.h"
#include "../Public/FortPlayerPawnAthena.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortKismetLibrary.h"
#include "../../Erbium/Public/Configuration.h"
#include <objbase.h>

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

UFortWorldItem* AFortInventory::GiveItem(const UFortItemDefinition* Def, int Count, int LoadedAmmo, int Level, bool ShowPickupNoti, bool updateInventory, int PhantomReserveAmmo, TArray<FFortItemEntryStateValue> StateValues)
{
    if (!this || !Def || !Count)
        return nullptr;
    UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, Level);
    Item->SetOwningControllerForTemporaryItem(Owner);
    Item->ItemEntry.LoadedAmmo = LoadedAmmo;
    if (Item->ItemEntry.HasPhantomReserveAmmo())
        Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;
    if (Item->ItemEntry.HasStateValues()) // dk
        Item->ItemEntry.StateValues = StateValues;

    /*if (Item->ItemEntry.ItemGuid.A == 0 && Item->ItemEntry.ItemGuid.B == 0 && Item->ItemEntry.ItemGuid.C == 0 && Item->ItemEntry.ItemGuid.D == 0)
    {
        CoCreateGuid((GUID*)&Item->ItemEntry.ItemGuid);

        if (FFortItemEntry::HasTrackerGuid() && Item->ItemEntry.TrackerGuid.A == 0 && Item->ItemEntry.TrackerGuid.B == 0 && Item->ItemEntry.TrackerGuid.C == 0 && Item->ItemEntry.TrackerGuid.D == 0)
            CoCreateGuid((GUID*)&Item->ItemEntry.TrackerGuid);
    }*/


    auto& repEntry = this->Inventory.ReplicatedEntries.Add(Item->ItemEntry, FFortItemEntry::Size());
    repEntry.bIsReplicatedCopy = true;
    this->Inventory.ItemInstances.Add(Item);

    static auto OnItemInstanceAddedVft = FindOnItemInstanceAddedVft();
    if (OnItemInstanceAddedVft && VersionInfo.FortniteVersion >= 4)
    {
        static auto InterfaceClass = FindClass("FortInventoryOwnerInterface");
        ((bool(*)(const UFortItemDefinition*, const IInterface*, UFortWorldItem*, uint8)) Def->Vft[OnItemInstanceAddedVft])(Def, Owner->GetInterface(InterfaceClass), Item, 1);
    }

    /*if (Item->ItemEntry.ItemDefinition->bForceFocusWhenAdded)
    {
        ((AFortPlayerControllerAthena*)Owner)->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
        ((AFortPlayerControllerAthena*)Owner)->ClientEquipItem(Item->ItemEntry.ItemGuid, true);
    }*/

    if (VersionInfo.EngineVersion < 4.20)
        ((AFortPlayerControllerAthena*)Owner)->QuickBars->ServerAddItemInternal(Item->ItemEntry.ItemGuid, !IsPrimaryQuickbar(Def), -1);

    if (updateInventory)
    {
        Update(&repEntry);

        HandleInventoryLocalUpdate(); // calls UpdateItemInstances, the func we actually want
    }
    return Item;
}

UFortWorldItem* AFortInventory::GiveItem(FFortItemEntry& entry, int Count, bool ShowPickupNoti, bool updateInventory)
{
    if (Count == -1)
        Count = entry.Count;

    return GiveItem(entry.ItemDefinition, Count, entry.LoadedAmmo, entry.Level, ShowPickupNoti, updateInventory, entry.HasPhantomReserveAmmo() ? entry.PhantomReserveAmmo : 0, entry.HasStateValues() ? entry.StateValues : TArray<FFortItemEntryStateValue>{});
}

void AFortInventory::SetRequiresUpdate()
{
    Inventory.MarkArrayDirty();
    bRequiresLocalUpdate = true;
    bRequiresSaving = true;

    ForceNetUpdate();
}

void AFortInventory::Update(FFortItemEntry* Entry)
{
    if (!Entry)
        return SetRequiresUpdate();

    if (Entry->bIsReplicatedCopy)
    {
        Inventory.MarkItemDirty(*Entry);
        SetRequiresUpdate();
        Entry->bIsDirty = true;
        return;
    }

    for (int i = 0; i < Inventory.ReplicatedEntries.Num(); i++)
    {
        auto& repEntry = Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

        if (repEntry.ItemGuid == Entry->ItemGuid)
        {
            repEntry = *Entry;
            repEntry.bIsDirty = false;
            Inventory.MarkItemDirty(repEntry);
            SetRequiresUpdate();
        }
    }
    Entry->bIsDirty = true;
    /*bRequiresLocalUpdate = true;
    HandleInventoryLocalUpdate();

    return Entry ? Inventory.MarkItemDirty(*Entry) : Inventory.MarkArrayDirty();*/
}


void AFortInventory::Remove(FGuid Guid)
{
    auto ItemEntryIdx = Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& entry) { return entry.ItemGuid == Guid; }, FFortItemEntry::Size());
    auto& ItemEntry = Inventory.ReplicatedEntries.Get(ItemEntryIdx, FFortItemEntry::Size());
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
    
    if (VersionInfo.EngineVersion < 4.20)
    {
        auto PlayerController = (AFortPlayerControllerAthena*)Owner;
        auto& QuickBar = IsPrimaryQuickbar(ItemEntry.ItemDefinition) ? PlayerController->QuickBars->PrimaryQuickBar : PlayerController->QuickBars->SecondaryQuickBar;
        int i = 0;
        for (i = 0; i < QuickBar.Slots.Num(); i++)
        {
            auto& Slot = QuickBar.Slots.Get(i, FQuickBarSlot::Size());

            for (auto& Item : Slot.Items)
                if (Item == Guid)
                    goto _Out;
        }
        goto _Skip;
    _Out:
        PlayerController->QuickBars->EmptySlot(!IsPrimaryQuickbar(ItemEntry.ItemDefinition), i);
        PlayerController->QuickBars->ServerRemoveItemInternal(Guid, false, true);
    }

_Skip:
    Update(nullptr);
    HandleInventoryLocalUpdate();
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
        if (Stats)
        {
            ItemEntry->LoadedAmmo = Stats->ClipSize;
            if (Weapon->HasbUsesPhantomReserveAmmo() && Weapon->bUsesPhantomReserveAmmo)
                ItemEntry->PhantomReserveAmmo = Stats->InitialClips * Stats->ClipSize;
        }
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

    static auto SetPickupItems = FindSetPickupItems();
    if (SetPickupItems)
    {
        TArray<FFortItemEntry> a{};
        if (VersionInfo.FortniteVersion >= 16)
            ((void(*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, uint8_t, bool, uint8_t)) SetPickupItems)(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, (uint8_t)EFortPickupSourceTypeFlag::GetContainer(), false, (uint8_t)EFortPickupSpawnSource::GetChest());
        else
            ((void(*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, bool)) SetPickupItems)(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, false);
    }
    else
        NewPickup->OnRep_PrimaryPickupItemEntry();
    //NewPickup->OnRep_PrimaryPickupItemEntry();
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
    auto& SpawnLocation = Container->HasLootSpawnLocation_Athena() ? Container->LootSpawnLocation_Athena : Container->LootSpawnLocation;
    auto Loc = ContainerLoc + (Container->GetActorForwardVector() * SpawnLocation.X) + (Container->GetActorRightVector() * SpawnLocation.Y) + (Container->GetActorUpVector() * SpawnLocation.Z);
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
    static auto SetPickupItems = FindSetPickupItems();
    if (SetPickupItems)
    {
        TArray<FFortItemEntry> a{};
        if (VersionInfo.FortniteVersion >= 16)
            ((void(*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, uint8_t, bool, uint8_t)) SetPickupItems)(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, (uint8_t)EFortPickupSourceTypeFlag::GetContainer(), false, (uint8_t)EFortPickupSpawnSource::GetChest());
        else
            ((void(*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, bool)) SetPickupItems)(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, false);
    }
    else
        NewPickup->OnRep_PrimaryPickupItemEntry();
    //NewPickup->OnRep_PrimaryPickupItemEntry();

    NewPickup->PawnWhoDroppedPickup = Pawn;
    //auto bFloorLoot = Container->IsA<ATiered_Athena_FloorLoot_01_C>() || Container->IsA<ATiered_Athena_FloorLoot_Warmup_C>();
    //UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, NewPickup, 1, 0, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, false);
    static auto tpfcPtr = UFortKismetLibrary::GetDefaultObj()->GetFunction("TossPickupFromContainer");
    if (tpfcPtr)
    {
        if (!UFortKismetLibrary::TossPickupFromContainer__Ptr)
            UFortKismetLibrary::TossPickupFromContainer__Ptr = tpfcPtr;

        UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, NewPickup, 10, (int32)std::clamp((float)rand() * 0.0003357036f, 0.f, 10.f), Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, Container->bForceHidePickupMinimapIndicator);
    }
    else
        NewPickup->TossPickup(Loc, Pawn, -1, true, true, EFortPickupSourceTypeFlag::GetContainer(), EFortPickupSpawnSource::GetChest());

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


    /*auto ent = Inventory.ReplicatedEntries.Search([&](FFortItemEntry& item)
        { return item.ItemGuid == Entry.ItemGuid; }, FFortItemEntry::Size());
    if (ent)
        *ent = Entry;*/

    auto ent2 = Inventory.ItemInstances.Search([&](UFortWorldItem* item)
        { return item->ItemEntry.ItemGuid == Entry.ItemGuid; });
    if (ent2)
    {
        (*ent2)->ItemEntry = Entry;
        Update(&(*ent2)->ItemEntry);
    }
        //__movsb((PBYTE)&(*ent)->ItemEntry, (const PBYTE)&Entry, FFortItemEntry::Size());

}

bool RemoveInventoryItem(IInterface* Interface, FGuid& ItemGuid, int Count, bool bForceRemoval)
{
    if (FConfiguration::bInfiniteAmmo)
        return true;

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
    //PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);

    PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
}

void SetPhantomReserveAmmo(UFortWorldItem* Item, unsigned int PhantomReserveAmmo)
{
    Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;

    auto PlayerController = (AFortPlayerControllerAthena*)Item->GetOwningController();
    //PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);

    PlayerController->WorldInventory->UpdateEntry(Item->ItemEntry);
}


void SpawnPickup_(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
    UFortItemDefinition* ItemDefinition;
    int32 NumberToSpawn;
    AFortPlayerPawnAthena* TriggeringPawn;
    FVector Position;
    FVector Direction;
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&NumberToSpawn);
    Stack.StepCompiledIn(&TriggeringPawn);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.IncrementCode();

    *Ret = AFortInventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, ItemDefinition->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)ItemDefinition)->ClipSize : 0, EFortPickupSourceTypeFlag::GetOther(), EFortPickupSpawnSource::GetSupplyDrop());
}

void AFortInventory::Hook()
{
    Utils::Hook(FindRemoveInventoryItem(), RemoveInventoryItem);

    auto SetOwningInventory = Memcury::Scanner::FindPattern("48 85 D2 74 ? 80 BA ? ? ? ? ? 75 ? 48 89 91").Get();
    if (!SetOwningInventory)
        SetOwningInventory = Memcury::Scanner::FindPattern("48 83 EC ? 48 85 D2 74 ? 80 BA ? ? ? ? ? 75 ? 48 81 C1").Get();

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
    
    Utils::ExecHook(DefaultObjImpl("FortAthenaSupplyDrop")->GetFunction("SpawnPickup"), SpawnPickup_);
}
