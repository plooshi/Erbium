#include "pch.h"
#include "../../Public/Navigation/BotNavigation.h"
#include "../../Public/BotController.h"
#include "../../Public/BotEngineTypes.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortPlayerPawnAthena.h"

// Engine enum values resolved by name, with the engine's long-standing numeric values as fallback.
static int64 GetEnumValue(const char* EnumName, const char* ValueName, int64 Fallback)
{
    auto Enum = SDK::FindEnum(EnumName);
    auto Value = Enum ? Enum->GetValue(ValueName) : -1;
    return Value != -1 ? Value : Fallback;
}

static EBotNavRequestResult ToRequestResult(uint8 Result)
{
    static auto Failed = GetEnumValue("EPathFollowingRequestResult", "Failed", 0);
    static auto AlreadyAtGoal = GetEnumValue("EPathFollowingRequestResult", "AlreadyAtGoal", 1);

    if (Result == Failed)
        return EBotNavRequestResult::Failed;
    if (Result == AlreadyAtGoal)
        return EBotNavRequestResult::AlreadyAtGoal;
    return EBotNavRequestResult::Moving;
}

// AAIController::MoveToLocation / MoveToActor. Pathfinding and DirectPath differ only in bUsePathfinding.
class FBotNavigation_AIMoveTo final : public IBotNavigation
{
public:
    explicit FBotNavigation_AIMoveTo(bool bInUsePathfinding)
        : bUsePathfinding(bInUsePathfinding)
    {
    }

    EBotNavMode GetMode() const override { return bUsePathfinding ? EBotNavMode::Pathfinding : EBotNavMode::DirectPath; }

    EBotNavRequestResult MoveToLocation(FBotController& Bot, const FVector& Goal, float AcceptanceRadius) override
    {
        auto AI = Bot.GetAIController();
        if (!AI)
            return EBotNavRequestResult::Failed;

        // Face the destination while walking: Fortnite pawns turn with the control rotation, and the
        // AI controller derives that from its focal point.
        FVector Dest = Goal;
        AI->K2_SetFocalPoint(Dest);

        // (Dest, AcceptanceRadius, bStopOnOverlap, bUsePathfinding, bProjectDestinationToNavigation,
        //  bCanStrafe, FilterClass, bAllowPartialPath). Argument types must match the UFunction exactly.
        bool bStopOnOverlap = true, bUsePath = bUsePathfinding, bProject = bUsePathfinding, bCanStrafe = true, bAllowPartialPath = true;
        const UClass* FilterClass = nullptr;
        auto Result = AI->MoveToLocation(Dest, AcceptanceRadius, bStopOnOverlap, bUsePath, bProject, bCanStrafe, FilterClass, bAllowPartialPath);

        return ToRequestResult(Result);
    }

    EBotNavRequestResult MoveToActor(FBotController& Bot, AActor* Goal, float AcceptanceRadius) override
    {
        auto AI = Bot.GetAIController();
        if (!AI || !Goal)
            return EBotNavRequestResult::Failed;

        AI->K2_SetFocus(Goal);

        // (Goal, AcceptanceRadius, bStopOnOverlap, bUsePathfinding, bCanStrafe, FilterClass, bAllowPartialPath)
        bool bStopOnOverlap = true, bUsePath = bUsePathfinding, bCanStrafe = true, bAllowPartialPath = true;
        const UClass* FilterClass = nullptr;
        auto Result = AI->MoveToActor(Goal, AcceptanceRadius, bStopOnOverlap, bUsePath, bCanStrafe, FilterClass, bAllowPartialPath);

        return ToRequestResult(Result);
    }

    void Stop(FBotController& Bot) override
    {
        if (auto AI = Bot.GetAIController())
        {
            AI->StopMovement();
            AI->K2_ClearFocus();
        }
    }

    bool IsMoving(FBotController& Bot) const override
    {
        static auto Moving = GetEnumValue("EPathFollowingStatus", "Moving", 3);

        auto AI = Bot.GetAIController();
        return AI && AI->GetMoveStatus() == Moving;
    }

private:
    bool bUsePathfinding;
};

// APawn::AddMovementInput every frame. The pawn of an AI controller is locally controlled on the
// server, so the character movement component consumes this input like a human's stick input.
class FBotNavigation_Steering final : public IBotNavigation
{
public:
    EBotNavMode GetMode() const override { return EBotNavMode::Steering; }

    EBotNavRequestResult MoveToLocation(FBotController& Bot, const FVector& Goal, float AcceptanceRadius) override
    {
        auto AI = Bot.GetAIController();
        if (!AI || !Bot.GetPawn())
            return EBotNavRequestResult::Failed;

        FVector Dest = Goal;
        AI->K2_SetFocalPoint(Dest);
        return EBotNavRequestResult::Moving;
    }

    EBotNavRequestResult MoveToActor(FBotController& Bot, AActor* Goal, float AcceptanceRadius) override
    {
        auto AI = Bot.GetAIController();
        if (!AI || !Goal || !Bot.GetPawn())
            return EBotNavRequestResult::Failed;

        AI->K2_SetFocus(Goal);
        return EBotNavRequestResult::Moving;
    }

    void Stop(FBotController& Bot) override
    {
        if (auto AI = Bot.GetAIController())
            AI->K2_ClearFocus();
    }

    // Steering has no request to finish; FBotMovement decides arrival from distance.
    bool IsMoving(FBotController& Bot) const override { return true; }

    bool NeedsFrameTick() const override { return true; }

    void TickFrame(FBotController& Bot, const FVector& Goal, float AcceptanceRadius) override
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn)
            return;

        auto Location = Pawn->K2_GetActorLocation();
        auto DX = Goal.X - Location.X;
        auto DY = Goal.Y - Location.Y;
        auto Distance = sqrt(DX * DX + DY * DY);

        if (Distance <= AcceptanceRadius || Distance < 1.0)
            return;

        FVector Direction(DX / Distance, DY / Distance, 0.0);
        float Scale = 1.f;
        bool bForce = false;
        ((ACharacter*)Pawn)->AddMovementInput(Direction, Scale, bForce);
    }
};

// Player-controller backend: nothing can move the pawn (see EBotControllerKind::PlayerController).
class FBotNavigation_None final : public IBotNavigation
{
public:
    EBotNavMode GetMode() const override { return EBotNavMode::None; }
    EBotNavRequestResult MoveToLocation(FBotController&, const FVector&, float) override { return EBotNavRequestResult::Failed; }
    EBotNavRequestResult MoveToActor(FBotController&, AActor*, float) override { return EBotNavRequestResult::Failed; }
    void Stop(FBotController&) override {}
    bool IsMoving(FBotController&) const override { return false; }
};

namespace BotNavigation
{
    struct FModeStats
    {
        int Successes = 0;
        int ConsecutiveFailures = 0;
        bool bDisabled = false;
    };

    static constexpr int MaxFailuresBeforeDisable = 8;

    static FModeStats Stats[4];
    static bool bNavQueriesBroken = false;
    static int NavQueryFailures = 0;
    static int NavQuerySuccesses = 0;

    IBotNavigation* Get(EBotNavMode Mode)
    {
        static FBotNavigation_AIMoveTo Pathfinding(true);
        static FBotNavigation_AIMoveTo DirectPath(false);
        static FBotNavigation_Steering Steering;
        static FBotNavigation_None None;

        switch (Mode)
        {
        case EBotNavMode::Pathfinding:
            return &Pathfinding;
        case EBotNavMode::DirectPath:
            return &DirectPath;
        case EBotNavMode::Steering:
            return &Steering;
        case EBotNavMode::None:
            break;
        }

        return &None;
    }

    EBotNavMode GetFallbackMode(EBotNavMode Mode)
    {
        switch (Mode)
        {
        case EBotNavMode::Pathfinding:
            return EBotNavMode::DirectPath;
        case EBotNavMode::DirectPath:
            return EBotNavMode::Steering;
        default:
            return EBotNavMode::None;
        }
    }

    EBotNavMode GetPreferredMode()
    {
        auto Mode = FBotSystemConfig::bUseNavMesh ? EBotNavMode::Pathfinding : EBotNavMode::DirectPath;

        while (Mode != EBotNavMode::Steering && Stats[(int)Mode].bDisabled)
            Mode = GetFallbackMode(Mode);

        return Mode;
    }

    void ReportResult(EBotNavMode Mode, bool bSucceeded)
    {
        if (Mode == EBotNavMode::None || Mode == EBotNavMode::Steering)
            return;

        auto& ModeStats = Stats[(int)Mode];

        if (bSucceeded)
        {
            ModeStats.Successes++;
            ModeStats.ConsecutiveFailures = 0;
            return;
        }

        if (++ModeStats.ConsecutiveFailures >= MaxFailuresBeforeDisable && ModeStats.Successes == 0 && !ModeStats.bDisabled)
        {
            ModeStats.bDisabled = true;
            BOT_LOG(BOT_CATEGORY_NAV, "%s never succeeded (%d failures); using %s from now on.", ToString(Mode), ModeStats.ConsecutiveFailures, ToString(GetPreferredMode()));
        }
    }

    FVector GetRandomPointInRadius(const FVector& Origin, float Radius)
    {
        if (FBotSystemConfig::bUseNavMesh && !bNavQueriesBroken && UNavigationSystemV1::StaticClass())
        {
            FVector Result{};
            FVector OriginCopy = Origin;
            AActor* NavData = nullptr;
            const UClass* FilterClass = nullptr;

            if (UNavigationSystemV1::K2_GetRandomReachablePointInRadius(UWorld::GetWorld(), OriginCopy, &Result, Radius, NavData, FilterClass))
            {
                NavQuerySuccesses++;
                return Result;
            }

            if (++NavQueryFailures >= MaxFailuresBeforeDisable && NavQuerySuccesses == 0)
            {
                bNavQueriesBroken = true;
                BOT_LOG(BOT_CATEGORY_NAV, "No nav mesh data answered %d queries; picking search points without it.", NavQueryFailures);
            }
        }

        auto Angle = (double)UKismetMathLibrary::RandomFloatInRange(0.f, 6.2831853f);
        auto Distance = (double)UKismetMathLibrary::RandomFloatInRange(Radius * 0.3f, Radius);

        return FVector(Origin.X + cos(Angle) * Distance, Origin.Y + sin(Angle) * Distance, Origin.Z);
    }

    const char* ToString(EBotNavMode Mode)
    {
        switch (Mode)
        {
        case EBotNavMode::None:
            return "None";
        case EBotNavMode::Pathfinding:
            return "Pathfinding";
        case EBotNavMode::DirectPath:
            return "DirectPath";
        case EBotNavMode::Steering:
            return "Steering";
        }
        return "Unknown";
    }
}
