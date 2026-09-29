#include "pch.h"
#include "../Public/BotManager.h"
#include "../Public/BotCompat.h"
#include "../Public/BotLog.h"
#include "../Public/BotSpawner.h"
#include "../Public/Perception/BotPerception.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../Erbium/Public/GUI.h"

static double NowMicros()
{
    static LARGE_INTEGER Frequency = []
    {
        LARGE_INTEGER F;
        QueryPerformanceFrequency(&F);
        return F;
    }();

    LARGE_INTEGER Counter;
    QueryPerformanceCounter(&Counter);
    return (double)Counter.QuadPart * 1000000.0 / (double)Frequency.QuadPart;
}

double FBotManager::GetWorldTime()
{
    auto World = UWorld::GetWorld();
    return World ? UGameplayStatics::GetTimeSeconds(World) : 0.0;
}

bool FBotManager::CanSpawnMore(int Additional)
{
    if (!FBotSystemConfig::bEnabled)
        return false;

    // Before this the world/game mode is still being set up (see AFortGameMode::ReadyToStartMatch_).
    if (GUI::gsStatus < Joinable)
        return false;

    return (int)Bots.size() + Additional <= FBotSystemConfig::MaxBots;
}

FBotController* FBotManager::SpawnBotInternal(const FSpawnRequest& Request)
{
    if (!CanSpawnMore(1))
    {
        BOT_LOG(BOT_CATEGORY_SPAWN, "Spawn rejected (enabled=%d, bots=%d/%d, server ready=%d).", FBotSystemConfig::bEnabled, (int)Bots.size(), FBotSystemConfig::MaxBots, GUI::gsStatus >= Joinable);
        return nullptr;
    }

    FBotSpawnParams Params;
    Params.bUseLocation = Request.bUseLocation;
    Params.Location = FVector(Request.Location);
    Params.ScatterRadius = FBotSystemConfig::SpawnScatterRadius;

    auto Bot = std::make_unique<FBotController>(NextBotId++, FBotConfig{});

    auto Start = NowMicros();
    bool bSpawned = BotSpawner::SpawnBot(*Bot, Params);
    auto Elapsed = NowMicros() - Start;

    if (!bSpawned)
        return nullptr;

    PerfSpawnCount++;
    PerfSpawnMicros += Elapsed;

    auto Now = GetWorldTime();
    Bot->Activate(Now);

    // Stagger thinking so bots spawned together do not all run on the same frame.
    Bot->NextThinkTime = Now + FBotSystemConfig::ThinkInterval * (double)(Bot->GetId() % 10) / 10.0;

    BOT_LOG(BOT_CATEGORY_SPAWN, "Spawned bot #%u (%s) in %.2fms (%d active).", Bot->GetId(), ToString(Bot->GetControllerKind()), Elapsed / 1000.0, (int)Bots.size() + 1);

    Bots.push_back(std::move(Bot));
    return Bots.back().get();
}

FBotController* FBotManager::SpawnBot()
{
    return SpawnBotInternal({});
}

FBotController* FBotManager::SpawnBot(const FVector& Location, const FRotator& Rotation)
{
    FSpawnRequest Request;
    Request.bUseLocation = true;
    Request.Location = FVector(Location);
    auto Bot = SpawnBotInternal(Request);

    if (Bot)
        if (auto Pawn = Bot->GetPawn())
            Pawn->K2_SetActorRotation(Rotation, false);

    return Bot;
}

int FBotManager::SpawnBots(int Count, const FVector* Near)
{
    // Clamp to what can still fit, counting bots already queued.
    auto Available = FBotSystemConfig::MaxBots - (int)Bots.size() - (int)SpawnQueue.size();
    // (std::max) is parenthesized throughout: <windows.h> defines min/max macros and the project does not set NOMINMAX.
    auto ToQueue = (std::min)((std::max)(Count, 0), (std::max)(Available, 0));

    for (int i = 0; i < ToQueue; i++)
    {
        FSpawnRequest Request;
        if (Near)
        {
            Request.bUseLocation = true;
            Request.Location = FVector(*Near);
        }
        SpawnQueue.push_back(Request);
    }

    if (ToQueue < Count)
        BOT_LOG(BOT_CATEGORY_SPAWN, "Requested %d bots, queued %d (MaxBots = %d).", Count, ToQueue, FBotSystemConfig::MaxBots);

    return ToQueue;
}

bool FBotManager::RemoveBot(FBotController* Bot)
{
    if (!Bot)
        return false;

    Bot->MarkPendingRemoval();

    // During Tick the vector may be being iterated; removals are flushed at the end of the tick.
    if (!bIsTicking)
        FlushPendingRemovals();

    return true;
}

bool FBotManager::RemoveBot(uint32 BotId)
{
    return RemoveBot(FindBot(BotId));
}

void FBotManager::RemoveAllBots()
{
    SpawnQueue.clear();

    for (auto& Bot : Bots)
        Bot->MarkPendingRemoval();

    if (!bIsTicking)
        FlushPendingRemovals();
}

void FBotManager::FlushPendingRemovals()
{
    for (size_t i = 0; i < Bots.size();)
    {
        auto& Bot = Bots[i];

        if (Bot->GetLifeState() != EBotLifeState::PendingRemoval)
        {
            i++;
            continue;
        }

        BotSpawner::DestroyBot(*Bot);
        BOT_LOG(BOT_CATEGORY_SYSTEM, "Removed bot #%u (%d remaining).", Bot->GetId(), (int)Bots.size() - 1);

        Bots.erase(Bots.begin() + i);
    }
}

std::vector<FBotController*> FBotManager::GetActiveBots()
{
    std::vector<FBotController*> Result;
    Result.reserve(Bots.size());

    for (auto& Bot : Bots)
        if (Bot->GetLifeState() == EBotLifeState::Active)
            Result.push_back(Bot.get());

    return Result;
}

int FBotManager::GetBotCount()
{
    return (int)Bots.size();
}

int FBotManager::GetPendingSpawnCount()
{
    return (int)SpawnQueue.size();
}

FBotController* FBotManager::FindBot(uint32 BotId)
{
    for (auto& Bot : Bots)
        if (Bot->GetId() == BotId)
            return Bot.get();

    return nullptr;
}

FBotController* FBotManager::FindBotByController(const AActor* Controller)
{
    if (!Controller)
        return nullptr;

    for (auto& Bot : Bots)
        if ((const AActor*)Bot->GetController() == Controller)
            return Bot.get();

    return nullptr;
}

void FBotManager::RequestSpawnBots(int Count)
{
    if (Count > 0)
        RequestedSpawnCount += Count;
}

void FBotManager::RequestRemoveAllBots()
{
    bRemoveAllRequested = true;
}

void FBotManager::NotifyPawnDied(const AActor* Controller)
{
    if (Bots.empty())
        return;

    if (auto Bot = FindBotByController(Controller))
        Bot->OnPawnDied(GetWorldTime());
}

void FBotManager::ProcessQueuedRequests()
{
    if (bRemoveAllRequested.exchange(false))
        RemoveAllBots();

    auto Requested = RequestedSpawnCount.exchange(0);
    if (Requested > 0)
        SpawnBots(Requested);

    for (int i = 0; i < FBotSystemConfig::MaxSpawnsPerTick && !SpawnQueue.empty(); i++)
    {
        auto Request = SpawnQueue.front();
        SpawnQueue.pop_front();

        if (!SpawnBotInternal(Request) && !CanSpawnMore(1))
        {
            // Nothing else in the queue can succeed either.
            SpawnQueue.clear();
            break;
        }
    }
}

void FBotManager::RunMaintenance(double Now)
{
    auto World = UWorld::GetWorld();
    auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;

    for (auto& Bot : Bots)
    {
        if (!Bot->Validate(Now))
        {
            Bot->MarkPendingRemoval();
            continue;
        }

        // Dead bots stop counting as alive players right away, not only when they are removed. For
        // player controllers the game's death handling usually did this already (idempotent).
        if (Bot->GetLifeState() == EBotLifeState::Dead && Bot->bRegisteredAlive && GameMode)
        {
            if (auto Controller = Bot->GetController())
                BotCompat::UnregisterAlive(GameMode, Controller, Bot->GetControllerKind());
            Bot->bRegisteredAlive = false;
        }

        if (FBotSystemConfig::bRemoveDeadBots && Bot->GetLifeState() == EBotLifeState::Dead && Now - Bot->GetDeathTime() >= FBotSystemConfig::DeadBotCleanupDelay)
            Bot->MarkPendingRemoval();
    }
}

void FBotManager::RunThink(double Now)
{
    for (auto& Bot : Bots)
    {
        if (Bot->GetLifeState() != EBotLifeState::Active || Now < Bot->NextThinkTime)
            continue;

        Bot->NextThinkTime = Now + FBotSystemConfig::ThinkInterval;
        Bot->Think(Now);
    }
}

void FBotManager::RunFrame()
{
    for (auto& Bot : Bots)
        if (Bot->GetLifeState() == EBotLifeState::Active)
            Bot->TickFrame();
}

template <typename Fn>
int FBotManager::ForEachCommandTarget(uint32 BotId, Fn&& Func)
{
    int Count = 0;

    for (auto& Bot : Bots)
    {
        if ((BotId != 0 && Bot->GetId() != BotId) || !Bot->IsAlive())
            continue;

        Func(*Bot);
        Count++;
    }

    return Count;
}

int FBotManager::CommandMoveTo(uint32 BotId, const FVector& Location)
{
    return ForEachCommandTarget(BotId, [&](FBotController& Bot) { Bot.CommandMoveTo(Location); });
}

int FBotManager::CommandFollow(uint32 BotId, AActor* Target)
{
    return ForEachCommandTarget(BotId, [&](FBotController& Bot) { Bot.CommandFollow(Target); });
}

int FBotManager::CommandState(uint32 BotId, EBotAIState State)
{
    return ForEachCommandTarget(BotId, [&](FBotController& Bot) { Bot.SetAIState(State); });
}

int FBotManager::CommandBuild(uint32 BotId, EBotBuildPiece Piece, AActor* Towards)
{
    if (!Towards)
        return 0;

    auto Target = Towards->K2_GetActorLocation();
    int Count = 0;

    ForEachCommandTarget(BotId, [&](FBotController& Bot)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn)
            return;

        auto Location = Pawn->K2_GetActorLocation();
        FVector Direction(Target.X - Location.X, Target.Y - Location.Y, 0.0);
        if (Bot.RequestBuild(Piece, Direction, EBotBuildReason::Manual))
            Count++;
    });

    return Count;
}

int FBotManager::CommandOutfit(uint32 BotId, const std::wstring& OutfitId)
{
    int Count = 0;

    ForEachCommandTarget(BotId, [&](FBotController& Bot)
    {
        auto Loadout = Bot.Cosmetics;
        if (BotCosmetics::ChooseSpecific(Loadout, EBotCosmeticSlot::Outfit, OutfitId) && BotCosmetics::ApplyLoadout(Bot, Loadout))
        {
            Bot.Cosmetics = Loadout;
            Count++;
        }
    });

    return Count;
}

void FBotManager::Tick(const UObject* NetDriver, float DeltaSeconds)
{
    auto World = UWorld::GetWorld();
    if (!World || NetDriver != World->NetDriver)
        return;

    if (Bots.empty() && SpawnQueue.empty() && !bRemoveAllRequested && RequestedSpawnCount == 0)
    {
        if (PublishedBotCount != 0)
        {
            PublishedBotCount = 0;
            BotPerceptionWorld::Reset();
        }
        return;
    }

    auto Start = NowMicros();
    auto Now = GetWorldTime();

    ProcessQueuedRequests();

    bIsTicking = true;

    if (Now >= NextMaintenanceTime)
    {
        NextMaintenanceTime = Now + FBotSystemConfig::MaintenanceInterval;
        RunMaintenance(Now);
    }

    // One shared actor snapshot per interval for all bots' perception.
    if (!Bots.empty())
        BotPerceptionWorld::Update(Now);

    RunThink(Now);
    RunFrame();

    bIsTicking = false;
    FlushPendingRemovals();
    PublishedBotCount = (int)Bots.size();

    RecordTickTime(Now, NowMicros() - Start);
}

void FBotManager::RecordTickTime(double Now, double Micros)
{
    PerfTickCount++;
    PerfTotalMicros += Micros;
    PerfMaxMicros = (std::max)(PerfMaxMicros, Micros);

    if (PerfWindowStart == 0.0)
        PerfWindowStart = Now;

    if (FBotSystemConfig::PerfReportInterval <= 0.f || Now - PerfWindowStart < FBotSystemConfig::PerfReportInterval)
        return;

    LastAverageTickMicros = (float)(PerfTotalMicros / PerfTickCount);

    // Tick time includes spawning; report spawn cost separately so steady-state cost is visible.
    auto SteadyMicros = PerfTickCount > 0 ? (PerfTotalMicros - PerfSpawnMicros) / PerfTickCount : 0.0;

    BOT_LOG(BOT_CATEGORY_PERF, "bots=%d alive=%d ticks=%d avg=%.1fus steady=%.1fus max=%.1fus spawns=%d avgSpawn=%.2fms", (int)Bots.size(), (int)GetActiveBots().size(), PerfTickCount,
            LastAverageTickMicros, SteadyMicros, PerfMaxMicros, PerfSpawnCount, PerfSpawnCount ? PerfSpawnMicros / PerfSpawnCount / 1000.0 : 0.0);

    PerfWindowStart = Now;
    PerfTotalMicros = 0.0;
    PerfMaxMicros = 0.0;
    PerfTickCount = 0;
    PerfSpawnCount = 0;
    PerfSpawnMicros = 0.0;
}
