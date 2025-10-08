#pragma once
#include "pch.h"

struct FEventFunction
{
    bool bIsLoaderFunction;
    UEAllocatedWString FunctionPath;
};

struct FEvent
{
    UEAllocatedWString Loader;
    UEAllocatedWString Scripting;
    std::vector<FEventFunction> EventFunctions;
    double Version;
    UEAllocatedWString LoaderFuncPath;
};

namespace Events
{
    void StartEvent();
}