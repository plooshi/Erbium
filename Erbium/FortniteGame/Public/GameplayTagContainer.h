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
		for (int x = 0; x < GameplayTags.Num(); x++)
		{
			auto& Tag = GameplayTags.Get(x, FGameplayTag::Size());

			if (Tag.TagName == TagToCheck.TagName)
				return true;
		}

		for (int x = 0; x < ParentTags.Num(); x++)
		{
			auto& Tag = ParentTags.Get(x, FGameplayTag::Size());

			if (Tag.TagName == TagToCheck.TagName)
				return true;
		}

		return false;
	}

	bool HasAll(const FGameplayTagContainer& ContainerToCheck) const
	{
		//for (auto& Tag : ContainerToCheck.GameplayTags) 
		for (int i = 0; i < ContainerToCheck.GameplayTags.Num(); i++)
		{
			auto& Tag = ContainerToCheck.GameplayTags.Get(i, FGameplayTag::Size());

			bool Found = false;
			for (int x = 0; x < GameplayTags.Num(); x++)
			{
				auto& Tag2 = GameplayTags.Get(x, FGameplayTag::Size());

				if (Tag2.TagName == Tag.TagName)
				{
					Found = true;
					break;
				}
			}

			if (!Found)
				for (int x = 0; x < ParentTags.Num(); x++)
				{
					auto& Tag2 = ParentTags.Get(x, FGameplayTag::Size());

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

	bool HasAny(const FGameplayTagContainer& ContainerToCheck) const
	{
		//for (auto& Tag : ContainerToCheck.GameplayTags) 
		for (int i = 0; i < ContainerToCheck.GameplayTags.Num(); i++)
		{
			auto& Tag = ContainerToCheck.GameplayTags.Get(i, FGameplayTag::Size());

			for (int x = 0; x < GameplayTags.Num(); x++)
			{
				auto& Tag2 = GameplayTags.Get(x, FGameplayTag::Size());

				if (Tag2.TagName == Tag.TagName)
					return true;
			}

			for (int x = 0; x < ParentTags.Num(); x++)
			{
				auto& Tag2 = ParentTags.Get(x, FGameplayTag::Size());

				if (Tag2.TagName == Tag.TagName)
					return true;
			}
		}

		return false;
	}

	void AppendTags(const FGameplayTagContainer& Other)
	{
		for (int i = 0; i < Other.GameplayTags.Num(); i++)
		{
			auto& GameplayTag = Other.GameplayTags.Get(i, FGameplayTag::Size());

			if (!HasTag(GameplayTag))
				GameplayTags.Add(GameplayTag, FGameplayTag::Size());
		}

		for (int i = 0; i < Other.ParentTags.Num(); i++)
		{
			auto& ParentTag = Other.ParentTags.Get(i, FGameplayTag::Size());

			if (!HasTag(ParentTag))
				ParentTags.Add(ParentTag, FGameplayTag::Size());
		}
	}
};