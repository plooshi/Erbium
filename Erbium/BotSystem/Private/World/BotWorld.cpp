#include "pch.h"
#include "../../Public/World/BotWorld.h"
#include "../../Public/BotConfig.h"
#include "../../Public/BotLog.h"
#include "../../Public/Navigation/BotNavigation.h"
#include "../../../FortniteGame/Public/BattleRoyaleGamePhaseLogic.h"
#include "../../../FortniteGame/Public/BuildingContainer.h"

namespace
{
    EBotMatchPhase Phase = EBotMatchPhase::Unknown;
    FBotSafeZone SafeZone;
    TWeakObjectPtr<AActor> Aircraft;
    std::vector<BotWorld::FContainer> Containers;
    double NextContainerSnapshotTime = 0.0;
}

static UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GetPhaseLogic()
{
    if (VersionInfo.FortniteVersion < 25.20)
        return nullptr;

    return UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
}

static EBotMatchPhase ReadPhase(AFortGameStateAthena* GameState)
{
    uint8 Raw = 0;

    if (auto Logic = GetPhaseLogic())
    {
        static auto Offset = Logic->GetOffset("GamePhase");
        if (Offset == -1)
            return EBotMatchPhase::Unknown;
        Raw = *(uint8*)(__int64(Logic) + Offset);
    }
    else if (GameState->HasGamePhase())
        Raw = GameState->GamePhase;
    else
        return EBotMatchPhase::Unknown;

    switch ((EAthenaGamePhase)Raw)
    {
    case EAthenaGamePhase::Warmup:
        return EBotMatchPhase::Warmup;
    case EAthenaGamePhase::Aircraft:
        return EBotMatchPhase::Aircraft;
    case EAthenaGamePhase::SafeZones:
        return EBotMatchPhase::SafeZones;
    case EAthenaGamePhase::EndGame:
        return EBotMatchPhase::EndGame;
    default:
        return EBotMatchPhase::Unknown;
    }
}

static AActor* ReadAircraft(AFortGameStateAthena* GameState)
{
    // Same lookup order as Erbium's late-game code.
    if (GameState->HasAircrafts() && GameState->Aircrafts.Num() > 0)
        return GameState->Aircrafts[0];
    if (GameState->HasAircraft() && GameState->Aircraft)
        return GameState->Aircraft;

    if (auto Logic = GetPhaseLogic())
        if (Logic->HasAircrafts_GameState() && Logic->Aircrafts_GameState.Num() > 0)
            return Logic->Aircrafts_GameState[0].Get();

    return nullptr;
}

static AFortSafeZoneIndicator* ReadSafeZoneIndicator(AFortGameMode* GameMode)
{
    if (GameMode->HasSafeZoneIndicator())
        return GameMode->SafeZoneIndicator;

    if (auto Logic = GetPhaseLogic())
        return Logic->SafeZoneIndicator;

    return nullptr;
}

static void ReadSafeZone(AFortGameMode* GameMode, double Now)
{
    SafeZone = FBotSafeZone{};

    auto Indicator = ReadSafeZoneIndicator(GameMode);
    if (!Indicator || Indicator->bActorIsBeingDestroyed || !Indicator->HasNextRadius())
        return;

    SafeZone.NextCenter = Indicator->NextCenter;
    SafeZone.NextRadius = Indicator->NextRadius;
    if (SafeZone.NextRadius <= 0.f)
        return;

    FVector PreviousCenter = Indicator->HasPreviousCenter() ? Indicator->PreviousCenter : (Indicator->HasLastCenter() ? Indicator->LastCenter : FVector(SafeZone.NextCenter));
    float PreviousRadius = Indicator->HasPreviousRadius() ? Indicator->PreviousRadius : (Indicator->HasLastRadius() ? Indicator->LastRadius : 0.f);

    auto Start = (double)Indicator->SafeZoneStartShrinkTime;
    auto Finish = (double)Indicator->SafeZoneFinishShrinkTime;

    // Before the first circle forms there is no previous one: nobody is in the storm yet.
    if (PreviousRadius <= 0.f)
    {
        SafeZone.CurrentCenter = FVector(SafeZone.NextCenter);
        SafeZone.CurrentRadius = 1.0e8f;
    }
    else if (Now <= Start || Finish <= Start)
    {
        SafeZone.CurrentCenter = PreviousCenter;
        SafeZone.CurrentRadius = PreviousRadius;
    }
    else
    {
        auto Alpha = (std::min)((Now - Start) / (Finish - Start), 1.0);
        SafeZone.CurrentCenter = FVector(PreviousCenter.X + (SafeZone.NextCenter.X - PreviousCenter.X) * Alpha, PreviousCenter.Y + (SafeZone.NextCenter.Y - PreviousCenter.Y) * Alpha,
                                         PreviousCenter.Z + (SafeZone.NextCenter.Z - PreviousCenter.Z) * Alpha);
        SafeZone.CurrentRadius = (float)(PreviousRadius + (SafeZone.NextRadius - PreviousRadius) * Alpha);
        SafeZone.bShrinking = Alpha < 1.0;
    }

    SafeZone.SecondsUntilShrinkStart = (float)(std::max)(Start - Now, 0.0);
    SafeZone.bActive = true;
}

static std::string ToLower(std::string Value)
{
    std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char C) { return (char)tolower(C); });
    return Value;
}

static void SnapshotContainers(double Now)
{
    if (Now < NextContainerSnapshotTime)
        return;

    NextContainerSnapshotTime = Now + FBotSystemConfig::ContainerSnapshotInterval;
    Containers.clear();

    TArray<ABuildingContainer*> All;
    Utils::GetAll<ABuildingContainer>(All);

    for (auto& Container : All)
    {
        if (!Container || Container->bActorIsBeingDestroyed || Container->bAlreadySearched)
            continue;

        // Only chests and ammo boxes; other searchables (trash cans, fridges, ...) are not worth it.
        auto TierGroup = ToLower(Container->SearchLootTierGroup.ToString().c_str());
        bool bIsChest = TierGroup.find("treasure") != std::string::npos;
        bool bIsAmmo = TierGroup.find("ammo") != std::string::npos;
        if (!bIsChest && !bIsAmmo)
            continue;

        BotWorld::FContainer Entry;
        Entry.Container = TWeakObjectPtr<ABuildingContainer>(Container);
        Entry.Location = Container->K2_GetActorLocation();
        Entry.bIsChest = bIsChest;
        Containers.push_back(Entry);
    }

    All.Free();
}

namespace BotWorld
{
    void Update(double Now)
    {
        auto World = UWorld::GetWorld();
        auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;
        if (!GameMode || !GameMode->GameState)
            return;

        auto NewPhase = ReadPhase(GameMode->GameState);
        if (NewPhase != Phase)
        {
            BOT_LOG(BOT_CATEGORY_SYSTEM, "Match phase: %s -> %s", ToString(Phase), ToString(NewPhase));
            Phase = NewPhase;
        }

        Aircraft = TWeakObjectPtr<AActor>(ReadAircraft(GameMode->GameState));
        ReadSafeZone(GameMode, Now);
        SnapshotContainers(Now);
    }

    void Reset()
    {
        Phase = EBotMatchPhase::Unknown;
        SafeZone = FBotSafeZone{};
        Containers.clear();
        NextContainerSnapshotTime = 0.0;
    }

    EBotMatchPhase GetPhase()
    {
        return Phase;
    }

    const char* ToString(EBotMatchPhase Value)
    {
        switch (Value)
        {
        case EBotMatchPhase::Warmup:
            return "Warmup";
        case EBotMatchPhase::Aircraft:
            return "Aircraft";
        case EBotMatchPhase::SafeZones:
            return "SafeZones";
        case EBotMatchPhase::EndGame:
            return "EndGame";
        default:
            return "Unknown";
        }
    }

    const FBotSafeZone& GetSafeZone()
    {
        return SafeZone;
    }

    static bool InsideCircle(const FVector& Location, const FVector& Center, float Radius, float MarginFraction)
    {
        auto DX = Location.X - Center.X, DY = Location.Y - Center.Y;
        return sqrt(DX * DX + DY * DY) <= Radius * (1.f - MarginFraction);
    }

    bool IsInsideCurrentZone(const FVector& Location, float MarginFraction)
    {
        return !SafeZone.bActive || InsideCircle(Location, SafeZone.CurrentCenter, SafeZone.CurrentRadius, MarginFraction);
    }

    bool IsInsideNextZone(const FVector& Location, float MarginFraction)
    {
        return !SafeZone.bActive || InsideCircle(Location, SafeZone.NextCenter, SafeZone.NextRadius, MarginFraction);
    }

    FVector GetRotationPoint(const FVector& From)
    {
        // Head for the nearest side of the next circle, a good way inside it.
        double DX = From.X - SafeZone.NextCenter.X, DY = From.Y - SafeZone.NextCenter.Y;
        double Length = sqrt(DX * DX + DY * DY);
        double Inside = SafeZone.NextRadius * 0.55;

        FVector Target = Length > 1.0 ? FVector(SafeZone.NextCenter.X + DX / Length * Inside, SafeZone.NextCenter.Y + DY / Length * Inside, From.Z) : FVector(SafeZone.NextCenter.X, SafeZone.NextCenter.Y, From.Z);

        return BotNavigation::GetRandomPointInRadius(Target, (float)(std::min)(Inside * 0.5, 3000.0));
    }

    AActor* GetAircraft()
    {
        auto Actor = Aircraft.Get();
        return Actor && !Actor->bActorIsBeingDestroyed ? Actor : nullptr;
    }

    bool GetDropWindow(double& OutStart, double& OutEnd)
    {
        auto Bus = (AFortAthenaAircraft*)GetAircraft();
        if (!Bus)
            return false;

        if (Bus->HasDropStartTime() && Bus->HasDropEndTime() && Bus->DropEndTime > Bus->DropStartTime)
        {
            OutStart = Bus->DropStartTime;
            OutEnd = Bus->DropEndTime;
            return true;
        }

        if (Bus->HasFlightStartTime() && Bus->HasFlightInfo() && FAircraftFlightInfo::HasTimeTillDropStart())
        {
            OutStart = Bus->FlightStartTime + Bus->FlightInfo.TimeTillDropStart;
            OutEnd = Bus->FlightStartTime + Bus->FlightInfo.TimeTillDropEnd;
            return OutEnd > OutStart;
        }

        return false;
    }

    std::vector<FContainer>& GetContainers()
    {
        return Containers;
    }

    void UpdatePawnStormState(AFortPlayerPawnAthena* Pawn)
    {
        auto Logic = GetPhaseLogic();
        if (!Logic || !Pawn || Phase != EBotMatchPhase::SafeZones)
            return;

        auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
        auto Location = Pawn->K2_GetActorLocation();

        // Same check and flag update as UFortGameStateComponent_BattleRoyaleGamePhaseLogic's player loop.
        bool bInZone = Logic->IsInCurrentSafeZone(Location, false);
        if (!UFortGameStateComponent_BattleRoyaleGamePhaseLogic::IsInCurrentSafeZone__Ptr)
            bInZone = GameMode->IsInCurrentSafeZone(Location, false);

        if (Pawn->bIsInsideSafeZone != bInZone || Pawn->bIsInAnyStorm != !bInZone)
        {
            Pawn->bIsInAnyStorm = !bInZone;
            Pawn->OnRep_IsInAnyStorm();
            Pawn->bIsInsideSafeZone = bInZone;
            Pawn->OnRep_IsInsideSafeZone();
        }
    }
}
