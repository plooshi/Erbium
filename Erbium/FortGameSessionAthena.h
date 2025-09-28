#pragma once
#include "pch.h"
#include "Utils.h"


class AFortGameSession : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameSession);

    DEFINE_PROP(MaxPlayers, int32);
};