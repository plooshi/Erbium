#pragma once
#include "../../../pch.h"
#include "../BotState.h"
#include "BotBuildCatalog.h"
#include "BotBuildGrid.h"

class FBotController;

enum class EBotBuildReason : uint8
{
    Defensive, // taking damage from a visible enemy: wall between bot and enemy
    Obstacle,  // movement got stuck: ramp over the obstacle
    Manual,    // commanded (cheat)
};

enum class EBotBuildResult : uint8
{
    Placed,
    Disabled,        // building turned off for this bot / no build catalogue on this build
    OnCooldown,      // too soon after the last build (or too many builds this minute)
    NoPiece,         // no buildable class for this piece in any material
    CannotAfford,    // not enough materials in any material that has the piece
    NoValidLocation, // every candidate was rejected by the game's placement validation
};

struct FBotBuildRequest
{
    EBotBuildPiece Piece = EBotBuildPiece::Wall;
    FVector Direction{};
    EBotBuildReason Reason = EBotBuildReason::Manual;

    // After the build, continue towards this location (obstacle ramps).
    bool bHasResumeGoal = false;
    FVector ResumeGoal{};

    // State to return to after the Building behavior finishes.
    EBotAIState ReturnState = EBotAIState::Searching;
};

// Per-bot building (Phase 4).
//
// Build() runs the whole pipeline synchronously and never assumes success:
//   select piece class + material -> check materials -> grid candidates -> for each candidate:
//   game placement validation (native CantBuild via AFortPlayerControllerAthena::PlaceBuildingActor,
//   the same code path as a player's build) -> verify the spawned actor exists -> measure it
//   (BotBuildGrid calibration; a misplaced piece is removed and placed again once) -> pay.
//
// Materials:
//  - player-controller bots with an inventory use the game's own CanAfford/Pay functions, exactly
//    like ServerCreateBuildingActor (and respect bInfiniteMats / bBuildFree);
//  - AI-controller bots have no inventory yet (Phase 2B), so they use a per-bot material budget from
//    FBotConfig::StartingMaterials. This is a stand-in until they carry real materials.
class FBotBuilder
{
public:
    explicit FBotBuilder(FBotController& InBot);

    void Initialize();

    bool IsReady(double Now) const;
    EBotBuildResult Build(const FBotBuildRequest& Request, double Now);

    int GetMaterials(EBotBuildMaterial Material) const;

    // Materials picked up by bots without a native inventory (Phase 2B looting).
    void AddMaterials(EBotBuildMaterial Material, int Count);
    int GetPlacedCount() const { return PlacedCount; }
    int GetFailedCount() const { return FailedCount; }
    AActor* GetLastBuilding() const { return (AActor*)LastBuilding.Get(); }

    // Set by the build decision, consumed by the Building behavior.
    bool bHasPending = false;
    FBotBuildRequest Pending;

    static const char* ToString(EBotBuildResult Result);

private:
    bool UsesNativeMaterials() const;
    bool CanAfford(EBotBuildMaterial Material, void* BuildingClassData) const;
    void Pay(EBotBuildMaterial Material, void* BuildingClassData);
    void RecordBuild(double Now);

    FBotController& Bot;

    int VirtualMaterials[(int)EBotBuildMaterial::Count] = {};
    std::vector<double> RecentBuildTimes;
    double NextBuildTime = 0.0;

    int PlacedCount = 0;
    int FailedCount = 0;
    TWeakObjectPtr<AActor> LastBuilding;
};
