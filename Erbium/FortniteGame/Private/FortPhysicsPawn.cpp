#include "pch.h"
#include "../Public/FortPhysicsPawn.h"

struct FReplicatedPhysicsPawnState
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FReplicatedPhysicsPawnState);

    DEFINE_STRUCT_PROP(Rotation, FQuat);
    DEFINE_STRUCT_PROP(Translation, FVector);
    DEFINE_STRUCT_PROP(LinearVelocity, FVector);
    DEFINE_STRUCT_PROP(AngularVelocity, FVector);
};

struct FReplicatedAthenaVehiclePhysicsState
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FReplicatedAthenaVehiclePhysicsState);

    DEFINE_STRUCT_PROP(Rotation, FQuat);
    DEFINE_STRUCT_PROP(Translation, FVector);
    DEFINE_STRUCT_PROP(LinearVelocity, FVector);
    DEFINE_STRUCT_PROP(AngularVelocity, FVector);
};

class UPrimitiveComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UPrimitiveComponent);

    DEFINE_FUNC(K2_SetWorldLocationAndRotation, void);
    DEFINE_FUNC(SetPhysicsLinearVelocity, void);
    DEFINE_FUNC(SetPhysicsAngularVelocityInDegrees, void);
};

void ServerMove(UObject* Context, FFrame& Stack)
{
    FQuat Rotation;
    FVector Translation;
    FVector LinearVelocity;
    FVector AngularVelocity;

    static auto StateStruct = FReplicatedPhysicsPawnState::StaticStruct();
    if (StateStruct)
    {
        auto State = (FReplicatedPhysicsPawnState*)malloc(FReplicatedPhysicsPawnState::Size());
        Stack.StepCompiledIn(State);
        Stack.IncrementCode();

        Rotation = State->Rotation;
        Translation = State->Translation;
        LinearVelocity = State->LinearVelocity;
        AngularVelocity = State->AngularVelocity;

        free(State);
    }
    else
    {
        auto State = (FReplicatedAthenaVehiclePhysicsState*)malloc(FReplicatedAthenaVehiclePhysicsState::Size());
        Stack.StepCompiledIn(State);
        Stack.IncrementCode();

        Rotation = State->Rotation;
        Translation = State->Translation;
        LinearVelocity = State->LinearVelocity;
        AngularVelocity = State->AngularVelocity;
        
        free(State);
    }
    auto Pawn = (AFortPhysicsPawn*)Context;


    if (VersionInfo.EngineVersion < 4.24)
    {
        Rotation.X -= 2.5f;
        Rotation.Y /= 0.3f;
        Rotation.Z -= -2.0f;
        Rotation.W /= -1.2f;
    }

    UPrimitiveComponent* RootComponent = Pawn->RootComponent->Cast<UPrimitiveComponent>();

    if (RootComponent)
    {
        FRotator RealRotation = Rotation.Rotator();

        if (VersionInfo.FortniteVersion >= 4.24)
        {
            RealRotation.Yaw = FRotator::UnwindDegrees(RealRotation.Yaw);

            RealRotation.Pitch = 0;
            RealRotation.Roll = 0;
        }

        RootComponent->K2_SetWorldLocationAndRotation(Translation, RealRotation, false, nullptr, true);
        RootComponent->SetPhysicsLinearVelocity(LinearVelocity, 0, FName(0));
        RootComponent->SetPhysicsAngularVelocityInDegrees(AngularVelocity, 0, FName(0));
    }
}

void AFortPhysicsPawn::Hook()
{
    auto ServerMoveFn = GetDefaultObj()->GetFunction("ServerMove");

    if (ServerMoveFn)
        Utils::ExecHook(ServerMoveFn, ServerMove);
    else
    {
        auto ServerUpdatePhysicsParamsFn = GetDefaultObj()->GetFunction("ServerUpdatePhysicsParams");

        if (ServerUpdatePhysicsParamsFn)
            Utils::ExecHook(ServerUpdatePhysicsParamsFn, ServerMove);
        else
            Utils::ExecHook(DefaultObjImpl("FortAthenaVehicle")->GetFunction("ServerUpdatePhysicsParams"), ServerMove);
    }
}