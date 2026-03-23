#include "WebSearchManager.h"
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "winhttp.lib")

// --- Static handle definitions ---
HINTERNET WebSearchManager::s_Session = NULL;
HINTERNET WebSearchManager::s_Connect = NULL;

// Simple URL Encoder
static std::string UrlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}

bool WebSearchManager::Initialize() {
    // Shutdown any existing session first (idempotent)
    Shutdown();

    s_Session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!s_Session) return false;

    // Apply session-level timeouts — propagated to all requests on this session
    DWORD timeout = 5000;
    WinHttpSetOption(s_Session, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(s_Session, WINHTTP_OPTION_SEND_TIMEOUT,    &timeout, sizeof(timeout));
    WinHttpSetOption(s_Session, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    s_Connect = WinHttpConnect(s_Session, L"search.brave.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!s_Connect) {
        WinHttpCloseHandle(s_Session);
        s_Session = NULL;
        return false;
    }

    // Connectivity ping: lightweight HEAD request to verify live internet access
    HINTERNET hPing = WinHttpOpenRequest(s_Connect, L"HEAD", L"/",
                                         NULL, WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         WINHTTP_FLAG_SECURE);
    if (!hPing) return false;

    BOOL ok = WinHttpSendRequest(hPing, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(hPing, NULL);
    WinHttpCloseHandle(hPing);

    if (!ok) {
        // Reachable session but ping failed — close and report offline
        Shutdown();
        return false;
    }

    return true; // Session warm, internet confirmed
}

void WebSearchManager::Shutdown() {
    if (s_Connect) { WinHttpCloseHandle(s_Connect); s_Connect = NULL; }
    if (s_Session) { WinHttpCloseHandle(s_Session); s_Session = NULL; }
}

std::string WebSearchManager::SearchBrave(const std::string& query) {
    // Guard: session must be initialized
    if (!s_Connect) return "";

    std::string encodedQuery = UrlEncode(query);
    std::wstring wPath = L"/search?q=" + std::wstring(encodedQuery.begin(), encodedQuery.end());

    // Reuse the persistent connection — no TLS handshake cost
    HINTERNET hRequest = WinHttpOpenRequest(s_Connect, L"GET", wPath.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) return "";

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                       WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    std::string htmlResponse;
    if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (bResults) {
        DWORD dwSize = 0, dwDownloaded = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;

            std::vector<char> buffer(dwSize + 1, 0);
            if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                htmlResponse.append(buffer.data(), dwDownloaded);
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);

    if (htmlResponse.empty()) return "";

    // Parse top 3 result snippets
    std::string snippets;
    std::string marker = "desktop-default-regular t-primary";
    size_t pos = 0;
    int count = 0;

    while ((pos = htmlResponse.find(marker, pos)) != std::string::npos && count < 3) {
        size_t tagEnd = htmlResponse.find('>', pos);
        if (tagEnd == std::string::npos) break;

        size_t nextDivEnd = htmlResponse.find("</div>", tagEnd);
        if (nextDivEnd != std::string::npos) {
            std::string content = htmlResponse.substr(tagEnd + 1, nextDivEnd - tagEnd - 1);

            // Strip HTML tags
            std::string clean;
            bool inTag = false;
            for (char c : content) {
                if (c == '<') inTag = true;
                else if (c == '>') inTag = false;
                else if (!inTag) clean += c;
            }

            // Decode common HTML entities
            auto replaceAll = [](std::string& s, const std::string& from, const std::string& to) {
                size_t p = 0;
                while ((p = s.find(from, p)) != std::string::npos) { s.replace(p, from.size(), to); p += to.size(); }
            };
            replaceAll(clean, "&quot;", "\"");
            replaceAll(clean, "&#39;",  "'");
            replaceAll(clean, "&amp;",  "&");
            replaceAll(clean, "&lt;",   "<");
            replaceAll(clean, "&gt;",   ">");

            if (clean.length() > 20) {
                snippets += "- " + clean + "\n";
                count++;
            }
        }
        pos = nextDivEnd;
    }

    return snippets;
}
