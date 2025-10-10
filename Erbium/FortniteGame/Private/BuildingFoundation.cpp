#include "pch.h"
#include "../Public/BuildingFoundation.h"

void ABuildingFoundation::SetDynamicFoundationEnabled_(UObject* Context, FFrame& Stack)
{
	auto Foundation = (ABuildingFoundation*)Context;
	bool bEnabled;
	Stack.StepCompiledIn(&bEnabled);
	Stack.IncrementCode();
	
	if (Foundation->HasDynamicFoundationRepData())
	{
		Foundation->DynamicFoundationRepData.EnabledState = bEnabled ? 1 : 2;
		Foundation->OnRep_DynamicFoundationRepData();
	}
	if (Foundation->HasFoundationEnabledState())
		Foundation->FoundationEnabledState = bEnabled ? 1 : 2;
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
	//Foundation->StreamingData.BoundingBox = Foundation->StreamingBoundingBox;
	if (Foundation->HasDynamicFoundationRepData())
		Foundation->OnRep_DynamicFoundationRepData();
}

void ABuildingFoundation::Hook()
{
	if (!GetDefaultObj())
		return;

	Utils::ExecHook(GetDefaultObj()->GetFunction("SetDynamicFoundationEnabled"), SetDynamicFoundationEnabled_);
	Utils::ExecHook(GetDefaultObj()->GetFunction("SetDynamicFoundationTransform"), SetDynamicFoundationTransform_);
}
