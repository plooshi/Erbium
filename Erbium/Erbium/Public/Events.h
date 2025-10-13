#pragma once
#include "../../pch.h"

struct FEventFunction
{
    bool bIsLoaderFunction;
    const wchar_t* FunctionPath;
};

struct FEvent
{
    const wchar_t* LoaderClass;
    const wchar_t* ScriptingClass;
    std::vector<FEventFunction> EventFunctions;
    double EventVersion;
    const wchar_t* LoaderFuncPath;
};

namespace Events
{
    void StartEvent();
}