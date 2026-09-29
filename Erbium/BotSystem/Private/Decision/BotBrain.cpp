#include "pch.h"
#include "../../Public/Decision/BotBrain.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"

// A new priority must beat the current one by this factor to cause a switch.
static constexpr float SwitchMargin = 1.15f;
// Minimum time in an autonomous state before the brain may leave it (Survival ignores this).
static constexpr double MinStateTime = 0.75;

namespace BotDifficulty
{
    void Apply(FBotConfig& Config, EBotDifficulty Difficulty)
    {
        switch (Difficulty)
        {
        case EBotDifficulty::Easy:
            Config.AimReactionTime = 0.9f;
            Config.AimAccuracy = 0.35f;
            Config.AimTurnSpeed = 90.f;
            Config.TargetSwitchDelay = 2.0f;
            Config.MaxAimDistance = 4000.f;
            Config.DetectionRadius = 4000.f;
            Config.Aggression = 0.6f;
            Config.RepositionInterval = 5.f;
            Config.BuildReactionTime = 0.9f;
            Config.DefensiveBuildChance = 0.25f;
            Config.MaxBuildsPerMinute = 4;
            Config.bCanSprint = false;
            break;
        case EBotDifficulty::Normal:
            Config.AimReactionTime = 0.5f;
            Config.AimAccuracy = 0.6f;
            Config.AimTurnSpeed = 180.f;
            Config.TargetSwitchDelay = 1.0f;
            Config.MaxAimDistance = 6000.f;
            Config.DetectionRadius = 5000.f;
            Config.Aggression = 1.0f;
            Config.RepositionInterval = 3.5f;
            Config.BuildReactionTime = 0.5f;
            Config.DefensiveBuildChance = 0.5f;
            Config.MaxBuildsPerMinute = 10;
            Config.bCanSprint = true;
            break;
        case EBotDifficulty::Hard:
            Config.AimReactionTime = 0.25f;
            Config.AimAccuracy = 0.82f;
            Config.AimTurnSpeed = 320.f;
            Config.TargetSwitchDelay = 0.5f;
            Config.MaxAimDistance = 8000.f;
            Config.DetectionRadius = 6500.f;
            Config.Aggression = 1.4f;
            Config.RepositionInterval = 2.0f;
            Config.BuildReactionTime = 0.25f;
            Config.DefensiveBuildChance = 0.8f;
            Config.MaxBuildsPerMinute = 20;
            Config.bCanSprint = true;
            break;
        }

        Config.Difficulty = Difficulty;
    }

    const char* ToString(EBotDifficulty Difficulty)
    {
        switch (Difficulty)
        {
        case EBotDifficulty::Easy:
            return "Easy";
        case EBotDifficulty::Normal:
            return "Normal";
        case EBotDifficulty::Hard:
            return "Hard";
        }
        return "Unknown";
    }

    bool Parse(const std::string& Text, EBotDifficulty& Out)
    {
        std::string Lower = Text;
        std::transform(Lower.begin(), Lower.end(), Lower.begin(), [](unsigned char C) { return (char)tolower(C); });

        if (Lower == "easy")
            Out = EBotDifficulty::Easy;
        else if (Lower == "normal")
            Out = EBotDifficulty::Normal;
        else if (Lower == "hard")
            Out = EBotDifficulty::Hard;
        else
            return false;

        return true;
    }
}

FBotBrain::FBotBrain(FBotController& InBot)
    : Bot(InBot)
{
}

const char* FBotBrain::ToString(EBotPriority Priority)
{
    switch (Priority)
    {
    case EBotPriority::Search:
        return "Search";
    case EBotPriority::Loot:
        return "Loot";
    case EBotPriority::Combat:
        return "Combat";
    case EBotPriority::Healing:
        return "Healing";
    case EBotPriority::Survival:
        return "Survival";
    default:
        return "Unknown";
    }
}

EBotAIState FBotBrain::ToState(EBotPriority Priority)
{
    switch (Priority)
    {
    case EBotPriority::Loot:
        return EBotAIState::Looting;
    case EBotPriority::Combat:
        return EBotAIState::Combat;
    case EBotPriority::Healing:
        return EBotAIState::Healing;
    case EBotPriority::Survival:
        return EBotAIState::Escaping;
    default:
        return EBotAIState::Searching;
    }
}

bool FBotBrain::IsAutonomousState(EBotAIState State)
{
    return State == EBotAIState::Searching || State == EBotAIState::Looting || State == EBotAIState::Combat || State == EBotAIState::Healing || State == EBotAIState::Escaping;
}

// --- Scores ----------------------------------------------------------------------------------------

float FBotBrain::ScoreSearch() const
{
    return 0.2f;
}

float FBotBrain::ScoreLoot(double Now)
{
    if (!Bot.GetConfig().bCanLoot)
        return 0.f;

    // Scanning pickups is the most expensive score; refresh it at most once per second.
    if (Now >= NextLootSearchTime)
    {
        NextLootSearchTime = Now + 1.0;
        CachedLootScore = 0.f;

        BotLootWorld::FEntry Entry;
        float Score = 0.f;
        if (BotLoot::FindBestPickup(Bot, Entry, Score))
        {
            LootTarget = Entry;
            CachedLootScore = Score;
        }
    }

    // A weapon when unarmed scores ~200 -> about 1.0.
    return (std::min)(CachedLootScore / 200.f, 1.5f);
}

static bool HasUsableWeapon(FBotController& Bot, float Distance)
{
    auto& Inventory = Bot.GetInventory();
    auto Items = Inventory.GetItems();

    if (Inventory.GetBestWeaponFor(Distance, Items))
        return true;

    // Pickaxe only makes sense up close.
    return Distance < 600.f && Inventory.GetBestOf(EBotItemCategory::Melee, Items);
}

float FBotBrain::ScoreCombat(double Now) const
{
    auto& Config = Bot.GetConfig();

    for (auto& Perceived : Bot.GetPerception().GetPawns())
    {
        if (!Perceived.bIsEnemy || Perceived.Distance > Config.MaxAimDistance)
            continue;

        bool bRecent = Perceived.bVisible || (Perceived.LastSeenTime > 0.0 && Now - Perceived.LastSeenTime <= Config.TargetMemory);
        if (!bRecent)
            continue;

        if (!HasUsableWeapon(Bot, Perceived.Distance))
            return 0.f;

        // Nearest relevant enemy decides; perception is sorted by distance.
        return Config.Aggression * (Perceived.bVisible ? 1.f : 0.6f) * (1.2f - 0.5f * Perceived.Distance / Config.MaxAimDistance);
    }

    return 0.f;
}

static float HealthFraction(FBotController& Bot, float& OutHealth, float& OutShield)
{
    auto Pawn = Bot.GetPawn();
    OutHealth = Pawn ? Pawn->GetHealth() : 0.f;
    OutShield = Pawn ? Pawn->GetShield() : 0.f;
    return (OutHealth + OutShield) / 200.f;
}

float FBotBrain::ScoreHealing(double Now) const
{
    auto& Config = Bot.GetConfig();
    if (!Config.bCanHeal)
        return 0.f;

    // Not while being shot at or with an enemy close by.
    if (Bot.GetLastDamageTime() > 0.0 && Now - Bot.GetLastDamageTime() < 2.0)
        return 0.f;

    auto Enemy = Bot.GetPerception().GetNearestVisibleEnemy();
    if (Enemy && Enemy->Distance < Config.SafeHealDistance)
        return 0.f;

    float Health, Shield;
    HealthFraction(Bot, Health, Shield);

    auto& Inventory = Bot.GetInventory();
    auto Items = Inventory.GetItems();

    float Need = 0.f;
    if (Health < Config.HealHealthThreshold && Inventory.GetBestOf(EBotItemCategory::Healing, Items))
        Need = (std::max)(Need, 1.f - Health / 100.f);
    if (Shield < Config.HealShieldThreshold && Inventory.GetBestOf(EBotItemCategory::Shield, Items))
        Need = (std::max)(Need, 1.f - Shield / 100.f);

    return Need * 1.2f;
}

float FBotBrain::ScoreSurvival(double Now) const
{
    auto& Config = Bot.GetConfig();

    float Health, Shield;
    auto Fraction = HealthFraction(Bot, Health, Shield);

    bool bThreatened = (Bot.GetLastDamageTime() > 0.0 && Now - Bot.GetLastDamageTime() < 3.0) || Bot.GetPerception().GetNearestVisibleEnemy();
    if (!bThreatened)
        return 0.f;

    // Low on health, or threatened with nothing to fight back with.
    if (Fraction < Config.RetreatHealthFraction)
        return 1.3f * (1.f - Fraction / Config.RetreatHealthFraction) + 0.5f;

    auto Enemy = Bot.GetPerception().GetNearestVisibleEnemy();
    if (Enemy && !HasUsableWeapon(Bot, Enemy->Distance))
        return 0.6f;

    return 0.f;
}

// --- Decision --------------------------------------------------------------------------------------

void FBotBrain::Update(double Now)
{
    auto& Config = Bot.GetConfig();
    if (Now < NextDecisionTime || !Bot.IsAlive() || !Bot.CanMove())
        return;

    NextDecisionTime = Now + Config.DecisionInterval;

    auto State = Bot.GetAIState();
    if (!IsAutonomousState(State))
        return;

    Scores[(int)EBotPriority::Search] = ScoreSearch() * Config.PriorityWeights[(int)EBotPriority::Search];
    Scores[(int)EBotPriority::Loot] = ScoreLoot(Now) * Config.PriorityWeights[(int)EBotPriority::Loot];
    Scores[(int)EBotPriority::Combat] = ScoreCombat(Now) * Config.PriorityWeights[(int)EBotPriority::Combat];
    Scores[(int)EBotPriority::Healing] = ScoreHealing(Now) * Config.PriorityWeights[(int)EBotPriority::Healing];
    Scores[(int)EBotPriority::Survival] = ScoreSurvival(Now) * Config.PriorityWeights[(int)EBotPriority::Survival];

    auto Best = EBotPriority::Search;
    for (int i = 0; i < (int)EBotPriority::Count; i++)
        if (Scores[i] > Scores[(int)Best])
            Best = (EBotPriority)i;

    // The state the bot is in may differ from Current (e.g. after a command ended); treat the state's
    // own priority as the one to beat.
    auto CurrentPriority = EBotPriority::Search;
    for (int i = 0; i < (int)EBotPriority::Count; i++)
        if (ToState((EBotPriority)i) == State)
            CurrentPriority = (EBotPriority)i;

    if (Best == CurrentPriority)
    {
        Current = Best;
        return;
    }

    bool bMargin = Scores[(int)Best] > Scores[(int)CurrentPriority] * SwitchMargin;
    bool bMinTimeOver = Now - StateEnteredTime >= MinStateTime || Best == EBotPriority::Survival;
    if (!bMargin || !bMinTimeOver)
        return;

    BOT_VERBOSE(BOT_CATEGORY_AI, "Bot #%u decides: %s (%.2f) over %s (%.2f)", Bot.GetId(), ToString(Best), Scores[(int)Best], ToString(CurrentPriority), Scores[(int)CurrentPriority]);

    Current = Best;
    StateEnteredTime = Now;
    Bot.SetAIState(ToState(Best));
}
