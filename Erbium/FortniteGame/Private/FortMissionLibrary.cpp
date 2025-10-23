#include "pch.h"
#include "../Public/FortMissionLibrary.h"
#include "../Public/FortPlayerPawnAthena.h"

// used for rifts
bool bHasWorldContextObject = false;
void UFortMissionLibrary::TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	AFortPlayerPawnAthena* PlayerPawn;
	FVector DestLocation;
	FRotator DestRotation;
	bool bIgnoreCollision;
	bool bIgnoreSupplementalKillVolumeSweep;

	if (bHasWorldContextObject)
		Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&PlayerPawn);
	Stack.StepCompiledIn(&DestLocation);
	Stack.StepCompiledIn(&DestRotation);
	Stack.StepCompiledIn(&bIgnoreCollision);
	Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
	Stack.IncrementCode();

	PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
	*Ret = true;
}

void UFortMissionLibrary::PostLoadHook()
{
	auto TeleportPlayerPawnFn = GetDefaultObj()->GetFunction("TeleportPlayerPawn");
	if (TeleportPlayerPawnFn)
		for (auto& Param : TeleportPlayerPawnFn->GetParamsNamed().NameOffsetMap)
		{
			if (Param.Name == "WorldContextObject")
			{
				bHasWorldContextObject = true;
				break;
			}
		}
	Utils::ExecHook(TeleportPlayerPawnFn, TeleportPlayerPawn);
}