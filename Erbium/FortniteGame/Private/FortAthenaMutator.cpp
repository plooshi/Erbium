#include "pch.h"
#include "../Public/FortAthenaMutator.h"

void AFortAthenaMutator_GiveItemsAtGamePhaseStep::OnGamePhaseStepChanged(UObject* Context, FFrame& Stack)
{
    TScriptInterface<IInterface> SafeZoneInterface;
    uint8_t GamePhaseStep;
    
    Stack.StepCompiledIn(&SafeZoneInterface);
    Stack.StepCompiledIn(&GamePhaseStep);
    Stack.IncrementCode();
    auto Mutator = (AFortAthenaMutator_GiveItemsAtGamePhaseStep*)Context;

    if (GamePhaseStep == Mutator->PhaseToGiveItems)
        for (auto& UncastedPC : Mutator->CachedGameMode->AlivePlayers)
        {
            auto PlayerController = (AFortPlayerControllerAthena*)UncastedPC;

            for (auto& Item : Mutator->ItemsToGive)
                PlayerController->WorldInventory->GiveItem(Item.ItemToDrop, Item.NumberToGive.Evaluate());
        }
}

void AFortAthenaMutator_GiveItemsAtGamePhase::OnGamePhaseChanged(UObject* Context, FFrame& Stack)
{
    uint8_t GamePhase;

    Stack.StepCompiledIn(&GamePhase);
    Stack.IncrementCode();
    auto Mutator = (AFortAthenaMutator_GiveItemsAtGamePhaseStep*)Context;

    if (GamePhase == Mutator->PhaseToGiveItems)
        for (auto& UncastedPC : Mutator->CachedGameMode->AlivePlayers)
        {
            auto PlayerController = (AFortPlayerControllerAthena*)UncastedPC;

            for (auto& Item : Mutator->ItemsToGive)
                PlayerController->WorldInventory->GiveItem(Item.ItemToDrop, Item.NumberToGive.Evaluate());
        }
}

void AFortAthenaMutator_GiveItemsAtGamePhaseStep::Hook()
{
    Utils::ExecHook(GetDefaultObj()->GetFunction("OnGamePhaseStepChanged"), OnGamePhaseStepChanged);
}

void AFortAthenaMutator_GiveItemsAtGamePhase::Hook()
{
    Utils::ExecHook(GetDefaultObj()->GetFunction("OnGamePhaseChanged"), OnGamePhaseChanged);
}