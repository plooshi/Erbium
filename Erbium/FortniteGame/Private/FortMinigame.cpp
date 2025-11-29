#include "pch.h"
#include "../Public/FortMinigame.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortPlayerStateAthena.h"
#include <thread>

void AFortMinigame::SetState(AFortMinigame* Minigame, uint8 NewState)
{
    TArray<AFortPlayerStateAthena *> Players;
    Minigame->GetParticipatingPlayers(&Players);

    printf("[CreativeRuntime] (SetState): %d\n", NewState);

    if (NewState == EFortMinigameState::GetTransitioning())
    {
        for (int i = 0; i < Players.Num(); i++)
        {
            AFortPlayerStateAthena* PlayerState = Players[i];
            if (PlayerState)
            {
                AFortPlayerControllerAthena* Controller = PlayerState->GetOwner()->Cast<AFortPlayerControllerAthena>();
                if (Controller && Controller->MyFortPawn)
                {
                    if (Minigame->NumTeams == 0)
                        Controller->ServerSetTeam(i + 3);
                    else
                        Controller->ServerSetTeam((i % Minigame->NumTeams) + 3);
                            
                    Minigame->OnPlayerPawnPossessedDuringTransition(&Controller->MyFortPawn);
                }
            }
        }
                
        Minigame->AdvanceState();
        Minigame->HandleMinigameStarted();
    }
    else if (NewState == EFortMinigameState::GetWaitingForCameras())
    {
        for (int i = 0; i < Players.Num(); i++)
        {
            auto Player = Players[i]->Cast<AFortPlayerStateAthena>();
            auto Controller = Player->GetOwner()->Cast<AFortPlayerControllerAthena>();
            if (!Controller)
                continue;
            auto Pawn = Controller->MyFortPawn;
            if (!Pawn)
                continue;
                    
            Minigame->OnClientFinishTeleportingForMinigame(&Pawn);
        }
                
        // this can crash btw!
        std::thread([Minigame, NewState]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            SetStateOG(Minigame, NewState);
        }).detach();    
    }
    else if (NewState == EFortMinigameState::GetPostGameReset())
    {
        SetStateOG(Minigame, NewState);
        for (int i = 0; i < Players.Num(); i++)
        {
            auto Player = Players[i]->Cast<AFortPlayerStateAthena>();
            auto Controller = Player->GetOwner()->Cast<AFortPlayerControllerAthena>();
            if (!Controller)
                continue;
            auto Pawn = Controller->MyFortPawn;
            if (!Pawn)
                continue;
                    
            Minigame->OnPlayerPawnPossessedDuringTransition(&Pawn);
        }
                
        Minigame->CurrentState = (uint8_t)EFortMinigameState::GetPreGame();
    }
    
    if (NewState != EFortMinigameState::GetWaitingForCameras() && NewState != EFortMinigameState::GetPostGameReset())
        return SetStateOG(Minigame, NewState);
}

void AFortMinigame::Hook()
{
    if (!GetDefaultObj())
        return;
    
    //Utils::Hook(FindSetState(), SetState, SetStateOG);
}
