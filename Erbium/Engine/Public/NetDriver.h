#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

struct FURL final
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FURL);
	
    DEFINE_STRUCT_PROP(Port, int32);
};

class UNetDriver : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UNetDriver);

    DEFINE_PROP(NetDriverName, FName);
    DEFINE_PROP(World, UWorld*);
    DEFINE_PROP(ReplicationDriver, UObject*);

    DefHookOg(void, TickFlush, UNetDriver*, float);

    InitHooks;
};