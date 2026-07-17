#include "../../pch.h"
#include "../Erbium/FortniteGame/Public/FortWeapon.h"

struct FHitResult
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FHitResult);
    DEFINE_STRUCT_PROP(Component, TWeakObjectPtr<UActorComponent>);
};

class AFortWeaponRanged : AFortWeapon
{
public:
    static void ServerNotifyPawnHit(UObject* Context, FFrame& Stack);
    InitPostLoadHooks;
};