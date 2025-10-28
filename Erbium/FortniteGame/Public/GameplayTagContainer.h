#pragma once
#include "../../pch.h"

struct FGameplayTag
{
	FName TagName;

	static int32 Size()
	{
		return VersionInfo.FortniteVersion >= 20.00 ? 4 : 8;
	}
};

struct FGameplayTagContainer
{
public:
	TArray<FGameplayTag> GameplayTags;
	TArray<FGameplayTag> ParentTags;

	bool HasTag(const FGameplayTag& TagToCheck) const
	{
		for (auto& Tag : GameplayTags)
			if (Tag.TagName == TagToCheck.TagName)
				return true;

		for (auto& Tag : ParentTags)
			if (Tag.TagName == TagToCheck.TagName)
				return true;

		return false;
	}

	bool HasAll(const FGameplayTagContainer& ContainerToCheck) const
	{
		for (auto& Tag : ContainerToCheck.GameplayTags) 
		{
			bool Found = false;
			for (auto& Tag2 : GameplayTags)
				if (Tag2.TagName == Tag.TagName) 
				{
					Found = true;
					break;
				}

			if (!Found) 
				for (auto& Tag2 : ParentTags)
    {
					if (Tag2.TagName == Tag.TagName) 
					{
						Found = true;
						break;
					}
				}

			if (!Found) 
				return false;
		}
		return true;
	}

	void AppendTags(const FGameplayTagContainer& Other)
	{
		for (const auto& GameplayTag : Other.GameplayTags)
			if (!HasTag(GameplayTag))
				GameplayTags.Add(GameplayTag);

		for (const auto& ParentTag : Other.ParentTags)
			if (!HasTag(ParentTag))
				ParentTags.Add(ParentTag);
	}
};