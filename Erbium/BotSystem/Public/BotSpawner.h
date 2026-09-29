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
    // On any failure every actor created so far is destroyed and false is returned.
    bool SpawnBot(FBotController& Bot, const FBotSpawnParams& Params);

    // Tears the bot down in reverse order. Safe to call on a partially spawned or already
    // half-destroyed bot; every step checks that its object still exists.
    void DestroyBot(FBotController& Bot);
}
