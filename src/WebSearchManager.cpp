#include "WebSearchManager.h"
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "winhttp.lib")

// Simple URL Encoder
static std::string UrlEncode(const std::string &value) {
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

std::string WebSearchManager::SearchBrave(const std::string& query) {
    std::string encodedQuery = UrlEncode(query);
    std::wstring wPath = L"/search?q=" + std::wstring(encodedQuery.begin(), encodedQuery.end());
    
    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", 
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, L"search.brave.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Send Request
    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                       WINHTTP_NO_REQUEST_DATA, 0,
                                       0, 0);

    std::string htmlResponse = "";
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;
            if (dwSize == 0)
                break;

            std::vector<char> buffer(dwSize + 1, 0);
            if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                htmlResponse.append(buffer.data(), dwDownloaded);
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (htmlResponse.empty()) return "";

    std::string snippets = "";
    // Search for Brave's typical description blocks
    std::string marker = "desktop-default-regular t-primary";
    size_t pos = 0;
    int count = 0;
    
    while ((pos = htmlResponse.find(marker, pos)) != std::string::npos && count < 3) {
        // Find the end of the div tag containing the marker
        size_t tagEnd = htmlResponse.find('>', pos);
        if (tagEnd == std::string::npos) break;
        
        size_t nextDivEnd = htmlResponse.find("</div>", tagEnd);
        if (nextDivEnd != std::string::npos) {
            std::string content = htmlResponse.substr(tagEnd + 1, nextDivEnd - tagEnd - 1);
            
            // Strip HTML tags roughly
            std::string clean = "";
            bool inTag = false;
            for(char c : content) {
                if(c == '<') inTag = true;
                else if(c == '>') inTag = false;
                else if(!inTag) clean += c;
            }
            
            // Remove lingering HTML entities roughly
            size_t entityPos = 0;
            while((entityPos = clean.find("&quot;")) != std::string::npos) clean.replace(entityPos, 6, "\"");
            while((entityPos = clean.find("&#39;")) != std::string::npos) clean.replace(entityPos, 5, "'");
            while((entityPos = clean.find("&amp;")) != std::string::npos) clean.replace(entityPos, 5, "&");
            while((entityPos = clean.find("&lt;")) != std::string::npos) clean.replace(entityPos, 4, "<");
            while((entityPos = clean.find("&gt;")) != std::string::npos) clean.replace(entityPos, 4, ">");
            
            if (clean.length() > 20) {
                snippets += "- " + clean + "\n";
                count++;
            }
        }
        pos = nextDivEnd;
    }

    return snippets;
}
