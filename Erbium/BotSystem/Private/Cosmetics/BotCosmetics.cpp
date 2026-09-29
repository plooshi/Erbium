#include "pch.h"
#include "../../Public/Cosmetics/BotCosmetics.h"
#include "../../Public/BotCompat.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include <atomic>

// --- Version / asset layer -------------------------------------------------------------------------
//
// Primary asset types and item classes per slot. These names are what the game's own profile/MCP
// data uses ("AthenaCharacter:CID_..."), unchanged across the supported range. Whether a build knows
// a given *cosmetic* is answered by the build itself (its asset registry), not by this table.

static const wchar_t* AssetTypeNames[(int)EBotCosmeticSlot::Count] = {
    L"AthenaCharacter", L"AthenaBackpack", L"AthenaPickaxe", L"AthenaGlider", L"AthenaSkyDiveContrail",
};

static const char* ItemClassNames[(int)EBotCosmeticSlot::Count] = {
    "AthenaCharacterItemDefinition", "AthenaBackpackItemDefinition", "AthenaPickaxeItemDefinition", "AthenaGliderItemDefinition", "AthenaSkyDiveContrailItemDefinition",
};

// FFortAthenaLoadout member for each slot (player-controller bots only).
static const char* LoadoutMemberNames[(int)EBotCosmeticSlot::Count] = {
    "Character", "Backpack", "Pickaxe", "Glider", "SkyDiveContrail",
};

// Engine FPrimaryAssetId: { FPrimaryAssetType { FName Name }, FName PrimaryAssetName }.
struct FBotPrimaryAssetId
{
    FName Type;
    FName Name;
};

// Large enough for a returned TSoftObjectPtr on every supported engine version (0x28 - 0x30 bytes).
struct FBotSoftObjectBuffer
{
    alignas(8) uint8 Data[0x40] = {};
};

static std::string ToLower(std::string Value)
{
    std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char C) { return (char)tolower(C); });
    return Value;
}

// Ids that exist in the registry but are not wearable player cosmetics (random-pick placeholders,
// NPC-only characters, test assets).
static bool IsExcludedId(const std::string& Id)
{
    auto Lower = ToLower(Id);
    return Lower.find("random") != std::string::npos || Lower.find("npc") != std::string::npos || Lower.find("test") != std::string::npos || Lower.find("tbd") != std::string::npos;
}

namespace
{
    struct FSlotCatalog
    {
        bool bBuilt = false;
        std::vector<FName> Ids;
        std::unordered_map<int32, TWeakObjectPtr<UObject>> Loaded; // keyed by FName comparison index
    };

    FSlotCatalog Catalogs[(int)EBotCosmeticSlot::Count];
    std::atomic<const char*> LastProblem = "none"; // string literals only (read by the GUI thread)
    std::atomic<int> KnownCounts[(int)EBotCosmeticSlot::Count] = {}; // published catalogue sizes (GUI)
    bool bSettingsLoaded = false;
    std::atomic<bool> bReloadRequested = false;
}

static void BuildCatalog(EBotCosmeticSlot Slot)
{
    auto& Catalog = Catalogs[(int)Slot];
    if (Catalog.bBuilt)
        return;

    Catalog.bBuilt = true;
    const char* Source = "asset registry";

    // 1. Everything the asset manager knows for this primary asset type.
    auto Kismet = UKismetSystemLibrary::GetDefaultObj();
    static auto GetPrimaryAssetIdList = Kismet->GetFunction("GetPrimaryAssetIdList");

    if (GetPrimaryAssetIdList)
    {
        TArray<FBotPrimaryAssetId> List;
        FName Type = FName(AssetTypeNames[(int)Slot]);
        Kismet->Call<void>(GetPrimaryAssetIdList, Type, &List);

        for (int i = 0; i < List.Num(); i++)
            if (!IsExcludedId(List[i].Name.ToString().c_str()))
                Catalog.Ids.push_back(List[i].Name);

        List.Free();
    }

    // 2. Fallback: cosmetics already loaded in memory (e.g. the asset manager does not scan this type).
    if (Catalog.Ids.empty())
    {
        Source = "loaded objects";

        if (auto ItemClass = FindClass(ItemClassNames[(int)Slot]))
            for (int32 i = 0; i < TUObjectArray::Num(); i++)
            {
                auto Object = TUObjectArray::GetObjectByIndex(i);
                if (!Object || Object->IsDefaultObject() || !Object->IsA(ItemClass))
                    continue;

                if (!IsExcludedId(Object->Name.ToString().c_str()))
                {
                    Catalog.Ids.push_back(Object->Name);
                    Catalog.Loaded[Object->Name.ComparisonIndex] = TWeakObjectPtr<UObject>((UObject*)Object);
                }
            }
    }

    KnownCounts[(int)Slot] = (int)Catalog.Ids.size();
    BOT_VERBOSE(BOT_CATEGORY_COSMETICS, "%s: %d available (%s).", BotCosmetics::ToString(Slot), (int)Catalog.Ids.size(), Source);
}

// Loads and validates one cosmetic. Returns nullptr if it does not exist or is not usable.
static UObject* LoadCosmetic(EBotCosmeticSlot Slot, FName Id)
{
    auto& Catalog = Catalogs[(int)Slot];

    if (auto Found = Catalog.Loaded.find(Id.ComparisonIndex); Found != Catalog.Loaded.end())
        if (auto Object = (UObject*)Found->second.Get())
            return Object;

    const UObject* Object = nullptr;
    auto Kismet = UKismetSystemLibrary::GetDefaultObj();
    FBotPrimaryAssetId AssetId{ FName(AssetTypeNames[(int)Slot]), Id };

    // Soft reference from the asset manager, then load it (Erbium's soft pointer resolves by path and
    // loads when needed).
    static auto GetSoftObjectReference = Kismet->GetFunction("GetSoftObjectReferenceFromPrimaryAssetId");
    if (GetSoftObjectReference)
    {
        auto Buffer = Kismet->Call<FBotSoftObjectBuffer>(GetSoftObjectReference, AssetId);
        Object = ((TSoftObjectPtr<UObject>*)&Buffer)->Get();
    }

    // Already-loaded objects only (older API).
    if (!Object)
        Object = UKismetSystemLibrary::GetObjectFromPrimaryAssetId(AssetId);

    if (!Object)
        return nullptr;

    static const UClass* ItemClasses[(int)EBotCosmeticSlot::Count] = {};
    auto& ItemClass = ItemClasses[(int)Slot];
    if (!ItemClass)
        ItemClass = FindClass(ItemClassNames[(int)Slot]);

    if (ItemClass && !Object->IsA(ItemClass))
        return nullptr;

    // An outfit without a hero definition has no parts to show.
    if (Slot == EBotCosmeticSlot::Outfit && !((UAthenaCharacterItemDefinition*)Object)->HeroDefinition)
        return nullptr;

    Catalog.Loaded[Id.ComparisonIndex] = TWeakObjectPtr<UObject>((UObject*)Object);
    return (UObject*)Object;
}

static bool TryAssign(FBotCosmeticLoadout& Loadout, EBotCosmeticSlot Slot, FName Id)
{
    auto Object = LoadCosmetic(Slot, Id);
    if (!Object)
        return false;

    Loadout.Items[(int)Slot] = TWeakObjectPtr<UObject>(Object);
    Loadout.Ids[(int)Slot] = Id.ToString().c_str();
    return true;
}

static bool ChooseRandom(FBotCosmeticLoadout& Loadout, EBotCosmeticSlot Slot)
{
    BuildCatalog(Slot);
    auto& Ids = Catalogs[(int)Slot].Ids;

    // A few attempts; ids that fail to load are dropped so they are never picked again.
    for (int Attempt = 0; Attempt < 6 && !Ids.empty(); Attempt++)
    {
        auto Index = (size_t)rand() % Ids.size();
        auto Id = Ids[Index];

        if (TryAssign(Loadout, Slot, Id))
            return true;

        BOT_VERBOSE(BOT_CATEGORY_COSMETICS, "%s %s is not usable, removing it from the pool.", BotCosmetics::ToString(Slot), Id.ToString().c_str());
        Ids[Index] = Ids.back();
        Ids.pop_back();
    }

    return false;
}

static void LoadSettingsFile()
{
    bSettingsLoaded = true;

    FILE* File = nullptr;
    if (_wfopen_s(&File, FBotCosmeticSettings::SettingsFile, L"r") != 0 || !File)
        return;

    char Line[4096];
    int Applied = 0;

    while (fgets(Line, sizeof(Line), File))
    {
        std::string Text = Line;
        if (auto Comment = Text.find('#'); Comment != std::string::npos)
            Text.resize(Comment);

        auto Equals = Text.find('=');
        if (Equals == std::string::npos)
            continue;

        auto Trim = [](std::string Value)
        {
            auto Begin = Value.find_first_not_of(" \t\r\n");
            auto End = Value.find_last_not_of(" \t\r\n");
            return Begin == std::string::npos ? std::string() : Value.substr(Begin, End - Begin + 1);
        };

        auto Key = ToLower(Trim(Text.substr(0, Equals)));
        auto Value = Trim(Text.substr(Equals + 1));

        int SlotIndex = -1;
        for (int i = 0; i < (int)EBotCosmeticSlot::Count; i++)
            if (Key == ToLower(BotCosmetics::ToString((EBotCosmeticSlot)i)))
                SlotIndex = i;

        if (SlotIndex == -1)
        {
            BOT_LOG(BOT_CATEGORY_COSMETICS, "Unknown slot '%s' in settings file.", Key.c_str());
            continue;
        }

        auto& Settings = FBotCosmeticSettings::Slots[SlotIndex];
        Settings.SpecificIds.clear();

        if (ToLower(Value) == "random")
            Settings.Mode = EBotCosmeticMode::Random;
        else if (ToLower(Value) == "none" || Value.empty())
            Settings.Mode = EBotCosmeticMode::None;
        else
        {
            Settings.Mode = EBotCosmeticMode::Specific;

            size_t Start = 0;
            while (Start <= Value.size())
            {
                auto Comma = Value.find(',', Start);
                auto Id = Trim(Value.substr(Start, Comma == std::string::npos ? std::string::npos : Comma - Start));
                if (!Id.empty())
                    Settings.SpecificIds.push_back(std::wstring(Id.begin(), Id.end()));

                if (Comma == std::string::npos)
                    break;
                Start = Comma + 1;
            }
        }

        Applied++;
    }

    fclose(File);
    BOT_VERBOSE(BOT_CATEGORY_COSMETICS, "Loaded %d slot setting(s) from settings file.", Applied);
}

std::string FBotCosmeticLoadout::Describe() const
{
    std::string Result;

    for (int i = 0; i < (int)EBotCosmeticSlot::Count; i++)
    {
        if (Ids[i].empty())
            continue;

        if (!Result.empty())
            Result += ", ";
        Result += Ids[i];
    }

    return Result.empty() ? "default" : Result;
}

namespace BotCosmetics
{
    const char* ToString(EBotCosmeticSlot Slot)
    {
        switch (Slot)
        {
        case EBotCosmeticSlot::Outfit:
            return "Outfit";
        case EBotCosmeticSlot::Backpack:
            return "Backpack";
        case EBotCosmeticSlot::Pickaxe:
            return "Pickaxe";
        case EBotCosmeticSlot::Glider:
            return "Glider";
        case EBotCosmeticSlot::Contrail:
            return "Contrail";
        default:
            return "Unknown";
        }
    }

    bool ChooseSpecific(FBotCosmeticLoadout& Loadout, EBotCosmeticSlot Slot, const std::wstring& Id)
    {
        BuildCatalog(Slot);

        if (TryAssign(Loadout, Slot, FName(Id.c_str())))
            return true;

        BOT_LOG(BOT_CATEGORY_COSMETICS, "%s '%ls' does not exist on this build or is not usable.", ToString(Slot), Id.c_str());
        return false;
    }

    FBotCosmeticLoadout ChooseLoadout()
    {
        FBotCosmeticLoadout Loadout;

        if (!FBotCosmeticSettings::bEnabled)
            return Loadout;

        if (bReloadRequested.exchange(false))
            Reload();
        else if (!bSettingsLoaded)
            LoadSettingsFile();

        for (int i = 0; i < (int)EBotCosmeticSlot::Count; i++)
        {
            auto Slot = (EBotCosmeticSlot)i;
            auto& Settings = FBotCosmeticSettings::Slots[i];

            if (Settings.Mode == EBotCosmeticMode::None)
                continue;

            if (Settings.Mode == EBotCosmeticMode::Specific && !Settings.SpecificIds.empty())
            {
                // Random order among the listed ids; first valid one wins.
                auto Candidates = Settings.SpecificIds;
                for (size_t n = Candidates.size(); n > 1; n--)
                    std::swap(Candidates[n - 1], Candidates[(size_t)rand() % n]);

                bool bAssigned = false;
                for (auto& Id : Candidates)
                    if ((bAssigned = ChooseSpecific(Loadout, Slot, Id)))
                        break;

                if (bAssigned)
                    continue;

                BOT_VERBOSE(BOT_CATEGORY_COSMETICS, "No listed %s is usable, picking a random one.", ToString(Slot));
            }

            ChooseRandom(Loadout, Slot);
        }

        return Loadout;
    }

    bool ApplyLoadout(FBotController& Bot, const FBotCosmeticLoadout& Loadout)
    {
        auto Pawn = Bot.GetPawn();
        auto PlayerState = Bot.GetPlayerState();
        if (!Pawn || !PlayerState)
            return false;

        // Player-controller bots: fill the controller's cosmetic loadout, which Erbium's existing code
        // reads (GiveStartingItems grants CosmeticLoadoutPC.Pickaxe; gliders come from it when skydiving).
        if (auto PlayerController = Bot.GetPlayerController(); PlayerController && PlayerController->HasCosmeticLoadoutPC())
        {
            auto& PCLoadout = PlayerController->CosmeticLoadoutPC;

            for (int i = 0; i < (int)EBotCosmeticSlot::Count; i++)
            {
                auto Item = Loadout.Get((EBotCosmeticSlot)i);
                if (!Item)
                    continue;

                auto Offset = FFortAthenaLoadout::StaticStruct()->GetOffset(LoadoutMemberNames[i]);
                if (Offset != -1)
                    *(UObject**)(__int64(&PCLoadout) + Offset) = Item;
            }
        }

        auto Character = (UAthenaCharacterItemDefinition*)Loadout.Get(EBotCosmeticSlot::Outfit);
        if (!Character)
        {
            LastProblem = !FBotCosmeticSettings::bEnabled                               ? "random cosmetics are turned off"
                          : FBotCosmeticSettings::Slots[(int)EBotCosmeticSlot::Outfit].Mode == EBotCosmeticMode::None ? "outfit slot is set to None"
                          : Catalogs[(int)EBotCosmeticSlot::Outfit].Ids.empty()           ? "no outfits found on this build"
                                                                                          : "no outfit could be loaded";
            return false;
        }

        // Same part resolution as Erbium's native-bot cosmetics (Misc.cpp InitializeCosmeticLoadout):
        // hero specializations give the body/head/..., the backpack adds its own parts.
        BotCompat::FCharacterParts Parts{};
        auto Hero = Character->HeroDefinition;

        for (auto& SoftSpecialization : Hero->Specializations)
        {
            auto Specialization = SoftSpecialization.Get();
            if (!Specialization)
                continue;

            for (auto& SoftPart : Specialization->CharacterParts)
                if (auto Part = SoftPart.Get())
                    if (Part->CharacterPartType < Parts.size())
                        Parts[Part->CharacterPartType] = Part;
        }

        if (auto Backpack = (UAthenaCharacterPartItemDefinition*)Loadout.Get(EBotCosmeticSlot::Backpack))
            for (auto& Part : Backpack->CharacterParts)
                if (Part && Part->CharacterPartType < Parts.size())
                    Parts[Part->CharacterPartType] = Part;

        // Nothing resolved (parts not loadable on this build): report failure so the caller applies the
        // default appearance instead of leaving the bot without any parts.
        size_t PartCount = 0;
        for (auto Part : Parts)
            PartCount += Part != nullptr;
        if (PartCount == 0)
        {
            LastProblem = "the outfit's character parts could not be loaded";
            BOT_LOG(BOT_CATEGORY_COSMETICS, "Bot #%u: outfit %s has no loadable character parts.", Bot.GetId(), Loadout.Ids[(int)EBotCosmeticSlot::Outfit].c_str());
            return false;
        }

        if (PlayerState->HasHeroType())
            PlayerState->HeroType = Hero;

        if (!BotCompat::ApplyCharacterParts(PlayerState, Pawn, Parts))
        {
            LastProblem = "this build has no way to set character parts";
            return false;
        }

        LastProblem = "none";

        BOT_VERBOSE(BOT_CATEGORY_COSMETICS, "Bot #%u wears %s", Bot.GetId(), Loadout.Describe().c_str());
        return true;
    }

    int GetAvailableCount(EBotCosmeticSlot Slot)
    {
        BuildCatalog(Slot);
        return (int)Catalogs[(int)Slot].Ids.size();
    }

    const char* GetLastProblem()
    {
        return LastProblem;
    }

    int GetKnownCount(EBotCosmeticSlot Slot)
    {
        return KnownCounts[(int)Slot];
    }

    void RequestReload()
    {
        bReloadRequested = true;
    }

    void Reload()
    {
        for (auto& Catalog : Catalogs)
            Catalog = FSlotCatalog{};

        LoadSettingsFile();
    }
}
