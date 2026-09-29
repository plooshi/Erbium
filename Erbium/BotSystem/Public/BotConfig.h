#pragma once
#include "BotState.h"
#include "Building/BotBuildCatalog.h"

// Bot system configuration.
//
// FBotSystemConfig holds server-wide settings in the same style as Erbium's FConfiguration
// (Erbium/Public/Configuration.h): static inline values that can be edited before building or
// changed at runtime from the GUI.
//
// FBotConfig holds per-bot settings. Every bot receives its own copy when it is spawned, so later
// phases (difficulty, aim, perception...) can give different bots different values without touching
// the global settings.

enum class EBotControllerMode : uint8
{
    // Use an AI controller when the build has a usable class, otherwise fall back to a player controller.
    Auto,
    AIController,
    // Phase 1 behavior: bots exist and can be killed, but cannot move (see EBotControllerKind).
    PlayerController,
};

struct FBotSystemConfig
{
    // Master switch. When false, spawn requests are rejected.
    static inline bool bEnabled = true;

    static inline EBotControllerMode ControllerMode = EBotControllerMode::Auto;

    // Hard cap on simultaneously existing bots (alive + dead-awaiting-cleanup).
    static inline int MaxBots = 100;

    // How often (seconds) the manager validates bots: detects destroyed/dead pawns and cleans up.
    static inline float MaintenanceInterval = 0.25f;

    // How often (seconds) each bot's behavior is ticked. Bots are staggered across this interval so
    // that 100 bots do not all think on the same frame.
    static inline float ThinkInterval = 0.1f;

    // How often (seconds) the shared world snapshot used by perception is rebuilt. One actor query per
    // interval, shared by all bots.
    static inline float PerceptionSnapshotInterval = 0.5f;

    // Maximum number of bots spawned per server tick. Spawning is comparatively expensive (several
    // actors, inventory, abilities), so large requests are spread over multiple frames.
    static inline int MaxSpawnsPerTick = 4;

    // Seconds a dead bot is kept around before its controller/player state are destroyed. Gives the
    // game time to finish its own death handling (drops, kill feed, replication of DeathInfo).
    static inline float DeadBotCleanupDelay = 5.f;

    // When true, dead bots are removed automatically after DeadBotCleanupDelay.
    static inline bool bRemoveDeadBots = true;

    // Radius (cm) around the requested location in which bots are scattered, so a batch spawned at the
    // same point does not stack on top of each other.
    static inline float SpawnScatterRadius = 400.f;

    // Try nav-mesh pathfinding first. When the map has no usable nav data the navigation layer
    // automatically falls back to direct paths and then to steering (see BotNavigation.h).
    static inline bool bUseNavMesh = true;

    // Global switch for Phase 4 building on top of the per-bot FBotConfig::bCanBuild.
    static inline bool bAllowBuilding = true;

    // Log aggregated tick timings every PerfReportInterval seconds while bots exist (0 disables).
    static inline float PerfReportInterval = 30.f;

    static inline bool bVerboseLogging = false;
};

struct FBotConfig
{
    // Display name prefix. The bot id is appended: "Erbium Bot (#12)".
    const wchar_t* NamePrefix = L"Erbium Bot";

    float MaxHealth = 100.f;
    float StartingShield = 0.f;

    // Give the playlist's starting items (pickaxe, build tools, ...) the same way a real player gets them.
    bool bGiveStartingItems = true;

    // State entered after spawning (only when the bot can move; otherwise Idle).
    EBotAIState DefaultAIState = EBotAIState::Searching;

    // --- Perception ---
    float DetectionRadius = 5000.f;    // cm. Pawns further away are ignored entirely.
    float PerceptionInterval = 0.5f;   // Seconds between this bot's perception updates.
    int MaxLineOfSightChecks = 4;      // Nearest N candidates get a line-of-sight trace per update.

    // --- Movement ---
    float AcceptanceRadius = 100.f;    // cm. Close enough to a location goal.
    float FollowDistance = 400.f;      // cm. Following bots stop this far from their target.
    float SprintDistance = 1500.f;     // cm. Sprint when the goal is further than this.
    float SearchRadius = 3000.f;       // cm. Wander radius while Searching.
    float RepathInterval = 1.0f;       // Seconds between path refreshes for moving goals.
    float StuckCheckInterval = 1.5f;   // Seconds between progress checks.
    float StuckDistance = 50.f;        // cm. Less progress than this counts as stuck.
    int MaxStuckChecks = 3;            // Give up on a goal after this many stuck checks in a row.
    bool bCanSprint = true;
    bool bCanJump = true;              // Jump when stuck.

    // --- Building (Phase 4) ---
    bool bCanBuild = true;
    float BuildDecisionInterval = 0.5f; // Seconds between "should I build?" evaluations.
    float BuildReactionTime = 0.4f;     // Seconds from decision to placement (randomised +-50%).
    float BuildCooldown = 1.5f;         // Minimum seconds between placements (also back-off after a failure).
    int MaxBuildsPerMinute = 12;
    float DefensiveBuildChance = 0.6f;  // Chance to wall up when hit by a visible enemy (per decision).
    float DamageMemory = 1.5f;          // Seconds a hit counts as "being shot at".
    float DefensiveHoldTime = 1.5f;     // Seconds to stay behind a fresh wall before carrying on.
    bool bBuildRampsWhenStuck = true;
    int BuildCost = 10;                 // Materials per piece (the game's standard cost).

    // Stand-in materials for bots without an inventory (AI-controller bots until Phase 2B).
    int StartingMaterials[(int)EBotBuildMaterial::Count] = { 300, 150, 0 };

    // Material preference; the first affordable one with a buildable class is used.
    EBotBuildMaterial MaterialOrder[(int)EBotBuildMaterial::Count] = { EBotBuildMaterial::Wood, EBotBuildMaterial::Stone, EBotBuildMaterial::Metal };

    // Ramp facing cannot be measured automatically (see BotBuildGrid.h). If ramps come out rising
    // towards the bot instead of away from it on a build, set this to 180.
    float RampYawOffset = 0.f;
};
