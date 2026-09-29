#pragma once
#include "../../../pch.h"

class FBotController;
class AFortPlayerPawnAthena;

// What one bot knows about one other pawn.
struct FBotPerceivedPawn
{
    TWeakObjectPtr<AFortPlayerPawnAthena> Pawn;
    FVector Location{};
    float Distance = 0.f;
    uint8 TeamIndex = 0;
    bool bIsBot = false;
    bool bIsEnemy = false;

    // Line of sight confirmed on the latest update. Only the nearest MaxLineOfSightChecks candidates
    // are traced; the rest keep bVisible = false until they are close enough to be traced.
    bool bVisible = false;

    // World time this pawn was last visible (0 = never). Lets behaviors remember a target briefly after
    // it breaks line of sight instead of forgetting it instantly.
    double LastSeenTime = 0.0;
};

// Shared snapshot of all living player pawns, rebuilt at most every
// FBotSystemConfig::PerceptionSnapshotInterval. One GetAllActorsOfClass per interval serves every bot,
// instead of one per bot per update.
namespace BotPerceptionWorld
{
    struct FEntry
    {
        TWeakObjectPtr<AFortPlayerPawnAthena> Pawn;
        FVector Location{};
        uint8 TeamIndex = 0;
        bool bIsBot = false;
    };

    void Update(double Now);
    const std::vector<FEntry>& GetPawns();
    void Reset();
}

// Per-bot perception. Only information a player could have: pawns within DetectionRadius, with
// visibility from real line-of-sight traces from the bot's own controller. Teams come from the
// player states (players know who their teammates are).
class FBotPerception
{
public:
    explicit FBotPerception(FBotController& InBot);

    // Runs at the bot's PerceptionInterval; cheap no-op otherwise.
    void Update(double Now);

    // Sorted by distance, nearest first.
    const std::vector<FBotPerceivedPawn>& GetPawns() const { return Pawns; }

    const FBotPerceivedPawn* GetNearestVisibleEnemy() const;

    // Nearest pawn controlled by a human (not a bot), optionally only if visible.
    const FBotPerceivedPawn* GetNearestHuman(bool bVisibleOnly) const;

private:
    FBotController& Bot;
    std::vector<FBotPerceivedPawn> Pawns;
    double NextUpdateTime = 0.0;
};
