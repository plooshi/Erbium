#pragma once

// Bot system configuration.
//
// FBotSystemConfig holds server-wide settings in the same style as Erbium's FConfiguration
// (Erbium/Public/Configuration.h): static inline values that can be edited before building or
// changed at runtime from the GUI.
//
// FBotConfig holds per-bot settings. Every bot receives its own copy when it is spawned, so later
// phases (difficulty, aim, perception...) can give different bots different values without touching
// the global settings.

struct FBotSystemConfig
{
    // Master switch. When false, spawn requests are rejected.
    static inline bool bEnabled = true;

    // Hard cap on simultaneously existing bots (alive + dead-awaiting-cleanup).
    static inline int MaxBots = 100;

    // How often (seconds) the manager validates bots: detects destroyed/dead pawns and cleans up.
    static inline float MaintenanceInterval = 0.25f;

    // How often (seconds) each bot's behavior is ticked. Bots are staggered across this interval so
    // that 100 bots do not all think on the same frame.
    static inline float ThinkInterval = 0.1f;

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
};
