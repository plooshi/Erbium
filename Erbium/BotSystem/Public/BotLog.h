#pragma once
#include "../../pch.h"
#include "BotConfig.h"

// Structured logging for the bot system.
//
// Every line is prefixed with a category tag ([BotSystem], [BotSpawn], [BotAI], ...) so logs can be
// filtered. Erbium logs through printf (stdout is redirected to the console or stdout.log by
// dllmain), so we do the same instead of introducing another logging backend.
//
// BOT_LOG is only for things a server operator needs to see: failures, features missing on this
// build, and match results. Routine events (spawn, removal, deaths, bus, phase changes, performance)
// use BOT_VERBOSE, which is gated behind FBotSystemConfig::bVerboseLogging (GUI: AI tab, Advanced).
// Neither may be used in per-frame paths.

#define BOT_CATEGORY_SYSTEM "BotSystem"
#define BOT_CATEGORY_SPAWN "BotSpawn"
#define BOT_CATEGORY_AI "BotAI"
#define BOT_CATEGORY_PERF "BotPerf"
#define BOT_CATEGORY_COMPAT "BotCompat"
#define BOT_CATEGORY_NAV "BotNavigation"
#define BOT_CATEGORY_PERCEPTION "BotPerception"
#define BOT_CATEGORY_COSMETICS "BotCosmetics"
#define BOT_CATEGORY_BUILDING "BotBuilding"
#define BOT_CATEGORY_LOOT "BotLoot"
#define BOT_CATEGORY_COMBAT "BotCombat"

#define BOT_LOG(Category, Format, ...) printf("[" Category "] " Format "\n", ##__VA_ARGS__)

#define BOT_VERBOSE(Category, Format, ...)                                                                                                                                                                            \
    do                                                                                                                                                                                                                \
    {                                                                                                                                                                                                                 \
        if (FBotSystemConfig::bVerboseLogging)                                                                                                                                                                        \
            BOT_LOG(Category, Format, ##__VA_ARGS__);                                                                                                                                                                 \
    } while (0)
