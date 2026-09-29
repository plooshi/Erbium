#include "pch.h"
#include "../../Public/Perception/BotPerception.h"
#include "../../Public/BotCompat.h"
#include "../../Public/BotController.h"
#include "../../Public/BotEngineTypes.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortPlayerStateAthena.h"
#include "../../../FortniteGame/Public/FortPlayerPawnAthena.h"

namespace BotPerceptionWorld
{
    static std::vector<FEntry> Entries;
    static double NextSnapshotTime = 0.0;

    void Update(double Now)
    {
        if (Now < NextSnapshotTime)
            return;

        NextSnapshotTime = Now + FBotSystemConfig::PerceptionSnapshotInterval;
        Entries.clear();

        TArray<AFortPlayerPawnAthena*> AllPawns;
        Utils::GetAll<AFortPlayerPawnAthena>(AllPawns);

        for (auto& Pawn : AllPawns)
        {
            if (BotCompat::IsPawnDead(Pawn))
                continue;

            FEntry Entry;
            Entry.Pawn = TWeakObjectPtr<AFortPlayerPawnAthena>(Pawn);
            Entry.Location = Pawn->K2_GetActorLocation();

            if (auto PlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState)
            {
                Entry.TeamIndex = PlayerState->TeamIndex;
                Entry.bIsBot = PlayerState->HasbIsABot() && PlayerState->bIsABot;
            }

            Entries.push_back(Entry);
        }

        AllPawns.Free();
    }

    const std::vector<FEntry>& GetPawns()
    {
        return Entries;
    }

    void Reset()
    {
        Entries.clear();
        NextSnapshotTime = 0.0;
    }
}

// AController::LineOfSightTo is a reflected engine function. If a build does not expose it, bots fall
// back to "visible when close" (a quarter of the detection radius) instead of silently never seeing
// anyone; this is logged once so it is not mistaken for a behavior bug.
static bool HasLineOfSight(AController* Controller, AFortPlayerPawnAthena* Target, float Distance, float DetectionRadius)
{
    static auto LineOfSightFunc = Controller->GetFunction("LineOfSightTo");

    if (!LineOfSightFunc)
    {
        static bool bLogged = false;
        if (!bLogged)
        {
            bLogged = true;
            BOT_LOG(BOT_CATEGORY_PERCEPTION, "LineOfSightTo is not available on this build; using proximity-only visibility.");
        }
        return Distance <= DetectionRadius * 0.25f;
    }

    FVector ViewPoint{}; // zero = the controller's own eye position
    bool bAlternateChecks = false;
    return Controller->LineOfSightTo((AActor*)Target, ViewPoint, bAlternateChecks);
}

FBotPerception::FBotPerception(FBotController& InBot)
    : Bot(InBot)
{
}

void FBotPerception::Update(double Now)
{
    if (Now < NextUpdateTime)
        return;

    auto& Config = Bot.GetConfig();
    NextUpdateTime = Now + Config.PerceptionInterval;

    auto Self = Bot.GetPawn();
    auto Controller = Bot.GetController();
    if (!Self || !Controller)
    {
        Pawns.clear();
        return;
    }

    auto SelfLocation = Self->K2_GetActorLocation();
    auto SelfPlayerState = Bot.GetPlayerState();
    auto SelfTeam = SelfPlayerState ? SelfPlayerState->TeamIndex : (uint8)0;
    double RadiusSquared = (double)Config.DetectionRadius * Config.DetectionRadius;

    std::vector<FBotPerceivedPawn> NewPawns;

    // Distance culling against the shared snapshot: no traces, no engine calls.
    for (auto& Entry : BotPerceptionWorld::GetPawns())
    {
        auto DX = Entry.Location.X - SelfLocation.X;
        auto DY = Entry.Location.Y - SelfLocation.Y;
        auto DZ = Entry.Location.Z - SelfLocation.Z;
        auto DistanceSquared = DX * DX + DY * DY + DZ * DZ;

        if (DistanceSquared > RadiusSquared)
            continue;

        auto Pawn = Entry.Pawn.Get();
        if (!Pawn || Pawn == Self)
            continue;

        FBotPerceivedPawn Perceived;
        Perceived.Pawn = Entry.Pawn;
        Perceived.Location = FVector(Entry.Location);
        Perceived.Distance = (float)sqrt(DistanceSquared);
        Perceived.TeamIndex = Entry.TeamIndex;
        Perceived.bIsBot = Entry.bIsBot;
        Perceived.bIsEnemy = Entry.TeamIndex != SelfTeam;
        NewPawns.push_back(Perceived);
    }

    std::sort(NewPawns.begin(), NewPawns.end(), [](const FBotPerceivedPawn& A, const FBotPerceivedPawn& B) { return A.Distance < B.Distance; });

    // Line of sight only for the nearest few. Carry over when each pawn was last seen.
    int Traces = 0;
    for (auto& Perceived : NewPawns)
    {
        for (auto& Previous : Pawns)
            if (Previous.Pawn == Perceived.Pawn)
            {
                Perceived.LastSeenTime = Previous.LastSeenTime;
                break;
            }

        if (Traces >= Config.MaxLineOfSightChecks)
            continue;

        auto Pawn = Perceived.Pawn.Get();
        if (!Pawn)
            continue;

        Traces++;
        Perceived.bVisible = HasLineOfSight(Controller, Pawn, Perceived.Distance, Config.DetectionRadius);

        if (Perceived.bVisible)
            Perceived.LastSeenTime = Now;
    }

    Pawns = std::move(NewPawns);
}

const FBotPerceivedPawn* FBotPerception::GetNearestVisibleEnemy() const
{
    for (auto& Perceived : Pawns)
        if (Perceived.bIsEnemy && Perceived.bVisible)
            return &Perceived;

    return nullptr;
}

const FBotPerceivedPawn* FBotPerception::GetNearestHuman(bool bVisibleOnly) const
{
    for (auto& Perceived : Pawns)
        if (!Perceived.bIsBot && (!bVisibleOnly || Perceived.bVisible))
            return &Perceived;

    return nullptr;
}
