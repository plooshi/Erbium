#pragma once
#include "../../pch.h"

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

// FKey (InputCore): the key's name plus a TSharedPtr<FKeyDetails> the engine looks up lazily, so a
// zeroed pointer is valid. 0x18 bytes on every UE4/UE5 build Erbium supports.
struct FInputKey
{
    FName KeyName;
    void* KeyDetails[2] = {};
};

class UInputSettings : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UInputSettings);

    DEFINE_PROP(ConsoleKeys, TArray<FInputKey>);
};

class Client
{
public:
    static void Init();
};