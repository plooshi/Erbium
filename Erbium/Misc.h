#pragma once
#include "pch.h"
#include "Utils.h"

// these dont really fit into a class
class Misc
{
public:
    static int GetNetMode();
    DefHookOg(void*, DispatchRequest, void*, void*, int);
    DefHookOg(float, GetMaxTickRate, UEngine*, float, bool);
    static uint32 CheckCheckpointHeartBeat();

    InitHooks;
};