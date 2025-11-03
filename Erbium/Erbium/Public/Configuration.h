#pragma once

struct FConfiguration
{
    static inline auto Playlist = L"/DurianPlaylist/Playlist/Playlist_Durian.Playlist_Durian";
    static inline auto MaxTickRate = 30.f;
    static inline auto bLateGame = false;
    static inline auto bEnableCheats = true;
    static inline auto SiphonAmount = 50; // set to 0 to disable
    static inline auto bInfiniteMats = false;
    static inline auto bInfiniteAmmo = false;
    static inline auto bForceRespawns = false;
    static inline auto bAutoRestart = false;
    static inline constexpr auto bEnableIris = true; 
    static inline constexpr auto bCreative = false;
    static inline constexpr auto bGUI = false;
    static inline constexpr auto bCustomCrashReporter = true;
};
