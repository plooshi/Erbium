#pragma once
#include "../../pch.h"

class FFortPossibleMission
{

public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FFortPossibleMission);

	DEFINE_STRUCT_PROP(MissionInfo, TSoftObjectPtr<class UFortMissionInfo>);
	DEFINE_STRUCT_PROP(Weight, float);
	DEFINE_STRUCT_PROP(MinAlwaysGenerated, int32);
	DEFINE_STRUCT_PROP(bIsPrototype, bool);
};

class FFortMissionInfoOption 
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FFortMissionInfoOption);

	DEFINE_STRUCT_PROP(MissionInfo, TSoftObjectPtr<UFortMissionInfo>);
	DEFINE_STRUCT_PROP(MinDifficultyLevel, float);


};

class UFortMissionGenerator : public UObject 
{
public:
	UCLASS_COMMON_MEMBERS(UFortMissionGenerator);



	DEFINE_PROP(PrimaryMissionInfo, TSoftObjectPtr<UFortMissionInfo>);
	DEFINE_PROP(SecondaryMissionList, TArray<FFortPossibleMission>);
	DEFINE_PROP(TertiaryMissionList, TArray<FFortPossibleMission>);
	DEFINE_PROP(SurvivorMissionList, TArray<FFortPossibleMission>);

	DEFINE_PROP(MissionInfo, TSoftObjectPtr<UFortMissionInfo>);

	DEFINE_PROP(MissionInfoList, TArray<FFortMissionInfoOption>);

};

class AFortPlacementActor : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortPlacementActor);

	DEFINE_PROP(OccupyingActor, TWeakObjectPtr<AActor>);
};
	