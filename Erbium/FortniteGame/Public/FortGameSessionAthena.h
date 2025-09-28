#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"


class AFortGameSession : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameSession);

    DEFINE_PROP(MaxPlayers, int32);
};