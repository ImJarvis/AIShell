#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>

class WebSearchManager {
public:
    // Called once at app startup (alongside model load).
    // Opens a persistent WinHTTP session + TLS connection to search.brave.com.
    // Returns true if internet is reachable, false otherwise.
    static bool Initialize();

    // Called at app shutdown — closes the persistent session handles cleanly.
    static void Shutdown();

    // Performs a search on search.brave.com using the persistent session.
    // Returns top text snippets, or empty string if unreachable.
    static std::string SearchBrave(const std::string& query);

private:
    static HINTERNET s_Session;
    static HINTERNET s_Connect;
};
