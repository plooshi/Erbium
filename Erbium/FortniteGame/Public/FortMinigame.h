#pragma once
#include "../../pch.h"

class EFortMinigameState
{
public:
    UENUM_COMMON_MEMBERS(EFortMinigameState);

    DEFINE_ENUM_PROP(PreGame);
    DEFINE_ENUM_PROP(Setup);
    DEFINE_ENUM_PROP(Transitioning);
    DEFINE_ENUM_PROP(WaitingForCameras);
    DEFINE_ENUM_PROP(Warmup);
    DEFINE_ENUM_PROP(InProgress);
    DEFINE_ENUM_PROP(PostGameTimeDilation);
    DEFINE_ENUM_PROP(PostRoundEnd);
    DEFINE_ENUM_PROP(PostGameEnd);
    DEFINE_ENUM_PROP(PostGameAbandon);
    DEFINE_ENUM_PROP(PostGameReset);
};

class AFortMinigame : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortMinigame);
    
    DEFINE_PROP(NumTeams, int32);
    DEFINE_PROP(CurrentState, uint8);
    
    DEFINE_FUNC(GetParticipatingPlayers, void);
    DEFINE_FUNC(OnPlayerPawnPossessedDuringTransition, void);
    DEFINE_FUNC(OnClientFinishTeleportingForMinigame, void);
    DEFINE_FUNC(AdvanceState, void);
    DEFINE_FUNC(HandleMinigameStarted, void);
    DEFINE_FUNC(HandleVolumeEditModeChange, void);    
public:
    DefHookOg(void, SetState, AFortMinigame* Minigame, uint8 NewState);
    InitHooks;
};