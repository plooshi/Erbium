#pragma once
#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <sstream>
#include <algorithm>
#include <locale>
#include <Windows.h>
#include "Erbium/Public/Configuration.h"

class DiscordWebhook {
public:
    DiscordWebhook(const char* webhook_url)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, webhook_url);
            headers = curl_slist_append(NULL, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        }
    }

    ~DiscordWebhook()
    {
        if (headers) {
            curl_slist_free_all(headers);
            headers = nullptr;
        }
        if (curl) {
            curl_easy_cleanup(curl);
            curl = nullptr;
        }
    }

    bool handleCode(CURLcode res)
    {
        if (res != CURLE_OK) {
            return false;
        }
        return true;
    }

    std::string escapeJson(const std::string& input) {
        std::string output;
        output.reserve(input.size());
        for (char c : input) {
            switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
            }
        }
        return output;
    }

    inline bool send_message(const std::string& message)
    {
        if (!curl) return false;

        std::string escapedMessage = escapeJson(message);
        std::string json = "{\"content\": \"" + escapedMessage + "\"}";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json.length());

        CURLcode res = curl_easy_perform(curl);
        bool success = handleCode(res);

        if (success) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            success = (response_code == 200 || response_code == 204);
        }

        return success;
    }

    inline bool send_embedjson(const std::string& ajson)
    {
        if (!curl) return false;

        std::string json = ajson.find("embeds") != std::string::npos ? ajson : "{\"embeds\": " + ajson + "}";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json.length());

        CURLcode res = curl_easy_perform(curl);
        bool success = handleCode(res);

        if (success) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            success = (response_code == 200 || response_code == 204);
        }

        return success;
    }

    inline bool send_embed(const std::string& title, const std::string& description, int color = 0)
    {
        if (!curl) return false;

        std::string escapedTitle = escapeJson(title);
        std::string escapedDesc = escapeJson(description);

        std::ostringstream json;
        json << "{\"embeds\": [{\"title\": \"" << escapedTitle
            << "\", \"description\": \"" << escapedDesc
            << "\", \"color\": " << color << "}]}";

        std::string jsonStr = json.str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonStr.length());

        CURLcode res = curl_easy_perform(curl);
        bool success = handleCode(res);

        if (success) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            success = (response_code == 200 || response_code == 204);
        }

        return success;
    }

    inline bool send_embed_with_thumbnail(const std::string& title, const std::string& description,
        const std::string& thumbnail_url, int color = 0)
    {
        if (!curl) return false;

        std::string escapedTitle = escapeJson(title);
        std::string escapedDesc = escapeJson(description);
        std::string escapedThumb = escapeJson(thumbnail_url);

        std::ostringstream json;
        json << "{\"embeds\": [{\"title\": \"" << escapedTitle
            << "\", \"description\": \"" << escapedDesc
            << "\", \"color\": " << color
            << ", \"thumbnail\": {\"url\": \"" << escapedThumb << "\"}"
            << "}]}";

        std::string jsonStr = json.str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonStr.length());

        CURLcode res = curl_easy_perform(curl);
        bool success = handleCode(res);

        if (success) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            success = (response_code == 200 || response_code == 204);
        }

        return success;
    }

private:
    CURL* curl;
    curl_slist* headers = nullptr;
};


namespace WebhookManager
{
    static DiscordWebhook* webhook = nullptr;
    static volatile bool initialized = false;

    inline std::string WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    inline std::string GetPlaylistId(const wchar_t* playlistPath)
    {
        if (!playlistPath) return "Playlist_DefaultSolo";

        std::wstring path = playlistPath;
        size_t lastSlash = path.find_last_of(L'/');
        if (lastSlash != std::wstring::npos)
        {
            std::wstring id = path.substr(lastSlash + 1);
            size_t dot = id.find(L'.');
            if (dot != std::wstring::npos)
            {
                id = id.substr(0, dot);
            }
            return WStringToString(id);
        }
        return "Playlist_DefaultSolo";
    }

    inline std::string ToLowerCase(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return str;
    }

    inline std::string GetPlaylistThumbnail(const std::string& playlistId)
    {
        std::string lowerPlaylistId = ToLowerCase(playlistId);
        return "https://fortnite-api.com/images/playlists/" + lowerPlaylistId + "/showcase.png";
    }

    inline std::string FormatPlaylistName(const std::string& playlistId)
    {
        std::string name = playlistId;

        if (name.find("Playlist_") == 0) {
            name = name.substr(9);
        }

        std::string formatted;
        for (size_t i = 0; i < name.length(); i++) {
            if (i > 0 && isupper(name[i]) && islower(name[i - 1])) {
                formatted += " ";
            }
            formatted += name[i];
        }

        return formatted;
    }

    inline DiscordWebhook* GetWebhook()
    {
        if (!initialized && FConfiguration::bSendWebhook)
        {
            if (FConfiguration::WebhookUrl && strlen(FConfiguration::WebhookUrl) > 0) {
                try {
                    webhook = new DiscordWebhook(FConfiguration::WebhookUrl);
                    initialized = true;
                }
                catch (...) {
                }
            }
        }

        return webhook;
    }

    inline void SendJoinable()
    {
        if (!FConfiguration::bSendWebhook) return;

        DiscordWebhook* wh = GetWebhook();
        if (!wh) return;

        try {
            std::string playlistId = GetPlaylistId(FConfiguration::Playlist);
            std::string playlistName = FormatPlaylistName(playlistId);
            std::string thumbnail = GetPlaylistThumbnail(playlistId);

            std::string description = ":green_circle: The server is now ready to join!\n\n**Playlist:** " + playlistName;

            wh->send_embed_with_thumbnail(
                "Server Joinable",
                description,
                thumbnail,
                65280
            );
        }
        catch (...) {
        }
    }

    inline void SendBattleBusLaunched()
    {
        if (!FConfiguration::bSendWebhook) return;

        DiscordWebhook* wh = GetWebhook();
        if (!wh) return;

        try {
            std::string playlistId = GetPlaylistId(FConfiguration::Playlist);
            std::string playlistName = FormatPlaylistName(playlistId);
            std::string thumbnail = GetPlaylistThumbnail(playlistId);

            std::string description = ":bus: The Battle Bus has been launched!\n\n**Playlist:** " + playlistName;

            wh->send_embed_with_thumbnail(
                "Battle Bus Launched",
                description,
                thumbnail,
                39423
            );
        }
        catch (...) {
        }
    }

    inline void SendMatchEnded()
    {
        if (!FConfiguration::bSendWebhook) return;

        DiscordWebhook* wh = GetWebhook();
        if (!wh) return;

        try {
            std::string playlistId = GetPlaylistId(FConfiguration::Playlist);
            std::string playlistName = FormatPlaylistName(playlistId);
            std::string thumbnail = GetPlaylistThumbnail(playlistId);

            std::string description = ":red_circle: The match has ended!\n\n**Playlist:** " + playlistName;

            wh->send_embed_with_thumbnail(
                "Match Ended",
                description,
                thumbnail,
                16711680
            );
        }
        catch (...) {
        }
    }
}
