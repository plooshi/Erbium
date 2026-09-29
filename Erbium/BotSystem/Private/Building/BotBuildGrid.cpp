#include "pch.h"
#include "../../Public/Building/BotBuildGrid.h"
#include "../../Public/BotLog.h"

// A piece whose geometry centre is further than this from where it was meant to be is misplaced.
static constexpr double PositionTolerance = 96.0;

namespace
{
    struct FPieceCalibration
    {
        bool bCalibrated = false;
        double LocalOffsetX = 0.0; // geometry centre relative to pivot, in the piece's local (unrotated) space
        double LocalOffsetY = 0.0;
        double LocalOffsetZ = 0.0;
        float YawOffset = 0.f;     // walls: 0 or 90 so that the wall's thin side faces the build direction
    };

    FPieceCalibration Calibration[(int)EBotBuildPiece::Count];
}

static constexpr double DegToRad = 3.14159265358979323846 / 180.0;

static void RotateYaw(double X, double Y, double YawDegrees, double& OutX, double& OutY)
{
    auto Cos = cos(YawDegrees * DegToRad), Sin = sin(YawDegrees * DegToRad);
    OutX = X * Cos - Y * Sin;
    OutY = X * Sin + Y * Cos;
}

// Builds a placement whose geometry centre lands on Center.
static FBotBuildPlacement MakePlacement(EBotBuildPiece Piece, const FVector& Center, float BaseYaw, int MajorAxis)
{
    auto& Cal = Calibration[(int)Piece];

    FBotBuildPlacement Placement;
    Placement.IntendedCenter = FVector(Center);
    Placement.MajorAxis = MajorAxis;

    auto Yaw = BaseYaw + Cal.YawOffset;
    Placement.Rotation.Pitch = 0.0;
    Placement.Rotation.Yaw = Yaw;
    Placement.Rotation.Roll = 0.0;

    // Pivot = centre - rotated(local offset). Before calibration the offset is zero (centre pivot).
    double WorldOffsetX, WorldOffsetY;
    RotateYaw(Cal.LocalOffsetX, Cal.LocalOffsetY, Yaw, WorldOffsetX, WorldOffsetY);
    Placement.Location = FVector(Center.X - WorldOffsetX, Center.Y - WorldOffsetY, Center.Z - Cal.LocalOffsetZ);

    return Placement;
}

namespace BotBuildGrid
{
    std::vector<FBotBuildPlacement> GetCandidates(EBotBuildPiece Piece, const FVector& Feet, const FVector& Direction, float RampYawOffset)
    {
        // Major axis and sign of the build direction; pieces are axis aligned on the grid.
        int MajorAxis = fabs(Direction.X) >= fabs(Direction.Y) ? 0 : 1;
        double Sign = (MajorAxis == 0 ? Direction.X : Direction.Y) >= 0.0 ? 1.0 : -1.0;
        float BaseYaw = MajorAxis == 0 ? (Sign > 0 ? 0.f : 180.f) : (Sign > 0 ? 90.f : 270.f);

        // Cell the bot stands in and the level its feet are on.
        auto CellX = floor(Feet.X / CellSize) * CellSize;
        auto CellY = floor(Feet.Y / CellSize) * CellSize;
        auto Level = floor((Feet.Z + 16.0) / LevelHeight) * LevelHeight;

        // Next cell in the build direction.
        auto NextX = CellX + (MajorAxis == 0 ? Sign * CellSize : 0.0);
        auto NextY = CellY + (MajorAxis == 1 ? Sign * CellSize : 0.0);

        auto Half = CellSize / 2.0;
        std::vector<FBotBuildPlacement> Candidates;

        switch (Piece)
        {
        case EBotBuildPiece::Wall:
        {
            // Edge of the current cell facing the direction, then the far edge of the next cell.
            auto EdgeOffset = Sign > 0 ? CellSize : 0.0;
            FVector Near = MajorAxis == 0 ? FVector(CellX + EdgeOffset, CellY + Half, Level + LevelHeight / 2.0) : FVector(CellX + Half, CellY + EdgeOffset, Level + LevelHeight / 2.0);
            FVector Far = MajorAxis == 0 ? FVector(NextX + EdgeOffset, NextY + Half, Level + LevelHeight / 2.0) : FVector(NextX + Half, NextY + EdgeOffset, Level + LevelHeight / 2.0);

            Candidates.push_back(MakePlacement(Piece, Near, BaseYaw, MajorAxis));
            Candidates.push_back(MakePlacement(Piece, Far, BaseYaw, MajorAxis));
            break;
        }
        case EBotBuildPiece::Ramp:
        {
            // Rising in the build direction, in the next cell; one level higher as a second option
            // (e.g. the next cell's ground is already a level up).
            FVector Center(NextX + Half, NextY + Half, Level + LevelHeight / 2.0);
            FVector Upper(NextX + Half, NextY + Half, Level + LevelHeight * 1.5);
            Candidates.push_back(MakePlacement(Piece, Center, BaseYaw + RampYawOffset, MajorAxis));
            Candidates.push_back(MakePlacement(Piece, Upper, BaseYaw + RampYawOffset, MajorAxis));
            break;
        }
        case EBotBuildPiece::Floor:
        {
            FVector Next(NextX + Half, NextY + Half, Level);
            FVector Current(CellX + Half, CellY + Half, Level + LevelHeight);
            Candidates.push_back(MakePlacement(Piece, Next, BaseYaw, MajorAxis));
            Candidates.push_back(MakePlacement(Piece, Current, BaseYaw, MajorAxis));
            break;
        }
        case EBotBuildPiece::Cone:
        {
            FVector Top(CellX + Half, CellY + Half, Level + LevelHeight);
            Candidates.push_back(MakePlacement(Piece, Top, BaseYaw, MajorAxis));
            break;
        }
        default:
            break;
        }

        return Candidates;
    }

    bool VerifyAndCalibrate(EBotBuildPiece Piece, AActor* Building, const FBotBuildPlacement& Placement)
    {
        static auto GetActorBoundsFunc = Building->GetFunction("GetActorBounds");
        if (!GetActorBoundsFunc)
            return true; // cannot measure on this build; accept the placement

        FVector Origin{}, Extent{};
        bool bOnlyColliding = false, bIncludeChildActors = false;
        Building->Call<void>(GetActorBoundsFunc, bOnlyColliding, &Origin, &Extent, bIncludeChildActors);

        if (Extent.X <= 0.0 && Extent.Y <= 0.0)
            return true; // no geometry registered yet; nothing to compare

        auto& Cal = Calibration[(int)Piece];
        auto Pivot = Building->K2_GetActorLocation();
        bool bMisplaced = false;

        if (!Cal.bCalibrated)
        {
            // Learn the geometry centre relative to the pivot, in local space.
            double LocalX, LocalY;
            RotateYaw(Origin.X - Pivot.X, Origin.Y - Pivot.Y, -Placement.Rotation.Yaw, LocalX, LocalY);
            Cal.LocalOffsetX = LocalX;
            Cal.LocalOffsetY = LocalY;
            Cal.LocalOffsetZ = Origin.Z - Pivot.Z;
            Cal.bCalibrated = true;

            BOT_VERBOSE(BOT_CATEGORY_BUILDING, "Calibrated %s: pivot offset (%.0f, %.0f, %.0f).", BotBuildCatalog::ToString(Piece), LocalX, LocalY, Cal.LocalOffsetZ);
        }

        // Walls must be thin along the build direction.
        if (Piece == EBotBuildPiece::Wall)
        {
            auto AlongDirection = Placement.MajorAxis == 0 ? Extent.X : Extent.Y;
            auto Across = Placement.MajorAxis == 0 ? Extent.Y : Extent.X;

            if (AlongDirection > Across)
            {
                Cal.YawOffset = Cal.YawOffset == 0.f ? 90.f : 0.f;
                // The pivot offset was learned with the wrong yaw; measure again next time.
                Cal.bCalibrated = false;
                bMisplaced = true;
                BOT_VERBOSE(BOT_CATEGORY_BUILDING, "Wall faced the wrong way; wall yaw offset is now %.0f.", Cal.YawOffset);
            }
        }

        auto DX = Origin.X - Placement.IntendedCenter.X;
        auto DY = Origin.Y - Placement.IntendedCenter.Y;
        if (sqrt(DX * DX + DY * DY) > PositionTolerance)
            bMisplaced = true;

        return !bMisplaced;
    }

    bool IsCalibrated(EBotBuildPiece Piece)
    {
        return Calibration[(int)Piece].bCalibrated;
    }

    void ResetCalibration()
    {
        for (auto& Cal : Calibration)
            Cal = FPieceCalibration{};
    }
}
