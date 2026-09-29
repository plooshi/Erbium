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
    // Master switch (GUI "AI" tab). When false, spawn requests are rejected and existing bots are removed.
    static inline bool bEnabled = true;

    // Bots stay in the match but stop thinking and moving (debugging; the GUI "AI" tab).
    static inline bool bPaused = false;

    // Fill the match with AutoSpawnCount bots once the first player has connected and AutoSpawnDelay
    // seconds have passed, so bots are there when you load in without using the GUI or cheats.
    static inline bool bAutoSpawn = true;
    static inline int AutoSpawnCount = 20;
    static inline float AutoSpawnDelay = 5.f;

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

    // How often (seconds) the shared pickup snapshot used for looting is rebuilt.
    static inline float LootSnapshotInterval = 1.0f;

    // How often (seconds) the chest / ammo box snapshot is rebuilt (containers are many and static).
    static inline float ContainerSnapshotInterval = 5.0f;

    // Phase 6: AI-controller bots board the battle bus when it starts and jump during its drop window.
    static inline bool bBotsUseBattleBus = true;

    // Difficulty applied to newly spawned bots (Phase 5.2).
    static inline EBotDifficulty DefaultDifficulty = EBotDifficulty::Normal;

    // Virtual-inventory bots (engine AIController) cannot use consumables through the game's item
    // abilities, which need a real inventory item. When true they apply the item's effect after its
    // use time instead (see FBotConsumableEffects); when false they simply carry heals unused.
    static inline bool bEmulateConsumablesForVirtualInventory = true;

    // Prefer Fortnite's own FortAthenaAIBotController over the engine AIController in Auto mode. It has a
    // native inventory (real item pickups, game-driven reload/consumables) but also Fortnite bot logic of
    // its own, which the bot system stops at spawn. Try this if weapons do not fire with the default.
    static inline bool bPreferFortniteBotController = false;

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

    EBotDifficulty Difficulty = EBotDifficulty::Normal;

    // --- Decisions (Phase 5.1) ---
    float DecisionInterval = 0.4f;
    // Weight per EBotPriority: Search, Loot, Combat, Healing, Survival, Storm.
    float PriorityWeights[(int)EBotPriority::Count] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };
    float Aggression = 1.0f;            // Scales the Combat score.

    // --- Loot / inventory (Phase 2B) ---
    bool bCanLoot = true;
    float LootSearchRadius = 3000.f;    // cm
    float MinLootScore = 5.f;           // ignore pickups scoring below this (after distance falloff)

    // --- Combat (Phase 2C) ---
    float AimReactionTime = 0.5f;       // s before a new target is tracked at all
    float AimAccuracy = 0.6f;           // 0..1, 1 = no random error
    float AimTurnSpeed = 180.f;         // deg/s the aim point can move
    float TargetSwitchDelay = 1.0f;     // s before switching to a different target
    float MaxAimDistance = 6000.f;      // cm
    float TargetMemory = 3.0f;          // s a target out of sight is still pursued
    float AimHeightOffset = 35.f;       // cm above the target's origin (upper body)
    float FireToleranceDegrees = 6.f;   // fire only when the aim is this close to the target
    float RepositionInterval = 3.5f;    // s between combat strafes

    // --- Healing / survival (Phase 2C) ---
    bool bCanHeal = true;
    float HealHealthThreshold = 70.f;   // heal below this health (if carrying healing)
    float HealShieldThreshold = 50.f;   // shield up below this shield (if carrying shields)
    float SafeHealDistance = 2500.f;    // no healing with a visible enemy closer than this (cm)
    float RetreatHealthFraction = 0.3f; // (health+shield)/200 below which the bot retreats when threatened
    float EscapeDistance = 2000.f;      // cm

    // --- Match flow (Phase 6) ---
    float ChestValue = 120.f;           // loot score of an unsearched chest (a gun when unarmed ~200)
    float AmmoBoxValue = 45.f;
    float ContainerSearchTime = 1.0f;   // s standing at a container before it opens (like holding interact)
    float StormMargin = 0.1f;           // aim to be this fraction inside the next circle's edge

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
