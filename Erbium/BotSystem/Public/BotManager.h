#pragma once
#include "../../pch.h"
#include "BotConfig.h"
#include "BotController.h"
#include <atomic>
#include <deque>

// Central owner of all bots.
//
// Threading: everything that touches engine objects runs on the game thread, driven by Tick()
// (called from UNetDriver::TickFlush). The GUI runs on its own thread, so it must use the Request*
// functions, which only update atomics that Tick() drains.
//
// Ownership: the manager owns every FBotController (unique_ptr). Bot ids are never reused, so a stale
// id held by another system simply fails to resolve instead of pointing at a different bot.
class FBotManager
{
public:
    // --- Game-thread API ---------------------------------------------------------------------------

    // Spawn one bot immediately at a player start / at a location. Returns nullptr on failure (limit
    // reached, bot system disabled, match not ready, spawn failed).
    static FBotController* SpawnBot();
    static FBotController* SpawnBot(const FVector& Location, const FRotator& Rotation);

    // Queue Count bots; they are spawned over the next frames at MaxSpawnsPerTick. When Near is set,
    // bots are scattered around that location instead of player starts. Returns how many were queued.
    static int SpawnBots(int Count, const FVector* Near = nullptr);

    static bool RemoveBot(uint32 BotId);
    static bool RemoveBot(FBotController* Bot);
    static void RemoveAllBots();

    static std::vector<FBotController*> GetActiveBots();
    static int GetBotCount();
    static int GetPendingSpawnCount();

    // Commands for every living bot (0 = all bots) or one bot id. Return how many bots were commanded.
    static int CommandMoveTo(uint32 BotId, const FVector& Location);
    static int CommandFollow(uint32 BotId, AActor* Target);
    static int CommandState(uint32 BotId, EBotAIState State);
    // Build a piece facing Towards (e.g. the commanding player).
    static int CommandBuild(uint32 BotId, EBotBuildPiece Piece, AActor* Towards);
    // Dress one bot (or all) in a specific outfit id; returns how many changed.
    static int CommandOutfit(uint32 BotId, const std::wstring& OutfitId);
    static int CommandDifficulty(uint32 BotId, EBotDifficulty Difficulty);

    // Multi-line debug description of one bot (Phase 5.5): id, state, decision, target, weapon, health,
    // shield, destination, inventory, cosmetics, builds.
    static std::string DescribeBot(uint32 BotId);

    // Log DescribeBot for this bot every few seconds (0 = off).
    static inline uint32 WatchedBotId = 0;

    static FBotController* FindBot(uint32 BotId);
    static FBotController* FindBotByController(const AActor* Controller);
    static bool IsBot(const AActor* Controller) { return FindBotByController(Controller) != nullptr; }

    // --- Any-thread API (GUI) ----------------------------------------------------------------------

    static void RequestSpawnBots(int Count);
    static void RequestRemoveAllBots();

    // Bot count as of the last game-thread tick (safe to read from the GUI thread).
    static int GetPublishedBotCount() { return PublishedBotCount; }

    // --- Engine callbacks ----------------------------------------------------------------------------

    // Called once per server frame from every UNetDriver::TickFlush variant. Ignores net drivers other
    // than the world's main one.
    static void Tick(const UObject* NetDriver, float DeltaSeconds);

    // Called from AFortPlayerControllerAthena::ClientOnPawnDied for every controller; no-op for humans.
    static void NotifyPawnDied(const AActor* Controller);

    // Called from Erbium's RemoveInventoryItem hook. Returns true (and the result in bOutRemoved) if the
    // inventory-owner interface belongs to an AI-controller bot; false = not ours, use the player path.
    static bool HandleRemoveInventoryItem(const IInterface* Interface, const FGuid& ItemGuid, int Count, bool bForceRemoval, bool& bOutRemoved);

    // Latest perf numbers (microseconds), for the GUI.
    static float GetLastAverageTickMicros() { return LastAverageTickMicros; }

private:
    struct FSpawnRequest
    {
        bool bUseLocation = false;
        FVector Location{};
    };

    static bool CanSpawnMore(int Additional);
    static FBotController* SpawnBotInternal(const FSpawnRequest& Request);
    static void ProcessQueuedRequests();
    static void RunMaintenance(double Now);
    static void RunThink(double Now);
    static void RunFrame(double Now, float DeltaSeconds);
    template <typename Fn>
    static int ForEachCommandTarget(uint32 BotId, Fn&& Func);
    static void FlushPendingRemovals();
    static void RecordTickTime(double Now, double Micros);
    static double GetWorldTime();

    static inline std::vector<std::unique_ptr<FBotController>> Bots;
    static inline uint32 NextBotId = 1;
    static inline bool bIsTicking = false;

    // Game-thread queue for SpawnBots().
    static inline std::deque<FSpawnRequest> SpawnQueue;

    // Cross-thread requests from the GUI.
    static inline std::atomic<int> RequestedSpawnCount = 0;
    static inline std::atomic<bool> bRemoveAllRequested = false;
    static inline std::atomic<int> PublishedBotCount = 0;

    static inline double NextMaintenanceTime = 0.0;
    static inline double NextWatchTime = 0.0;

    // Perf accounting (see RecordTickTime).
    static inline double PerfWindowStart = 0.0;
    static inline double PerfTotalMicros = 0.0;
    static inline double PerfMaxMicros = 0.0;
    static inline int PerfTickCount = 0;
    static inline int PerfSpawnCount = 0;
    static inline double PerfSpawnMicros = 0.0;
    static inline float LastAverageTickMicros = 0.f;
};
