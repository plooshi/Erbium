#include "pch.h"
#include "../../Public/Loot/BotLoot.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortInventory.h"
#include "../../../FortniteGame/Public/FortLootPackage.h"
#include "../../../FortniteGame/Public/BuildingContainer.h"

// A pickup this close (2D, cm) is in reach; the game's own pickup radius is similar.
static constexpr double PickupReach = 250.0;

namespace BotLootWorld
{
    static std::vector<FEntry> Entries;
    static double NextSnapshotTime = 0.0;
    static std::unordered_map<int32, uint32> Claims; // pickup object index -> bot id

    void Update(double Now)
    {
        if (Now < NextSnapshotTime)
            return;

        NextSnapshotTime = Now + FBotSystemConfig::LootSnapshotInterval;
        Entries.clear();

        TArray<AFortPickupAthena*> Pickups;
        Utils::GetAll<AFortPickupAthena>(Pickups);

        for (auto& Pickup : Pickups)
        {
            if (!Pickup || Pickup->bActorIsBeingDestroyed || Pickup->bPickedUp)
                continue;

            auto Definition = Pickup->PrimaryPickupItemEntry.ItemDefinition;
            if (!Definition)
                continue;

            FEntry Entry;
            Entry.Pickup = TWeakObjectPtr<AFortPickupAthena>(Pickup);
            Entry.Location = Pickup->K2_GetActorLocation();
            Entry.Definition = Definition;
            Entry.Count = Pickup->PrimaryPickupItemEntry.Count;
            Entries.push_back(Entry);
        }

        Pickups.Free();

        // Forget claims on pickups that no longer exist.
        for (auto It = Claims.begin(); It != Claims.end();)
        {
            auto Item = TUObjectArray::GetObjectByIndex(It->first);
            It = Item ? std::next(It) : Claims.erase(It);
        }
    }

    std::vector<FEntry>& GetPickups()
    {
        return Entries;
    }

    void Reset()
    {
        Entries.clear();
        Claims.clear();
        NextSnapshotTime = 0.0;
    }

    bool Claim(AActor* Target, uint32 BotId)
    {
        auto [It, bInserted] = Claims.try_emplace(Target->Index, BotId);
        return bInserted || It->second == BotId;
    }

    void ReleaseClaims(uint32 BotId)
    {
        for (auto It = Claims.begin(); It != Claims.end();)
            It = It->second == BotId ? Claims.erase(It) : std::next(It);
    }
}

namespace BotLoot
{
    bool FindBestPickup(FBotController& Bot, BotLootWorld::FEntry& OutEntry, float& OutScore)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn || !Bot.GetInventory().IsInitialized())
            return false;

        auto& Config = Bot.GetConfig();
        auto Location = Pawn->K2_GetActorLocation();
        double RadiusSquared = (double)Config.LootSearchRadius * Config.LootSearchRadius;

        OutScore = 0.f;
        bool bFound = false;

        for (auto& Entry : BotLootWorld::GetPickups())
        {
            auto DX = Entry.Location.X - Location.X;
            auto DY = Entry.Location.Y - Location.Y;
            auto DZ = Entry.Location.Z - Location.Z;
            auto DistanceSquared = DX * DX + DY * DY + DZ * DZ;
            if (DistanceSquared > RadiusSquared || Bot.IsLootIgnored(Entry.Pickup.ObjectIndex))
                continue;

            auto Score = Bot.GetInventory().ScoreItem(Entry.Definition, Entry.Count);
            if (Score <= 0.f)
                continue;

            // Nearby items are worth more than slightly better ones far away.
            Score /= 1.f + (float)sqrt(DistanceSquared) / 1500.f;

            if (Score > OutScore)
            {
                OutScore = Score;
                OutEntry = Entry;
                bFound = true;
            }
        }

        return bFound && OutScore >= Config.MinLootScore;
    }

    bool FindBestContainer(FBotController& Bot, BotWorld::FContainer& OutContainer, float& OutScore)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn)
            return false;

        auto& Config = Bot.GetConfig();
        auto Location = Pawn->K2_GetActorLocation();
        double RadiusSquared = (double)Config.LootSearchRadius * Config.LootSearchRadius;

        OutScore = 0.f;
        bool bFound = false;

        for (auto& Entry : BotWorld::GetContainers())
        {
            auto DX = Entry.Location.X - Location.X, DY = Entry.Location.Y - Location.Y, DZ = Entry.Location.Z - Location.Z;
            auto DistanceSquared = DX * DX + DY * DY + DZ * DZ;
            if (DistanceSquared > RadiusSquared || !BotWorld::IsInsideNextZone(Entry.Location))
                continue;

            auto Container = Entry.Container.Get();
            if (!Container || Container->bAlreadySearched || Bot.IsLootIgnored(Entry.Container.ObjectIndex))
                continue;

            auto Score = (Entry.bIsChest ? Config.ChestValue : Config.AmmoBoxValue) / (1.f + (float)sqrt(DistanceSquared) / 1500.f);
            if (Score > OutScore)
            {
                OutScore = Score;
                OutContainer = Entry;
                bFound = true;
            }
        }

        return bFound && OutScore >= Config.MinLootScore;
    }

    bool IsInReach(FBotController& Bot, AActor* Target)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn || !Target)
            return false;

        auto TargetLocation = Target->K2_GetActorLocation();
        auto Location = Pawn->K2_GetActorLocation();
        auto DX = TargetLocation.X - Location.X, DY = TargetLocation.Y - Location.Y;
        return sqrt(DX * DX + DY * DY) <= PickupReach && fabs(TargetLocation.Z - Location.Z) <= 250.0;
    }

    bool OpenContainer(FBotController& Bot, ABuildingContainer* Container)
    {
        if (!Container || Container->bActorIsBeingDestroyed || Container->bAlreadySearched || !IsInReach(Bot, Container))
            return false;

        BOT_VERBOSE(BOT_CATEGORY_LOOT, "Bot #%u opens a container.", Bot.GetId());
        return UFortLootPackage::SpawnLootHook(Container);
    }

    bool TryPickup(FBotController& Bot, AFortPickupAthena* Pickup)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn || !Pickup || Pickup->bActorIsBeingDestroyed || Pickup->bPickedUp)
            return false;

        auto PickupLocation = Pickup->K2_GetActorLocation();
        auto Location = Pawn->K2_GetActorLocation();
        auto DX = PickupLocation.X - Location.X, DY = PickupLocation.Y - Location.Y;
        if (sqrt(DX * DX + DY * DY) > PickupReach || fabs(PickupLocation.Z - Location.Z) > 250.0)
            return false;

        auto& Entry = Pickup->PrimaryPickupItemEntry;
        auto Definition = Entry.ItemDefinition;
        if (!Definition || !Bot.GetInventory().Give(Definition, Entry.Count, Entry.LoadedAmmo))
            return false;

        Pickup->bPickedUp = true;
        Pickup->OnRep_bPickedUp();
        Pickup->K2_DestroyActor();

        BOT_VERBOSE(BOT_CATEGORY_LOOT, "Bot #%u picked up %d x %s", Bot.GetId(), Entry.Count, BotItems::GetName(Definition).c_str());
        return true;
    }

    void EquipBestLoadout(FBotController& Bot)
    {
        auto& Inventory = Bot.GetInventory();
        auto Items = Inventory.GetItems();

        // Medium range as the default "walking around" weapon; pickaxe when there is no gun.
        auto Best = Inventory.GetBestWeaponFor(2500.f, Items);
        if (!Best)
            Best = Inventory.GetBestOf(EBotItemCategory::Melee, Items);

        if (Best && !SameGuid(Best->Guid, Inventory.GetEquippedGuid()))
            Inventory.Equip(*Best);
    }
}
