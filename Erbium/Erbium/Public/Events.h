#pragma once
#include "../../pch.h"

struct FEventFunction
{
    bool bIsLoaderFunction;
    UEAllocatedWString FunctionPath;
};

struct FEvent
{
    UEAllocatedWString LoaderClass;
    UEAllocatedWString ScriptingClass;
    std::vector<FEventFunction> EventFunctions;
    double EventVersion;
    UEAllocatedWString LoaderFuncPath;
};

namespace Events
{
    void StartEvent();
}