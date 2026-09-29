#pragma once
#include "../../pch.h"
#include "BotState.h"
#include <array>

class AController;
class AFortGameMode;
class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;
class AFortInventory;

// Movement speed request. How (and whether) it maps onto the game depends on the build.
enum class EBotMoveSpeed : uint8
{
    Walk,
    Run,
    Sprint,
};

// Version / game-integration compatibility layer for the bot system.
//
// Erbium resolves most properties/functions by name at runtime (DEFINE_PROP/DEFINE_FUNC + Has*()),
// which already hides many version differences. What remains are places where the *correct call*,
// the *object layout*, or the *controller type* matters. All of those are collected here so the rest
// of the bot system never checks VersionInfo or the controller type directly.
//
// Every function is safe to call on any version: when an API is missing, it falls back to the closest
// equivalent Erbium already uses elsewhere, or does nothing and returns false.
namespace BotCompat
{
    struct FControllerClassChoice
    {
        const UClass* Class = nullptr;
        EBotControllerKind Kind = EBotControllerKind::PlayerController;
    };

    // Controller classes to try, most preferred first, according to FBotSystemConfig::ControllerMode.
    // AI candidates: the engine AAIController (plain, no built-in Fortnite bot logic), then Fortnite's
    // FortAthenaAIBotController. Player candidate: GameMode->PlayerControllerClass, then
    // Athena_PlayerController_C (what the original `cheat spawnbot` used).
    std::vector<FControllerClassChoice> GetBotControllerCandidates(AFortGameMode* GameMode);

    // Pawn class for a controller (GameMode->GetDefaultPawnClassForController, same as
    // AFortGameMode::SpawnDefaultPawnFor), falling back to GameMode->DefaultPawnClass.
    const UClass* GetBotPawnClass(AFortGameMode* GameMode, AController* Controller);

    // Player controllers (and Fortnite's AI bot controller) create a player state on the server; the
    // plain engine AIController does not. Creates and attaches one when missing.
    AFortPlayerStateAthena* EnsurePlayerState(AController* Controller, const FTransform& Transform);

    // Same inventory creation path as AFortGameMode::HandleStartingNewPlayer_. Player controllers only
    // for now: Erbium's inventory code expects the owner to implement the Fortnite inventory-owner
    // interface, which a plain AIController does not. AI inventories arrive with looting (Phase 2B).
    AFortInventory* EnsureWorldInventory(AController* Controller, EBotControllerKind Kind);

    // Stops any behavior tree / brain the controller class starts on its own, so it cannot fight the
    // bot system for control of the pawn.
    void DisableBuiltInAI(AController* Controller, EBotControllerKind Kind);

    // Possess and bind the pawn, then verify. For player controllers also sets MyFortPawn (required on
    // 27+, see original spawnbot comment).
    bool PossessPawn(AController* Controller, EBotControllerKind Kind, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState);

    // Team assignment + GameMemberInfo registration (squad/team UI), mirroring HandleStartingNewPlayer_.
    void AssignTeam(AFortGameMode* GameMode, AController* Controller, AFortPlayerStateAthena* PlayerState);

    // Pre-9.00 player controllers only have ServerChangeName; everything else goes through
    // AGameModeBase::ChangeName, which accepts any controller.
    void SetPlayerName(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind, AFortPlayerStateAthena* PlayerState, const wchar_t* Name);

    // Character parts indexed by EFortCustomPartType (Head, Body, Hat, Backpack, ...). nullptr = unset.
    using FCharacterParts = std::array<const UObject*, 8>;

    // Shows the given parts on the bot. Two mechanisms, both used when available:
    //  - APawn ServerChoosePart per part (what Erbium's native-bot cosmetics use),
    //  - the player state's CharacterParts (raw array or FCustomCharacterParts, depending on the build)
    //    followed by the same visual refresh ServerAcknowledgePossession uses for real players.
    // Returns false if neither mechanism exists on this build.
    bool ApplyCharacterParts(AFortPlayerStateAthena* PlayerState, AFortPlayerPawnAthena* Pawn, const FCharacterParts& Parts);

    // Fallback appearance (default commando) when cosmetics are disabled or the outfit fails.
    void ApplyDefaultAppearance(AFortPlayerStateAthena* PlayerState, AFortPlayerPawnAthena* Pawn);

    // Counts the bot as an alive player.
    //  - Player controllers join GameMode->AlivePlayers (and PlayersLeft), exactly like the original.
    //  - AI controllers only increase GameState->PlayersLeft. They must NOT be added to AlivePlayers:
    //    Erbium iterates that array in many places and casts every entry to AFortPlayerControllerAthena.
    void RegisterAlive(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind);

    // Reverse of RegisterAlive. Safe to call more than once; FBotController tracks whether the bot is
    // still counted. For player controllers uses the game's own RemoveFromAlivePlayers when Erbium
    // found it, then makes sure the (raw-pointer) AlivePlayers entry is gone before destruction.
    void UnregisterAlive(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind);

    // Health/death query that works whether or not the pawn exposes IsDead.
    bool IsPawnDead(AFortPlayerPawnAthena* Pawn);

    // True if the bot is out of the match. A missing pawn alone is not enough for player controllers:
    // when the battle bus starts, the game moves every AlivePlayers controller into the aircraft and
    // their ground pawns go away.
    bool IsEliminated(AController* Controller, EBotControllerKind Kind, AFortPlayerPawnAthena* Pawn);

    // Requests a movement speed. Uses the pawn's CurrentMovementStyle with values resolved by name from
    // EFortMovementStyle. Returns false (and logs once) when the build does not expose it; the pawn then
    // keeps its default (running) speed.
    bool SetMovementSpeed(AFortPlayerPawnAthena* Pawn, EBotMoveSpeed Speed);
}
