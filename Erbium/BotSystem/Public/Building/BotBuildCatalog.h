#pragma once
#include "../../../pch.h"

// Phase 4: building. Piece/material vocabulary + which engine class builds each combination.

enum class EBotBuildPiece : uint8
{
    Wall,
    Floor,
    Ramp,
    Cone,
    Count,
};

enum class EBotBuildMaterial : uint8
{
    Wood,
    Stone,
    Metal,
    Count,
};

// Resolves the player-buildable class for a piece + material on the running build.
//
// No class paths are hard-coded: the catalogue is built from GameState->AllPlayerBuildableClasses
// (the list the server itself validates player builds against), classified by each class default
// object's BuildingType (EFortBuildingType, values looked up by name) and ResourceType. When several
// classes share a piece type (edit variants), the one whose name matches the standard piece
// ("Solid", "Floor", "StairW", "RoofC") is preferred, otherwise the first one found.
namespace BotBuildCatalog
{
    // nullptr when the build/playlist has no such buildable (logged once when the catalogue is built).
    const UClass* GetBuildingClass(EBotBuildPiece Piece, EBotBuildMaterial Material);

    bool IsAvailable();

    // Clears the catalogue (e.g. after a playlist change).
    void Reset();

    const char* ToString(EBotBuildPiece Piece);
    const char* ToString(EBotBuildMaterial Material);
}
