#include "pch.h"
#include "../../Public/Building/BotBuildCatalog.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/BuildingSMActor.h"
#include "../../../FortniteGame/Public/FortGameStateAthena.h"

namespace
{
    bool bBuilt = false;
    bool bAvailable = false;
    const UClass* Classes[(int)EBotBuildPiece::Count][(int)EBotBuildMaterial::Count] = {};
}

// EFortBuildingType member per piece, and the class-name hint for the standard (unedited) piece.
static const char* BuildingTypeNames[(int)EBotBuildPiece::Count] = { "Wall", "Floor", "Stairs", "Roof" };
static const char* PreferredNameHints[(int)EBotBuildPiece::Count] = { "solid", "floor", "stairw", "roofc" };

static std::string ToLower(std::string Value)
{
    std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char C) { return (char)tolower(C); });
    return Value;
}

static void BuildCatalog()
{
    bBuilt = true;

    auto World = UWorld::GetWorld();
    auto GameState = World ? (AFortGameStateAthena*)World->GameState : nullptr;
    if (!GameState || !GameState->HasAllPlayerBuildableClasses())
    {
        BOT_LOG(BOT_CATEGORY_BUILDING, "No player buildable class list on this build; bots cannot build.");
        return;
    }

    auto BuildingTypeEnum = SDK::FindEnum("EFortBuildingType");
    if (!BuildingTypeEnum)
    {
        BOT_LOG(BOT_CATEGORY_BUILDING, "EFortBuildingType not found; bots cannot build.");
        return;
    }

    int64 TypeValues[(int)EBotBuildPiece::Count];
    for (int i = 0; i < (int)EBotBuildPiece::Count; i++)
        TypeValues[i] = BuildingTypeEnum->GetValue(BuildingTypeNames[i]);

    bool bPreferredFound[(int)EBotBuildPiece::Count][(int)EBotBuildMaterial::Count] = {};

    for (auto& SubClass : GameState->AllPlayerBuildableClasses)
    {
        const UClass* Class = SubClass;
        if (!Class)
            continue;

        auto Default = (ABuildingSMActor*)Class->GetDefaultObj();
        if (!Default)
            continue;

        auto Material = (int)Default->ResourceType;
        if (Material < 0 || Material >= (int)EBotBuildMaterial::Count)
            continue;

        for (int Piece = 0; Piece < (int)EBotBuildPiece::Count; Piece++)
        {
            if (TypeValues[Piece] == -1 || Default->BuildingType != (uint8)TypeValues[Piece] || bPreferredFound[Piece][Material])
                continue;

            bool bPreferred = ToLower(Class->GetName().ToString().c_str()).find(PreferredNameHints[Piece]) != std::string::npos;

            if (bPreferred || !Classes[Piece][Material])
                Classes[Piece][Material] = Class;

            bPreferredFound[Piece][Material] = bPreferred;
        }
    }

    for (int Piece = 0; Piece < (int)EBotBuildPiece::Count; Piece++)
        for (int Material = 0; Material < (int)EBotBuildMaterial::Count; Material++)
        {
            auto Class = Classes[Piece][Material];
            if (Class)
                bAvailable = true;

            BOT_VERBOSE(BOT_CATEGORY_BUILDING, "%s %s -> %s", BotBuildCatalog::ToString((EBotBuildMaterial)Material), BotBuildCatalog::ToString((EBotBuildPiece)Piece),
                        Class ? Class->GetName().ToString().c_str() : "(none)");
        }

    if (!bAvailable)
        BOT_LOG(BOT_CATEGORY_BUILDING, "No usable build pieces found on this build; bots cannot build.");
}

namespace BotBuildCatalog
{
    const UClass* GetBuildingClass(EBotBuildPiece Piece, EBotBuildMaterial Material)
    {
        if (!bBuilt)
            BuildCatalog();

        return Classes[(int)Piece][(int)Material];
    }

    bool IsAvailable()
    {
        if (!bBuilt)
            BuildCatalog();

        return bAvailable;
    }

    void Reset()
    {
        bBuilt = false;
        bAvailable = false;
        memset(Classes, 0, sizeof(Classes));
    }

    const char* ToString(EBotBuildPiece Piece)
    {
        switch (Piece)
        {
        case EBotBuildPiece::Wall:
            return "Wall";
        case EBotBuildPiece::Floor:
            return "Floor";
        case EBotBuildPiece::Ramp:
            return "Ramp";
        case EBotBuildPiece::Cone:
            return "Cone";
        default:
            return "Unknown";
        }
    }

    const char* ToString(EBotBuildMaterial Material)
    {
        switch (Material)
        {
        case EBotBuildMaterial::Wood:
            return "Wood";
        case EBotBuildMaterial::Stone:
            return "Stone";
        case EBotBuildMaterial::Metal:
            return "Metal";
        default:
            return "Unknown";
        }
    }
}
