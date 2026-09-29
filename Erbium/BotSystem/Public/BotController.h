#pragma once
#include "../../pch.h"
#include "BotBehavior.h"
#include "BotConfig.h"
#include "BotState.h"
#include "Navigation/BotMovement.h"
#include "Perception/BotPerception.h"

class AController;
class AAIController;
class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;

// Per-bot AI driver.
//
// Why this is not a UClass: Erbium is injected into a shipped game binary. It can call and hook
// existing reflected classes, but it cannot register new UClasses (there is no UHT/reflection data
// for code compiled into the DLL). So a bot is a *real* engine controller possessing a *real*
// AFortPlayerPawnAthena, and FBotController is a plain C++ object that drives them from the outside.
//
// The engine controller is either an AI controller (can move; see EBotControllerKind) or, as a
// fallback, a client-less player controller. Code outside BotCompat never needs to care which: use
// GetController() for anything AController offers and CanMove() to know whether movement works.
//
// All engine references are TWeakObjectPtr: they validate the object index + serial number on every
// access, so a pawn destroyed and garbage-collected by the game yields nullptr instead of a dangling
// pointer.
class FBotController
{
public:
    FBotController(uint32 InId, const FBotConfig& InConfig);
    ~FBotController();

    FBotController(const FBotController&) = delete;
    FBotController& operator=(const FBotController&) = delete;

    uint32 GetId() const { return Id; }
    const FBotConfig& GetConfig() const { return Config; }
    EBotControllerKind GetControllerKind() const { return ControllerKind; }

    // Engine objects. Each returns nullptr if the object no longer exists or is being destroyed.
    AController* GetController() const;
    AAIController* GetAIController() const;             // nullptr for the player-controller backend
    AFortPlayerControllerAthena* GetPlayerController() const; // nullptr for the AI backend
    AFortPlayerPawnAthena* GetPawn() const;
    AFortPlayerStateAthena* GetPlayerState() const;

    // Whether this bot's pawn can be driven at all (AI controller backend).
    bool CanMove() const { return ControllerKind == EBotControllerKind::AIController; }

    EBotLifeState GetLifeState() const { return LifeState; }
    EBotAIState GetAIState() const { return Behavior ? Behavior->GetState() : EBotAIState::None; }
    bool IsAlive() const { return LifeState == EBotLifeState::Active; }

    FBotMovement& GetMovement() { return Movement; }
    const FBotMovement& GetMovement() const { return Movement; }
    FBotPerception& GetPerception() { return Perception; }
    const FBotPerception& GetPerception() const { return Perception; }

    // Called by the spawner once the controller exists. Possession is done separately so failures
    // can be detected and reported.
    void BindController(AController* InController, EBotControllerKind InKind, AFortPlayerStateAthena* InPlayerState);

    // Records the possessed pawn. Returns false if the controller does not actually possess it.
    bool OnPossessed(AFortPlayerPawnAthena* InPawn);

    // Marks the bot as fully initialized and enters its default AI state.
    void Activate(double Now);

    // Death notification, either from the ClientOnPawnDied hook or detected by Validate().
    void OnPawnDied(double Now);

    // Low-frequency maintenance: detects a destroyed/dead pawn or a destroyed controller.
    // Returns false when the bot should be removed from the manager.
    bool Validate(double Now);

    // Perception + behavior + movement. Called at FBotSystemConfig::ThinkInterval.
    void Think(double Now);

    // Every server frame (only movement modes that need continuous input do work here).
    void TickFrame();

    void SetAIState(EBotAIState NewState);

    // Commands, e.g. from cheats. They set the data the Moving/Following behaviors read, then switch.
    void CommandMoveTo(const FVector& Location);
    void CommandFollow(AActor* Target);
    const FVector& GetCommandLocation() const { return CommandLocation; }
    AActor* GetCommandTarget() const;

    // Whether the bot currently counts towards the match's alive players (see BotCompat::RegisterAlive).
    bool bRegisteredAlive = false;

    void MarkPendingRemoval() { LifeState = EBotLifeState::PendingRemoval; }
    double GetDeathTime() const { return DeathTime; }

    // Next time (world seconds) Think() should run. Owned by the manager for staggering.
    double NextThinkTime = 0.0;

private:
    uint32 Id;
    FBotConfig Config;

    EBotLifeState LifeState = EBotLifeState::Spawning;
    EBotControllerKind ControllerKind = EBotControllerKind::PlayerController;
    std::unique_ptr<IBotBehavior> Behavior;

    FBotMovement Movement;
    FBotPerception Perception;

    TWeakObjectPtr<AController> Controller;
    TWeakObjectPtr<AFortPlayerPawnAthena> Pawn;
    TWeakObjectPtr<AFortPlayerStateAthena> PlayerState;

    FVector CommandLocation{};
    TWeakObjectPtr<AActor> CommandTarget;

    double LastThinkTime = 0.0;
    double DeathTime = 0.0;
};
