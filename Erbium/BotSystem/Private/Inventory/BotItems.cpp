#include "pch.h"
#include "../../Public/Inventory/BotItems.h"
#include "../../../FortniteGame/Public/FortInventory.h"

static bool ContainsAny(const std::string& Name, const std::vector<std::string>& Tokens)
{
    for (auto& Token : Tokens)
        if (Name.find(Token) != std::string::npos)
            return true;

    return false;
}

namespace BotItems
{
    const FBotConsumableEffect* GetConsumableEffect(const UFortItemDefinition* Definition)
    {
        auto Name = GetName(Definition);
        for (auto& Effect : FBotConsumableEffects::Table)
            if (Name.find(Effect.Token) != std::string::npos)
                return &Effect;

        return nullptr;
    }

    std::string GetName(const UFortItemDefinition* Definition)
    {
        if (!Definition)
            return {};

        std::string Name = Definition->Name.ToString().c_str();
        std::transform(Name.begin(), Name.end(), Name.begin(), [](unsigned char C) { return (char)tolower(C); });
        return Name;
    }

    const UFortItemDefinition* GetAmmoDefinition(const UFortItemDefinition* Definition)
    {
        if (!Definition || !Definition->IsA<UFortWeaponItemDefinition>())
            return nullptr;

        auto Ammo = ((UFortWorldItemDefinition*)Definition)->GetAmmoWorldItemDefinition_BP();
        if (!Ammo || Ammo == Definition || !Ammo->IsA<UFortAmmoItemDefinition>())
            return nullptr;

        return Ammo;
    }

    EBotItemCategory Classify(const UFortItemDefinition* Definition)
    {
        if (!Definition)
            return EBotItemCategory::Other;

        if (Definition->IsA<UFortResourceItemDefinition>())
            return EBotItemCategory::Resource;
        if (Definition->IsA<UFortAmmoItemDefinition>())
            return EBotItemCategory::Ammo;
        if (Definition->IsA<UFortBuildingItemDefinition>() || Definition->IsA<UFortEditToolItemDefinition>())
            return EBotItemCategory::Tool;
        if (Definition->IsA<UFortWeaponMeleeItemDefinition>())
            return EBotItemCategory::Melee;
        if (Definition->IsA<UFortGadgetItemDefinition>())
            return EBotItemCategory::Utility;

        if (Definition->IsA<UFortWeaponItemDefinition>())
        {
            // Guns load separate ammo; consumables and throwables do not.
            if (GetAmmoDefinition(Definition))
                return EBotItemCategory::Weapon;

            auto Name = GetName(Definition);
            if (ContainsAny(Name, FBotItemPriorities::ShieldTokens))
                return EBotItemCategory::Shield;
            if (ContainsAny(Name, FBotItemPriorities::HealingTokens))
                return EBotItemCategory::Healing;

            return EBotItemCategory::Utility;
        }

        return EBotItemCategory::Other;
    }

    EBotWeaponRange GetWeaponRange(const UFortItemDefinition* Definition)
    {
        auto Name = GetName(Definition);

        if (ContainsAny(Name, FBotItemPriorities::LongRangeTokens))
            return EBotWeaponRange::Long;
        if (ContainsAny(Name, FBotItemPriorities::CloseRangeTokens))
            return EBotWeaponRange::Close;

        return EBotWeaponRange::Medium;
    }

    int GetClipSize(const UFortItemDefinition* Definition)
    {
        if (!Definition || !Definition->IsA<UFortWeaponItemDefinition>())
            return 0;

        auto Stats = AFortInventory::GetStats((const UFortWeaponItemDefinition*)Definition);
        return Stats ? Stats->ClipSize : 0;
    }

    int GetRarity(const UFortItemDefinition* Definition)
    {
        if (!Definition)
            return 0;

        if (Definition->HasRarity())
            return Definition->Rarity;

        return Definition->HasTier() ? Definition->Tier : 0;
    }

    bool UsesPrimarySlot(const UFortItemDefinition* Definition)
    {
        return Definition && AFortInventory::IsPrimaryQuickbar(Definition);
    }

    const char* ToString(EBotItemCategory Category)
    {
        switch (Category)
        {
        case EBotItemCategory::Weapon:
            return "Weapon";
        case EBotItemCategory::Melee:
            return "Melee";
        case EBotItemCategory::Ammo:
            return "Ammo";
        case EBotItemCategory::Healing:
            return "Healing";
        case EBotItemCategory::Shield:
            return "Shield";
        case EBotItemCategory::Utility:
            return "Utility";
        case EBotItemCategory::Resource:
            return "Resource";
        case EBotItemCategory::Tool:
            return "Tool";
        case EBotItemCategory::Other:
            return "Other";
        }
        return "Unknown";
    }
}
