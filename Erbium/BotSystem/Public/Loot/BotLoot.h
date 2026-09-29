#pragma once
#include "../../../pch.h"
#include "../World/BotWorld.h"

class FBotController;
class AFortPickupAthena;
class UFortItemDefinition;

// Phase 2B: loot on the ground (pickups: floor loot, drops, eliminated players' items).
//
// Like perception, one shared snapshot of all pickups is rebuilt at FBotSystemConfig::LootSnapshotInterval
// and every bot filters it by distance, so the actor query cost does not grow with the bot count.
// Pickups are *claimed* by the bot heading for them so a group of bots does not race for one item.
//
// Chests and ammo boxes (Phase 6) come from BotWorld's container snapshot. They are opened with
// UFortLootPackage::SpawnLootHook -- the function Erbium's ServerAttemptInteract calls when a player
// searches a container -- after the bot has stood at it for ContainerSearchTime.
namespace BotLootWorld
{
    struct FEntry
    {
        TWeakObjectPtr<AFortPickupAthena> Pickup;
        FVector Location{};
        const UFortItemDefinition* Definition = nullptr;
        int Count = 0;
    };

    void Update(double Now);
    std::vector<FEntry>& GetPickups();
    void Reset();

    // Returns false if another bot already claimed it (pickups and containers).
    bool Claim(AActor* Target, uint32 BotId);
    void ReleaseClaims(uint32 BotId);
}

namespace BotLoot
{
    // Most worthwhile pickup within the bot's LootSearchRadius (score falls off with distance).
    // Returns false when nothing is worth walking to.
    bool FindBestPickup(FBotController& Bot, BotLootWorld::FEntry& OutEntry, float& OutScore);

    // Most worthwhile unsearched chest/ammo box within LootSearchRadius (inside the next storm circle).
    bool FindBestContainer(FBotController& Bot, BotWorld::FContainer& OutContainer, float& OutScore);

    // Opens the container if the bot is next to it. Loot drops as pickups the bot then collects.
    bool IsInReach(FBotController& Bot, AActor* Target);
    bool OpenContainer(FBotController& Bot, ABuildingContainer* Container);

    // Takes the pickup into the bot's inventory if it is in reach and still available, exactly like
    // the game finishes a player's pickup (mark picked up, notify, destroy the pickup actor).
    bool TryPickup(FBotController& Bot, AFortPickupAthena* Pickup);

    // Equip the best weapon for general use after the inventory changed (out of combat).
    void EquipBestLoadout(FBotController& Bot);
}
