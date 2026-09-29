#pragma once
#include "../../pch.h"
#include "BotConfig.h"

// Structured logging for the bot system.
//
// Every line is prefixed with a category tag ([BotSystem], [BotSpawn], [BotAI], ...) so logs can be
// filtered. Erbium logs through printf (stdout is redirected to the console or stdout.log by
// dllmain), so we do the same instead of introducing another logging backend.
//
// BOT_LOG is for lifecycle events (spawn, remove, death, state changes) and must never be used in
// per-frame paths. BOT_VERBOSE is gated behind FBotSystemConfig::bVerboseLogging.

#define BOT_CATEGORY_SYSTEM "BotSystem"
#define BOT_CATEGORY_SPAWN "BotSpawn"
#define BOT_CATEGORY_AI "BotAI"
#define BOT_CATEGORY_PERF "BotPerf"
#define BOT_CATEGORY_COMPAT "BotCompat"

#define BOT_LOG(Category, Format, ...) printf("[" Category "] " Format "\n", ##__VA_ARGS__)

#define BOT_VERBOSE(Category, Format, ...)                                                                                                                                                                            \
    do                                                                                                                                                                                                                \
    {                                                                                                                                                                                                                 \
        if (FBotSystemConfig::bVerboseLogging)                                                                                                                                                                        \
            BOT_LOG(Category, Format, ##__VA_ARGS__);                                                                                                                                                                 \
    } while (0)
