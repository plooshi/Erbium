#pragma once
#include "../../pch.h"

class AFortGameMode;
class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;
class AFortInventory;

// Version compatibility layer for the bot system.
//
// Erbium resolves most properties/functions by name at runtime (DEFINE_PROP/DEFINE_FUNC + Has*()),
// which already hides many version differences. What remains are places where the *correct call* or
// the *object layout* differs by Fortnite/engine version. All of those are collected here so the rest
// of the bot system never checks VersionInfo directly.
//
// Every function is safe to call on any version: when an API is missing, it falls back to the closest
// equivalent Erbium already uses elsewhere, or does nothing and returns false.
namespace BotCompat
{
    // Controller class used for bots. Prefers the game mode's PlayerControllerClass (correct for the
    // current playlist/version) and falls back to Athena_PlayerController_C, which the original
    // `cheat spawnbot` implementation used.
    const UClass* GetBotControllerClass(AFortGameMode* GameMode);

    // Pawn class for a given controller (GameMode->GetDefaultPawnClassForController, same as
    // AFortGameMode::SpawnDefaultPawnFor), falling back to GameMode->DefaultPawnClass.
    const UClass* GetBotPawnClass(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller);

    // APlayerController::PostInitializeComponents normally creates a PlayerState on the server. If the
    // spawned controller has none (observed on some builds, which is why the original implementation
    // spawned one manually), create and attach one. Returns the player state in use.
    AFortPlayerStateAthena* EnsurePlayerState(AFortPlayerControllerAthena* Controller, const FTransform& Transform);

    // Same inventory creation path as AFortGameMode::HandleStartingNewPlayer_.
    AFortInventory* EnsureWorldInventory(AFortPlayerControllerAthena* Controller);

    // Possess and bind the pawn. Sets MyFortPawn (required on 27+, see original spawnbot comment) and
    // propagates the player state. Returns true only if possession is verified afterwards.
    bool PossessPawn(AFortPlayerControllerAthena* Controller, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState);

    // Team assignment + GameMemberInfo registration (squad/team UI), mirroring HandleStartingNewPlayer_.
    void AssignTeam(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller, AFortPlayerStateAthena* PlayerState);

    // Pre-9.00 builds only have ServerChangeName; later builds route through GameMode->ChangeName.
    void SetPlayerName(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller, AFortPlayerStateAthena* PlayerState, const wchar_t* Name);

    // Default (non-random) appearance. CharacterParts layout differs between builds (raw array vs
    // FCustomCharacterParts). Phase 3 replaces this with cosmetic loadouts.
    void ApplyDefaultAppearance(AFortPlayerStateAthena* PlayerState, AFortPlayerPawnAthena* Pawn);

    // Adds the controller to GameMode->AlivePlayers and updates GameState->PlayersLeft.
    void AddToAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller);

    // Removes the controller from AlivePlayers if it is still in it. Uses the game's own
    // RemoveFromAlivePlayers when Erbium found it (same call as ClientOnPawnDied), otherwise removes it
    // manually and updates PlayersLeft. Must run before the controller is destroyed: AlivePlayers holds
    // raw pointers.
    void RemoveFromAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller);

    bool IsInAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller);

    // Health/death query that works whether or not the pawn exposes IsDead.
    bool IsPawnDead(AFortPlayerPawnAthena* Pawn);

    // True if the bot is out of the match. A missing pawn alone is not enough: when the battle bus
    // starts, the game moves every alive controller into the aircraft and their ground pawns go away.
    bool IsEliminated(AFortPlayerControllerAthena* Controller, AFortPlayerPawnAthena* Pawn);
}
