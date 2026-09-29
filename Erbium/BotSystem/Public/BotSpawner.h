#pragma once
#include "../../pch.h"

class FBotController;
class AFortGameMode;

struct FBotSpawnParams
{
    // When false the spawner picks a player start through the game mode (same as real players).
    bool bUseLocation = false;
    FVector Location{};
    FRotator Rotation{};

    // Scatter bots around Location/player start so batches do not stack. 0 disables.
    float ScatterRadius = 0.f;
};

// Creates and destroys the engine actors behind a bot. Stateless: the manager decides *when*,
// the spawner knows *how*.
namespace BotSpawner
{
    // Full spawn sequence:
    //   1. controller  2. player state  3. inventory  4. spawn location  5. pawn
    //   6. possess (verified)  7. team / abilities / starting items / appearance / name
    //   8. alive players
    // Steps 1-6 are tried with every controller class candidate in turn (AI controller first, player
    // controller last in Auto mode): a build where, say, possession by an AI controller fails still gets
    // bots. On a failed attempt every actor it created is destroyed before the next one.
    // Returns false only when every candidate failed.
    bool SpawnBot(FBotController& Bot, const FBotSpawnParams& Params);

    // Why the most recent spawn failed, or "none" (a string literal; safe to read from the GUI thread).
    const char* GetLastError();

    // Tears the bot down in reverse order. Safe to call on a partially spawned or already
    // half-destroyed bot; every step checks that its object still exists.
    void DestroyBot(FBotController& Bot);
}
