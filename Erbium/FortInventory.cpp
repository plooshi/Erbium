#include "pch.h"
#include "FortInventory.h"

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


void AFortInventory::Update(FFortItemEntry* Entry)
{
    bRequiresLocalUpdate = true;
    HandleInventoryLocalUpdate();

    return Entry ? Inventory.MarkItemDirty(*Entry) : Inventory.MarkArrayDirty();
}