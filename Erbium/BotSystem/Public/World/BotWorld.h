#pragma once
#include "../../../pch.h"

class ABuildingContainer;
class AFortPlayerPawnAthena;

// Phase 6: match-level information bots react to. Everything here is information a player has too:
// the match phase, where the battle bus is, the storm circles shown on the map, and chests/ammo boxes
// in the world.
//
// Version differences (from Erbium's own code paths):
//  - Before 25.20 the game phase, aircraft and safe zone indicator live on the game state / game mode.
//  - From 25.20 they live on UFortGameStateComponent_BattleRoyaleGamePhaseLogic, and Erbium updates
//    storm state itself, only for AlivePlayers (UpdatePawnStormState does the same for bot pawns).

enum class EBotMatchPhase : uint8
{
    Unknown,
    Warmup,
    Aircraft,
    SafeZones,
    EndGame,
};

struct FBotSafeZone
{
    bool bActive = false;     // a storm circle exists
    FVector CurrentCenter{};  // circle right now (interpolated while shrinking)
    float CurrentRadius = 0.f;
    FVector NextCenter{};     // circle it is shrinking / will shrink to
    float NextRadius = 0.f;
    bool bShrinking = false;
    float SecondsUntilShrinkStart = 0.f;
};

namespace BotWorld
{
    // Refreshes the cached phase / zone / aircraft (called once per manager tick) and the container
    // snapshot (every FBotSystemConfig::ContainerSnapshotInterval).
    void Update(double Now);
    void Reset();

    EBotMatchPhase GetPhase();
    const char* ToString(EBotMatchPhase Phase);

    const FBotSafeZone& GetSafeZone();
    bool IsInsideCurrentZone(const FVector& Location, float MarginFraction = 0.f);
    bool IsInsideNextZone(const FVector& Location, float MarginFraction = 0.f);

    // A point well inside the next circle, on the side facing From (reachable if a nav mesh exists).
    FVector GetRotationPoint(const FVector& From);

    // The battle bus while it flies, and its drop window in world time.
    AActor* GetAircraft();

    // Bus state that does not depend on the game-phase value alone (its numbering differs between
    // builds). Bus phase: the match started (Erbium's StartAircraftPhase) and the bus has not left.
    // Bus gone: storm phase, or the bus actor disappeared (with a grace period for the first seconds
    // after the start, before the bus actor exists).
    bool IsBusPhase();
    bool IsBusGone();
    bool GetDropWindow(double& OutStart, double& OutEnd);

    struct FContainer
    {
        TWeakObjectPtr<ABuildingContainer> Container;
        FVector Location{};
        bool bIsChest = false; // chest (treasure) vs ammo box
    };

    // Unsearched chests and ammo boxes.
    std::vector<FContainer>& GetContainers();

    // 25.20+: sets bIsInAnyStorm / bIsInsideSafeZone for a bot pawn exactly like Erbium's phase logic
    // does for players (the storm damage effect reads them). No-op on older builds (native handling).
    void UpdatePawnStormState(AFortPlayerPawnAthena* Pawn);
}
