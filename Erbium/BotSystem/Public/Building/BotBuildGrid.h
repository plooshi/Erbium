#pragma once
#include "../../../pch.h"
#include "BotBuildCatalog.h"

// Where to put a piece.
struct FBotBuildPlacement
{
    FVector Location{};       // actor location passed to the placement (pivot)
    FRotator Rotation{};
    FVector IntendedCenter{}; // where the piece's geometry should end up (used for verification)
    int MajorAxis = 0;        // 0 = X, 1 = Y: axis of the build direction
};

// Structural grid geometry + runtime calibration.
//
// Known: Fortnite's structural grid is 512 x 512 uu per cell and 384 uu per level. Not known here:
// where each piece class has its pivot and which way a wall faces at yaw 0. Rather than hard-coding
// guesses, the first successfully placed piece of each type is measured with AActor::GetActorBounds:
//  - the offset between pivot and geometry centre is learned in the piece's local space, so later
//    placements put the geometry exactly on the intended grid position;
//  - for walls, the thin axis of the bounds tells whether yaw 0 faces along X or Y.
// A piece found misplaced is reported so the builder can replace it once with the corrected values.
// Ramps are symmetric in their bounds, so their facing cannot be measured this way; see
// FBotConfig::RampYawOffset.
namespace BotBuildGrid
{
    constexpr double CellSize = 512.0;
    constexpr double LevelHeight = 384.0;

    // Candidate placements, best first. Feet = bottom of the bot's capsule, Direction = where to build
    // (towards a threat for walls, direction of travel for ramps/floors). Candidates cover the bot's
    // own cell and the next cell in Direction.
    std::vector<FBotBuildPlacement> GetCandidates(EBotBuildPiece Piece, const FVector& Feet, const FVector& Direction, float RampYawOffset);

    // Measures a placed piece. Returns false if it ended up off target (wrong position or, for walls,
    // wrong orientation); calibration has been updated so the next GetCandidates call is corrected.
    bool VerifyAndCalibrate(EBotBuildPiece Piece, AActor* Building, const FBotBuildPlacement& Placement);

    bool IsCalibrated(EBotBuildPiece Piece);
    void ResetCalibration();
}
