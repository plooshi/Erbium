#pragma once

struct FConfiguration
{
    /// playlits if u want to change it  then change the playlitspath OR commend and uncomment the 1 u want here
    //static inline auto Playlist = L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo"; // BR Solo
    //static inline auto Playlist = L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2"; // Creative
    static inline auto Playlist = L"/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo"; // Arena Solo


    static inline auto MaxTickRate = 30.f;
    static inline auto bLateGame = false;
    static inline auto LateGameZone = 4; // starting zone
    static inline auto bLateGameLongZone = true; // zone doesnt close for a long time
    static inline auto bEnableCheats = true;
    static inline auto SiphonAmount = 50; // set to 0 to disable
    static inline auto bInfiniteMats = true;
    static inline auto bInfiniteAmmo = true;
    static inline auto bForceRespawns = true; // build your client with this too!
    static inline auto bJoinInProgress = true;
    static inline auto bAutoRestart = false;
    static inline auto bKeepInventory = false;
    static inline auto Port = 7777;
    static inline constexpr auto bEnableIris = true; 
    static inline constexpr auto bGUI = true;
    static inline constexpr auto bCustomCrashReporter = true;
    static inline constexpr auto bUseStdoutLog = false;
    static inline constexpr auto WebhookURL = ""; // fill in if you want status to send to a webhook

    // creative stuff
    static inline auto PlotDefinition = L"/Game/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium";  // put any map / plot here (e.g. "/Game/Playgrounds/Items/Plots/TheBlock_Season7" The Block from S7)

};
