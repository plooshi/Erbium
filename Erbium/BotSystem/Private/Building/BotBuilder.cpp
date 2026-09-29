#include "pch.h"
#include "../../Public/Building/BotBuilder.h"
#include "../../Public/BotController.h"
#include "../../Public/BotEngineTypes.h"
#include "../../Public/BotLog.h"
#include "../../../Erbium/Public/Configuration.h"
#include "../../../FortniteGame/Public/BuildingSMActor.h"
#include "../../../FortniteGame/Public/FortPlayerControllerAthena.h"

// Engine functions Erbium locates for ServerCreateBuildingActor (FortPlayerControllerAthena.cpp).
// Either may be 0 on a build where the pattern was not found.
extern uint64_t CanAffordToPlaceBuildableClass_;
extern uint64_t PayBuildableClassPlacementCost_;

// Distance from the pawn's location (capsule centre) to its feet. Fortnite player capsules have a
// half height of roughly 90 uu; only used to pick the grid level the bot stands on.
static constexpr double CapsuleHalfHeight = 90.0;

// Zeroed FBuildingClassData sized for the running build, with BuildingClass set.
struct FBuildingClassDataBuffer
{
    alignas(8) uint8 Data[0x40] = {};

    explicit FBuildingClassDataBuffer(const UClass* BuildingClass)
    {
        static auto ClassOffset = FBuildingClassData::StaticStruct()->GetOffset("BuildingClass");
        *(const UClass**)(Data + (ClassOffset != -1 ? ClassOffset : 0)) = BuildingClass;
    }

    FBuildingClassData& Get() { return *(FBuildingClassData*)Data; }
};

static void DestroyIfAlive(AActor* Actor)
{
    if (Actor && !Actor->bActorIsBeingDestroyed)
        Actor->K2_DestroyActor();
}

FBotBuilder::FBotBuilder(FBotController& InBot)
    : Bot(InBot)
{
}

void FBotBuilder::Initialize()
{
    auto& Config = Bot.GetConfig();
    for (int i = 0; i < (int)EBotBuildMaterial::Count; i++)
        VirtualMaterials[i] = Config.StartingMaterials[i];
}

bool FBotBuilder::UsesNativeMaterials() const
{
    auto PlayerController = Bot.GetPlayerController();
    return PlayerController && PlayerController->WorldInventory && CanAffordToPlaceBuildableClass_ && PayBuildableClassPlacementCost_;
}

bool FBotBuilder::CanAfford(EBotBuildMaterial Material, void* BuildingClassData) const
{
    if (FConfiguration::bInfiniteMats)
        return true;

    if (UsesNativeMaterials())
    {
        auto PlayerController = Bot.GetPlayerController();
        if (PlayerController->bBuildFree)
            return true;

        auto CanAffordToPlaceBuildableClass = (bool (*)(AFortPlayerControllerAthena*, FBuildingClassData))CanAffordToPlaceBuildableClass_;
        return CanAffordToPlaceBuildableClass(PlayerController, *(FBuildingClassData*)BuildingClassData);
    }

    return VirtualMaterials[(int)Material] >= Bot.GetConfig().BuildCost;
}

void FBotBuilder::Pay(EBotBuildMaterial Material, void* BuildingClassData)
{
    if (FConfiguration::bInfiniteMats)
        return;

    if (UsesNativeMaterials())
    {
        auto PlayerController = Bot.GetPlayerController();
        if (PlayerController->bBuildFree)
            return;

        auto PayBuildableClassPlacementCost = (int (*)(AFortPlayerControllerAthena*, FBuildingClassData))PayBuildableClassPlacementCost_;
        PayBuildableClassPlacementCost(PlayerController, *(FBuildingClassData*)BuildingClassData);
        return;
    }

    VirtualMaterials[(int)Material] -= Bot.GetConfig().BuildCost;
}

int FBotBuilder::GetMaterials(EBotBuildMaterial Material) const
{
    return VirtualMaterials[(int)Material];
}

void FBotBuilder::AddMaterials(EBotBuildMaterial Material, int Count)
{
    // Same cap as the game's resource stacks.
    VirtualMaterials[(int)Material] = (std::min)(VirtualMaterials[(int)Material] + Count, 999);
}

bool FBotBuilder::IsReady(double Now) const
{
    auto& Config = Bot.GetConfig();
    if (!Config.bCanBuild || Now < NextBuildTime)
        return false;

    int RecentBuilds = 0;
    for (auto Time : RecentBuildTimes)
        if (Now - Time < 60.0)
            RecentBuilds++;

    return RecentBuilds < Config.MaxBuildsPerMinute;
}

void FBotBuilder::RecordBuild(double Now)
{
    RecentBuildTimes.erase(std::remove_if(RecentBuildTimes.begin(), RecentBuildTimes.end(), [Now](double Time) { return Now - Time >= 60.0; }), RecentBuildTimes.end());
    RecentBuildTimes.push_back(Now);
}

EBotBuildResult FBotBuilder::Build(const FBotBuildRequest& Request, double Now)
{
    auto& Config = Bot.GetConfig();

    if (!Config.bCanBuild || !FBotSystemConfig::bAllowBuilding || !BotBuildCatalog::IsAvailable())
        return EBotBuildResult::Disabled;

    if (!IsReady(Now))
        return EBotBuildResult::OnCooldown;

    auto Pawn = Bot.GetPawn();
    auto Controller = Bot.GetController();
    if (!Pawn || !Controller)
        return EBotBuildResult::Disabled;

    // 1. Select piece class + material (Config.MaterialOrder, first affordable one wins).
    const UClass* BuildingClass = nullptr;
    EBotBuildMaterial Material = EBotBuildMaterial::Wood;
    bool bAnyClass = false;

    for (auto Candidate : Config.MaterialOrder)
    {
        auto Class = BotBuildCatalog::GetBuildingClass(Request.Piece, Candidate);
        if (!Class)
            continue;

        bAnyClass = true;
        FBuildingClassDataBuffer Data(Class);
        if (CanAfford(Candidate, &Data.Get()))
        {
            BuildingClass = Class;
            Material = Candidate;
            break;
        }
    }

    if (!BuildingClass)
    {
        // Short back-off so an unaffordable/unsupported piece is not retried every decision.
        NextBuildTime = Now + Config.BuildCooldown;
        return bAnyClass ? EBotBuildResult::CannotAfford : EBotBuildResult::NoPiece;
    }

    // 2. Locations.
    auto Location = Pawn->K2_GetActorLocation();
    FVector Feet(Location.X, Location.Y, Location.Z - CapsuleHalfHeight);
    auto Candidates = BotBuildGrid::GetCandidates(Request.Piece, Feet, Request.Direction, Config.RampYawOffset);

    FBuildingClassDataBuffer Data(BuildingClass);
    auto PlayerController = Bot.GetPlayerController(); // nullptr for AI-controller bots
    auto PlayerState = Bot.GetPlayerState();

    // 3. Validate + place, candidate by candidate.
    for (size_t Index = 0; Index < Candidates.size(); Index++)
    {
        bool bRetriedAfterCalibration = false;

        while (true)
        {
            auto& Placement = Candidates[Index];
            auto Building = (AActor*)AFortPlayerControllerAthena::PlaceBuildingActor(Controller, PlayerController, PlayerState, TSubclassOf<AActor>(BuildingClass), Data.Get(), FVector(Placement.Location),
                                                                                     FRotator(Placement.Rotation), false);

            // Rejected by the game's validation (occupied, invalid grid position, ...).
            if (!Building || Building->bActorIsBeingDestroyed)
                break;

            // 4. Verify where it actually ended up. A misplaced piece is replaced once with the
            // corrected calibration (only the first pieces of each type should ever need this).
            if (!BotBuildGrid::VerifyAndCalibrate(Request.Piece, Building, Placement) && !bRetriedAfterCalibration)
            {
                DestroyIfAlive(Building);
                bRetriedAfterCalibration = true;
                Candidates = BotBuildGrid::GetCandidates(Request.Piece, Feet, Request.Direction, Config.RampYawOffset);
                continue;
            }

            // 5. Pay and record.
            Pay(Material, &Data.Get());
            RecordBuild(Now);
            NextBuildTime = Now + Config.BuildCooldown;
            LastBuilding = TWeakObjectPtr<AActor>(Building);
            PlacedCount++;

            BOT_VERBOSE(BOT_CATEGORY_BUILDING, "Bot #%u placed %s %s (candidate %d).", Bot.GetId(), BotBuildCatalog::ToString(Material), BotBuildCatalog::ToString(Request.Piece), (int)Index);
            return EBotBuildResult::Placed;
        }
    }

    // Recovery: nothing fit here. Back off; the next decision may be from a different position.
    FailedCount++;
    NextBuildTime = Now + Config.BuildCooldown;
    BOT_VERBOSE(BOT_CATEGORY_BUILDING, "Bot #%u could not place a %s (%d candidates rejected).", Bot.GetId(), BotBuildCatalog::ToString(Request.Piece), (int)Candidates.size());
    return EBotBuildResult::NoValidLocation;
}

const char* FBotBuilder::ToString(EBotBuildResult Result)
{
    switch (Result)
    {
    case EBotBuildResult::Placed:
        return "Placed";
    case EBotBuildResult::Disabled:
        return "Disabled";
    case EBotBuildResult::OnCooldown:
        return "OnCooldown";
    case EBotBuildResult::NoPiece:
        return "NoPiece";
    case EBotBuildResult::CannotAfford:
        return "CannotAfford";
    case EBotBuildResult::NoValidLocation:
        return "NoValidLocation";
    }
    return "Unknown";
}
