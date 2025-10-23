#pragma once
#include "../../pch.h"
#include "Utils.h"

// these dont really fit into a class
class Misc
{
public:
    static inline bool bHookedAll = false;

    static int GetNetMode();
    DefHookOg(void*, SendRequestNow, void*, void*, int);
    DefHookOg(float, GetMaxTickRate, UEngine*, float, bool);
    static uint32 CheckCheckpointHeartBeat();
    DefHookOg(void, ApplyHomebaseEffectsOnPlayerSetup, __int64*, __int64, __int64, __int64, UObject*, char, unsigned __int8);
    static void InitClient();

    InitHooks;
};