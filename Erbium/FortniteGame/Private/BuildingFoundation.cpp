#include "pch.h"
#include "../Public/BuildingFoundation.h"

uint64_t SelectAndSetupMyBuildingLevel_ = 0;

void ABuildingFoundation::SetDynamicFoundationEnabled_(UObject* Context, FFrame& Stack)
{
    auto Foundation = (ABuildingFoundation*)Context;
    bool bEnabled;
    Stack.StepCompiledIn(&bEnabled);
    Stack.IncrementCode();
    if (Foundation->LevelToStream.ComparisonIndex == 0 && SelectAndSetupMyBuildingLevel_) 
    {
        auto SelectAndSetupMyBuildingLevel = (bool (*)(ABuildingFoundation*, void*))SelectAndSetupMyBuildingLevel_;
        SelectAndSetupMyBuildingLevel(Foundation, nullptr);
    }
    
    if (Foundation->HasDynamicFoundationRepData())
    {
        Foundation->DynamicFoundationRepData.EnabledState = bEnabled ? 1 : 2;
        Foundation->OnRep_DynamicFoundationRepData();
    }
    if (Foundation->HasFoundationEnabledState())
        Foundation->FoundationEnabledState = bEnabled ? 1 : 2;

    //if (VersionInfo.FortniteVersion >= 14 and VersionInfo.FortniteVersion < 15)// Idk why, but it is not called on s14 maybe on more szns
    Foundation->SetDynamicFoundationTransform(Foundation->GetTransform());
}

void ABuildingFoundation::SetDynamicFoundationTransform_(UObject* Context, FFrame& Stack)
{
    auto Foundation = (ABuildingFoundation*)Context;
    auto& Transform = Stack.StepCompiledInRef<FTransform>();
    Stack.IncrementCode();

    Foundation->DynamicFoundationTransform = Transform;
    if (Foundation->HasDynamicFoundationRepData())
    {
        Foundation->DynamicFoundationRepData.Rotation = Transform.Rotation.Rotator();
        Foundation->DynamicFoundationRepData.Translation = Transform.Translation;
    }
    Foundation->StreamingData.FoundationLocation = Transform.Translation;
    // Foundation->StreamingData.BoundingBox = Foundation->StreamingBoundingBox;
    if (Foundation->HasDynamicFoundationRepData())
        Foundation->OnRep_DynamicFoundationRepData();
}

void ABuildingFoundation::Hook()
{
    if (!GetDefaultObj())
        return;

    SelectAndSetupMyBuildingLevel_ = FindSelectAndSetupMyBuildingLevel();

    Utils::ExecHook(GetDefaultObj()->GetFunction("SetDynamicFoundationEnabled"), SetDynamicFoundationEnabled_);
    Utils::ExecHook(GetDefaultObj()->GetFunction("SetDynamicFoundationTransform"), SetDynamicFoundationTransform_);
}
