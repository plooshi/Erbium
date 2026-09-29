#pragma once
#include "../../../pch.h"
#include "../BotState.h"
#include "../Loot/BotLoot.h"

class FBotController;
struct FBotConfig;

namespace BotDifficulty
{
    void Apply(FBotConfig& Config, EBotDifficulty Difficulty);
    const char* ToString(EBotDifficulty Difficulty);
    bool Parse(const std::string& Text, EBotDifficulty& Out);
}

// Utility-based decision system. Every FBotConfig::DecisionInterval each priority is scored by its own
// small function (no nested if-chains), multiplied by its configurable weight, and the bot switches to
// the state of the best one. A switch needs a clear margin (hysteresis) so bots do not flip between
// states every decision.
//
// The brain only drives *autonomous* states (Searching, Looting, Combat, Healing, Escaping). Commanded
// states (Idle / Moving / Following from cheats) and Building (which returns to the state it
// interrupted) are left alone.
class FBotBrain
{
public:
    explicit FBotBrain(FBotController& InBot);

    void Update(double Now);

    EBotPriority GetPriority() const { return Current; }
    float GetScore(EBotPriority Priority) const { return Scores[(int)Priority]; }

    // Pickup chosen by the last Loot evaluation (read by the Looting behavior).
    const BotLootWorld::FEntry& GetLootTarget() const { return LootTarget; }

    static const char* ToString(EBotPriority Priority);
    static EBotAIState ToState(EBotPriority Priority);
    static bool IsAutonomousState(EBotAIState State);

private:
    float ScoreSearch() const;
    float ScoreLoot(double Now);
    float ScoreCombat(double Now) const;
    float ScoreHealing(double Now) const;
    float ScoreSurvival(double Now) const;

    FBotController& Bot;
    EBotPriority Current = EBotPriority::Search;
    float Scores[(int)EBotPriority::Count] = {};
    double NextDecisionTime = 0.0;
    double StateEnteredTime = 0.0;

    double NextLootSearchTime = 0.0;
    float CachedLootScore = 0.f;
    BotLootWorld::FEntry LootTarget;
};
