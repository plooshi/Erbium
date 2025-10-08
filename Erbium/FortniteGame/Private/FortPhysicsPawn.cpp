#include "pch.h"
#include "../Public/FortPhysicsPawn.h"

void ServerMove(UObject* Context, FFrame& Stack)
{

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