#pragma once
#include "../../pch.h"
#include "BotBehavior.h"
#include "BotConfig.h"
#include "BotState.h"

class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;

// Per-bot AI driver.
//
// Why this is not a UClass: Erbium is injected into a shipped game binary. It can call and hook
// existing reflected classes, but it cannot register new UClasses (there is no UHT/reflection data
// for code compiled into the DLL). So a bot is a *real* engine AFortPlayerControllerAthena possessing
// a *real* AFortPlayerPawnAthena -- exactly the objects a human player gets -- and FBotController is
// a plain C++ object that drives them from the outside.
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

    // Engine objects. Each returns nullptr if the object no longer exists or is being destroyed.
    AFortPlayerControllerAthena* GetController() const;
    AFortPlayerPawnAthena* GetPawn() const;
    AFortPlayerStateAthena* GetPlayerState() const;

    EBotLifeState GetLifeState() const { return LifeState; }
    EBotAIState GetAIState() const { return Behavior ? Behavior->GetState() : EBotAIState::None; }
    bool IsAlive() const { return LifeState == EBotLifeState::Active; }

    // Called by the spawner once the controller exists. Possession is done separately so failures
    // can be detected and reported.
    void BindController(AFortPlayerControllerAthena* InController, AFortPlayerStateAthena* InPlayerState);

    // Records the possessed pawn. Returns false if the controller does not actually possess it.
    bool OnPossessed(AFortPlayerPawnAthena* InPawn);

    // Marks the bot as fully initialized and enters the Idle AI state.
    void Activate(double Now);

    // Death notification, either from the ClientOnPawnDied hook or detected by Validate().
    void OnPawnDied(double Now);

    // Low-frequency maintenance: detects a destroyed/dead pawn or a destroyed controller.
    // Returns false when the bot should be removed from the manager.
    bool Validate(double Now);

    // Runs the current behavior. Called at FBotSystemConfig::ThinkInterval.
    void Think(double Now);

    void SetAIState(EBotAIState NewState);

    void MarkPendingRemoval() { LifeState = EBotLifeState::PendingRemoval; }
    double GetDeathTime() const { return DeathTime; }

    // Next time (world seconds) Think() should run. Owned by the manager for staggering.
    double NextThinkTime = 0.0;

private:
    uint32 Id;
    FBotConfig Config;

    EBotLifeState LifeState = EBotLifeState::Spawning;
    std::unique_ptr<IBotBehavior> Behavior;

    TWeakObjectPtr<AFortPlayerControllerAthena> Controller;
    TWeakObjectPtr<AFortPlayerPawnAthena> Pawn;
    TWeakObjectPtr<AFortPlayerStateAthena> PlayerState;

    double LastThinkTime = 0.0;
    double DeathTime = 0.0;
};
