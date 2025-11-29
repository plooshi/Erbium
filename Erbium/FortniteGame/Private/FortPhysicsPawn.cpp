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

    DEFINE_BITFIELD_PROP(bComponentToWorldUpdated);

    DEFINE_FUNC(K2_SetWorldLocationAndRotation, void);
    DEFINE_FUNC(K2_SetWorldTransform, void);
    DEFINE_FUNC(SetPhysicsLinearVelocity, void);
    DEFINE_FUNC(SetPhysicsAngularVelocityInDegrees, void);
    DEFINE_FUNC(SetPhysicsAngularVelocityInRadians, void);
};

void AFortPhysicsPawn::ServerMove(UObject* Context, FFrame& Stack)
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

        if (VersionInfo.EngineVersion >= 4.24)
        {
            RealRotation.Yaw = FRotator::UnwindDegrees(RealRotation.Yaw);
            RealRotation.Pitch = 0;
            RealRotation.Roll = 0;
        }
        
        RootComponent->K2_SetWorldLocationAndRotation(Translation, RealRotation, false, nullptr, true);
        RootComponent->SetPhysicsLinearVelocity(LinearVelocity, 0, FName(0));
        RootComponent->SetPhysicsAngularVelocityInRadians(AngularVelocity, 0, FName(0));
    }
}

void AFortOctopusVehicle::ServerUpdateTowhook(UObject* Context, FFrame& Stack)
{
    FVector InNetTowhookAimDir;

    Stack.StepCompiledIn(&InNetTowhookAimDir);
    Stack.IncrementCode();
    auto Vehicle = (AFortOctopusVehicle*)Context;

    printf("ServerUpdateTowhook\n");
    Vehicle->NetTowhookAimDir = InNetTowhookAimDir;
    Vehicle->OnRep_NetTowhookAimDir();
}

void AFortSpaghettiVehicle::ServerUpdateTowhook(UObject* Context, FFrame& Stack)
{
    FVector InNetTowhookAimDir;

    Stack.StepCompiledIn(&InNetTowhookAimDir);
    Stack.IncrementCode();
    auto Vehicle = (AFortSpaghettiVehicle*)Context;

    Vehicle->NetTowhookAimDir = InNetTowhookAimDir;
    Vehicle->OnRep_NetTowhookAimDir();
}

struct FHitResult
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FHitResult);

    DEFINE_STRUCT_PROP(Location, FVector);
    DEFINE_STRUCT_PROP(ImpactPoint, FVector);
    DEFINE_STRUCT_PROP(ImpactNormal, FVector);
    DEFINE_STRUCT_PROP(Normal, FVector);
    DEFINE_STRUCT_PROP(Component, TWeakObjectPtr<UActorComponent>);
};

struct FAttachedInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAttachedInfo);

    DEFINE_STRUCT_PROP(Hit, FHitResult);
};

class AFortOctopusTowhookAttachableProjectile : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortOctopusTowhookAttachableProjectile);

    DEFINE_PROP(AttachedInfo, FAttachedInfo);
    DEFINE_PROP(OwningVehicle, AFortOctopusVehicle*);

    DEFINE_FUNC(Kill, void);
};

uint64_t CanGrappleToComponent_ = 0;
void (*OnRep_ReplicatedAttachedInfoOG)(AFortOctopusTowhookAttachableProjectile* _this);
void OnRep_ReplicatedAttachedInfo(AFortOctopusTowhookAttachableProjectile* _this)
{
    OnRep_ReplicatedAttachedInfoOG(_this);

    auto OwningVehicle = _this->OwningVehicle;
    if (!OwningVehicle)
        return;

    auto Comp = _this->AttachedInfo.Hit.Component.Get();
    /*if (!Comp)
    {
        _this->Kill();
        return;
    }*/

    auto CanGrappleToComponent = (bool(*)(AFortOctopusVehicle*, UActorComponent*))CanGrappleToComponent_;

    if (!CanGrappleToComponent(OwningVehicle, Comp))
    {
        // game automatically kills for us in OG
        return;
    }

    // the old projectile is supposed to die too, i dont know why it doesn't. i'll have to look at this more
    OwningVehicle->ReplicatedAttachState.Component = Comp;
    OwningVehicle->ReplicatedAttachState.LocalLocation = UKismetMathLibrary::InverseTransformLocation(Comp->K2_GetComponentToWorld(), _this->AttachedInfo.Hit.Location);
    OwningVehicle->ReplicatedAttachState.LocalNormal = UKismetMathLibrary::InverseTransformDirection(Comp->K2_GetComponentToWorld(), _this->AttachedInfo.Hit.Normal);
    OwningVehicle->OnRep_ReplicatedAttachState();

    printf("[Ballers] Comp: %s, Owner %s\n", Comp->Name.ToString().c_str(), Comp->GetOwner()->Name.ToString().c_str());
    printf("[Ballers] Location: %f %f %f [World] -> ", _this->AttachedInfo.Hit.Location.X, _this->AttachedInfo.Hit.Location.Y, _this->AttachedInfo.Hit.Location.Z);
    printf("%f %f %f [Local]\n", OwningVehicle->ReplicatedAttachState.LocalLocation.X, OwningVehicle->ReplicatedAttachState.LocalLocation.Y, OwningVehicle->ReplicatedAttachState.LocalLocation.Z);
    printf("[Ballers] Normal: %f %f %f [World] -> ", _this->AttachedInfo.Hit.Normal.X, _this->AttachedInfo.Hit.Normal.Y, _this->AttachedInfo.Hit.Normal.Z);
    printf("%f %f %f [Local]\n", OwningVehicle->ReplicatedAttachState.LocalNormal.X, OwningVehicle->ReplicatedAttachState.LocalNormal.Y, OwningVehicle->ReplicatedAttachState.LocalNormal.Z);
    //printf("CALLED!!!!\n");
}

void AFortPhysicsPawn::Hook()
{
    auto DefaultPhysPawn = GetDefaultObj();
    if (DefaultPhysPawn)
    {
        auto ServerMoveFn = DefaultPhysPawn->GetFunction("ServerMove");

        if (ServerMoveFn)
        {
            Utils::ExecHook(ServerMoveFn, ServerMove);
            Utils::ExecHook(DefaultPhysPawn->GetFunction("ServerMoveReliable"), ServerMove);
        }
        else
        {
            auto ServerUpdatePhysicsParamsFn = DefaultPhysPawn->GetFunction("ServerUpdatePhysicsParams");

            if (ServerUpdatePhysicsParamsFn)
                Utils::ExecHook(ServerUpdatePhysicsParamsFn, ServerMove);
        }
    }
    else
    {
        auto DefaultVehicle = DefaultObjImpl("FortAthenaVehicle");

        if (DefaultVehicle)
            Utils::ExecHook(DefaultVehicle->GetFunction("ServerUpdatePhysicsParams"), ServerMove);
    }

    auto DefaultOctopusVehicle = AFortOctopusVehicle::GetDefaultObj();

    if (DefaultOctopusVehicle)
    {
        Utils::ExecHook(DefaultOctopusVehicle->GetFunction("ServerUpdateTowhook"), AFortOctopusVehicle::ServerUpdateTowhook);
    }

    auto DefaultSpaghettiVehicle = AFortSpaghettiVehicle::GetDefaultObj();

    if (DefaultSpaghettiVehicle)
        Utils::ExecHook(DefaultSpaghettiVehicle->GetFunction("ServerUpdateTowhook"), AFortSpaghettiVehicle::ServerUpdateTowhook);

    if (AFortOctopusTowhookAttachableProjectile::StaticClass())
    {
        auto OnRep_ReplicatedAttachedInfoIdx = AFortOctopusTowhookAttachableProjectile::GetDefaultObj()->GetFunction("OnRep_ReplicatedAttachedInfo")->GetVTableIndex();

        auto OnRep_ReplicatedAttachedInfo__Impl = AFortOctopusTowhookAttachableProjectile::GetDefaultObj()->Vft[OnRep_ReplicatedAttachedInfoIdx];
        auto CanGrappleToComponent = Memcury::Scanner(OnRep_ReplicatedAttachedInfo__Impl).ScanFor({ 0xFF, 0x90 }).Get();
        
        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(CanGrappleToComponent - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B)
            {
                if (*(Ptr + 3) == 0xE8)
                {
                    CanGrappleToComponent_ = Memcury::Scanner(Ptr).RelativeOffset(4).Get();
                    break;
                }
                else if (*(Ptr + 3) == 0xFF && (*(Ptr + 4) & 0xF0) == 0x90)
                {
                    CanGrappleToComponent_ = uint64_t(AFortOctopusTowhookAttachableProjectile::GetDefaultObj()->Vft[*(uint32_t*)(Ptr + 5) / 8]);
                    break;
                }
            }
        }
        //.ScanFor({ 0x48, 0x8B, 0xFF, 0xE8 }, false, 0, 1, 2048, true).RelativeOffset(4).Get();

        Utils::Hook<AFortOctopusTowhookAttachableProjectile>(OnRep_ReplicatedAttachedInfoIdx, OnRep_ReplicatedAttachedInfo, OnRep_ReplicatedAttachedInfoOG);
    }
}