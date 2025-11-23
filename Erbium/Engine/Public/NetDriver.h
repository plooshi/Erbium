#pragma once
#include "../../pch.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"

struct FObjectReplicationBridgeFilterConfig
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FObjectReplicationBridgeFilterConfig);

    DEFINE_STRUCT_PROP(ClassName, FName);
    DEFINE_STRUCT_PROP(DynamicFilterName, FName);
};
class UObjectReplicationBridgeConfig : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UObjectReplicationBridgeConfig);

    DEFINE_PROP(FilterConfigs, TArray<FObjectReplicationBridgeFilterConfig>);
};

struct FURL
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FURL);
	
    DEFINE_STRUCT_PROP(Port, int32);
};

class UActorChannel : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UActorChannel);

    DEFINE_PROP(Actor, AActor*);

    double& GetRelevantTime()
    {
        static auto Offset = GetOffset("Actor") + (VersionInfo.EngineVersion >= 5.3 ? 20 : 16);
        return *(double*)(__int64(this) + Offset);
    }

    bool IsPendingDormancy()
    {
        static auto BitfieldOffset = GetOffset("Connection") + 8;
        return *(uint8_t*)(__int64(this) + BitfieldOffset) & (1 << 7);
    }

    bool IsDormant()
    {
        static auto BitfieldOffset = GetOffset("Connection") + 8;
        return *(uint8_t*)(__int64(this) + BitfieldOffset) & (1 << 2);
    }
};

class UNetConnection : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UNetConnection);

    DEFINE_PROP(PlayerController, AFortPlayerControllerAthena*);
    DEFINE_PROP(OwningActor, AActor*);
    DEFINE_PROP(ViewTarget, AActor*);
    DEFINE_PROP(Children, TArray<UNetConnection*>);
    DEFINE_PROP(OpenChannels, TArray<UActorChannel*>);
};

struct FNetworkObjectInfo
{
    AActor* Actor;
    TWeakObjectPtr<AActor> WeakActor;
    double NextUpdateTime;
    double LastNetReplicateTime;
    float OptimalNetUpdateDelta;
    float LastNetUpdateTime;
    uint32 bPendingNetUpdate : 1;
    uint32 bForceRelevantNextUpdate : 1;
    TSet<TWeakObjectPtr<UNetConnection>> DormantConnections;
    TSet<TWeakObjectPtr<UNetConnection>> RecentlyDormantConnections;
};
template< class ObjectType>
class TSharedPtr
{
public:
    ObjectType* Object;
    void* ReferenceController;


    bool IsValid() const
    {
        return Object != nullptr;
    }
    ObjectType* Get()
    {
        return Object;
    }
    ObjectType* Get() const
    {
        return Object;
    }
    ObjectType& operator*()
    {
        return *Object;
    }
    const ObjectType& operator*() const
    {
        return *Object;
    }
    ObjectType* operator->()
    {
        return Object;
    }
    ObjectType* operator->() const
    {
        return Object;
    }
};

class FNetworkObjectList
{
public:
    typedef TSet<TSharedPtr<FNetworkObjectInfo>> FNetworkObjectSet;

    FNetworkObjectSet AllNetworkObjects;
    FNetworkObjectSet ActiveNetworkObjects;
    FNetworkObjectSet ObjectsDormantOnAllConnections;

    TMap<TWeakObjectPtr<UNetConnection>, int32> NumDormantObjectsPerConnection;
};


struct FActorDestructionInfo
{
public:
    FActorDestructionInfo()
        : Reason(0)
        , bIgnoreDistanceCulling(false)
    {
    }

    TWeakObjectPtr<class ULevel> Level;
    TWeakObjectPtr<class UObject> ObjOuter;
    struct SDK::FVector DestroyedPosition;
    uint32 NetGUID;
    class FString PathName;
    class SDK::FName StreamingLevelName;
    uint8_t Reason;

    bool bIgnoreDistanceCulling;
};

struct FNetViewer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FNetViewer);

    DEFINE_STRUCT_PROP(Connection, UNetConnection*);
    DEFINE_STRUCT_PROP(InViewer, AActor*);
    DEFINE_STRUCT_PROP(ViewTarget, AActor*);
    DEFINE_STRUCT_PROP(ViewLocation, FVector);
    DEFINE_STRUCT_PROP(ViewDir, FVector);

    static FNetViewer* Create(UNetConnection* Conn)
    {
        auto NetViewer = (FNetViewer*)malloc(FNetViewer::Size());
        NetViewer->Connection = Conn;
        NetViewer->InViewer = Conn->PlayerController ? Conn->PlayerController : Conn->OwningActor;
        NetViewer->ViewTarget = Conn->ViewTarget;

        auto ViewingController = Conn->PlayerController;

        if (ViewingController)
        {
            //FRotator ViewRotation = ViewingController->GetControlRotation();
            FRotator ViewRotation;
            AFortPlayerControllerAthena::GetPlayerViewPoint(ViewingController, NetViewer->ViewLocation, ViewRotation);
            constexpr auto radian = 0.017453292519943295;
            auto UnwindedPitch = FRotator::UnwindDegrees(ViewRotation.Pitch);
            auto UnwindedYaw = FRotator::UnwindDegrees(ViewRotation.Pitch);
            double cosPitch = cos(UnwindedPitch * radian), sinPitch = sin(UnwindedPitch * radian), cosYaw = cos(UnwindedYaw * radian), sinYaw = sin(UnwindedYaw * radian);
            NetViewer->ViewDir = FVector(cosPitch * cosYaw, cosPitch * sinYaw, sinPitch);
        }
        else
        {
            NetViewer->ViewLocation = {};
            NetViewer->ViewDir = {};
        }

        return NetViewer;
    }
};

class UNetDriver : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UNetDriver);

    DEFINE_PROP(NetDriverName, FName);
    DEFINE_PROP(World, UWorld*);
    DEFINE_PROP(ReplicationDriver, UObject*);
    DEFINE_PROP(ClientConnections, TArray<UNetConnection*>);
    DEFINE_PROP(WorldPackage, UObject*);
    DEFINE_PROP(NetServerMaxTickRate, int32);
    DEFINE_PROP(RelevantTimeout, float);

    DefHookOg(void, TickFlush, UNetDriver*, float);
    static void TickFlush__RepGraph(UNetDriver*, float);
    static void TickFlush__Iris(UNetDriver*, float);
    DefHookOg(void, NotifyActorDestroyed, UNetDriver*, AActor*, bool);

    double GetTime()
    {
        static auto TimeOff = GetOffset("Time");
        static auto Offset = TimeOff == -1 ? GetOffset("bNoTimeouts") - 9 : TimeOff;
        return TimeOff == -1 ? *(double*)(__int64(this) + Offset) : *(float*)(__int64(this) + Offset);
    }

    InitPostLoadHooks;
};