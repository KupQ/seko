#include "settings.h"
#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

#define REGISTRY_KEY L"Software\\Seko\\Screenshot"
#define AUTOSTART_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTOSTART_VALUE L"Seko"

AppSettings LoadSettings() {
    AppSettings settings;
    
    // Default values
    settings.fullscreenModifiers = MOD_CONTROL | MOD_SHIFT;
    settings.fullscreenKey = 'S';
    settings.regionModifiers = MOD_CONTROL | MOD_SHIFT;
    settings.regionKey = 'R';
    settings.autoStart = false;
    settings.apiKey = L"";
    
    // Video defaults
    wchar_t documentsPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, documentsPath);
    settings.videoSavePath = std::wstring(documentsPath) + L"\\Seko Videos";
    settings.videoFPS = 60;
    settings.videoBitrate = 8000000; // 8 Mbps
    settings.videoAutoUpload = false;
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue;
        DWORD dwSize = sizeof(DWORD);
        
        if (RegQueryValueEx(hKey, L"FullscreenModifiers", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.fullscreenModifiers = dwValue;
        }
        if (RegQueryValueEx(hKey, L"FullscreenKey", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.fullscreenKey = dwValue;
        }
        if (RegQueryValueEx(hKey, L"RegionModifiers", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.regionModifiers = dwValue;
        }
        if (RegQueryValueEx(hKey, L"RegionKey", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.regionKey = dwValue;
        }
        
        // Load API key
        wchar_t apiKeyBuffer[256] = {0};
        dwSize = sizeof(apiKeyBuffer);
        if (RegQueryValueEx(hKey, L"ApiKey", NULL, NULL, (LPBYTE)apiKeyBuffer, &dwSize) == ERROR_SUCCESS) {
            settings.apiKey = apiKeyBuffer;
        }
        
        // Load video settings
        wchar_t videoPathBuffer[MAX_PATH] = {0};
        dwSize = sizeof(videoPathBuffer);
        if (RegQueryValueEx(hKey, L"VideoSavePath", NULL, NULL, (LPBYTE)videoPathBuffer, &dwSize) == ERROR_SUCCESS) {
            settings.videoSavePath = videoPathBuffer;
        }
        
        if (RegQueryValueEx(hKey, L"VideoFPS", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.videoFPS = dwValue;
        }
        
        if (RegQueryValueEx(hKey, L"VideoBitrate", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.videoBitrate = dwValue;
        }
        
        if (RegQueryValueEx(hKey, L"VideoAutoUpload", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            settings.videoAutoUpload = (dwValue != 0);
        }
        
        RegCloseKey(hKey);
    }
    
    settings.autoStart = IsAutoStartEnabled();
    
    return settings;
}

void SaveSettings(const AppSettings& settings) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD dwValue;
        
        dwValue = settings.fullscreenModifiers;
        RegSetValueEx(hKey, L"FullscreenModifiers", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = settings.fullscreenKey;
        RegSetValueEx(hKey, L"FullscreenKey", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = settings.regionModifiers;
        RegSetValueEx(hKey, L"RegionModifiers", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = settings.regionKey;
        RegSetValueEx(hKey, L"RegionKey", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        // Save API key
        RegSetValueEx(hKey, L"ApiKey", 0, REG_SZ, (LPBYTE)settings.apiKey.c_str(), 
                     (DWORD)((settings.apiKey.length() + 1) * sizeof(wchar_t)));
        
        // Save video settings
        RegSetValueEx(hKey, L"VideoSavePath", 0, REG_SZ, (LPBYTE)settings.videoSavePath.c_str(),
                     (DWORD)((settings.videoSavePath.length() + 1) * sizeof(wchar_t)));
        
        dwValue = settings.videoFPS;
        RegSetValueEx(hKey, L"VideoFPS", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = settings.videoBitrate;
        RegSetValueEx(hKey, L"VideoBitrate", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = settings.videoAutoUpload ? 1 : 0;
        RegSetValueEx(hKey, L"VideoAutoUpload", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
    
    SetAutoStart(settings.autoStart);
}

bool IsAutoStartEnabled() {
    HKEY hKey;
    bool enabled = false;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH];
        DWORD dwSize = sizeof(path);
        
        if (RegQueryValueEx(hKey, AUTOSTART_VALUE, NULL, NULL, (LPBYTE)path, &dwSize) == ERROR_SUCCESS) {
            enabled = true;
        }
        
        RegCloseKey(hKey);
    }
    
    return enabled;
}

void SetAutoStart(bool enable) {
    HKEY hKey;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileName(NULL, exePath, MAX_PATH);
            
            RegSetValueEx(hKey, AUTOSTART_VALUE, 0, REG_SZ, (LPBYTE)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValue(hKey, AUTOSTART_VALUE);
        }
        
        RegCloseKey(hKey);
    }
}

// Validate API key with nekoo server
bool ValidateApiKey(const std::wstring& apiKey, std::wstring& username) {
    if (apiKey.empty()) {
        return false;
    }
    
    // Convert API key to UTF-8 for JSON
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, apiKey.c_str(), -1, NULL, 0, NULL, NULL);
    std::string apiKeyUtf8(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, apiKey.c_str(), -1, &apiKeyUtf8[0], sizeNeeded, NULL, NULL);
    apiKeyUtf8.pop_back(); // Remove null terminator
    
    // Create JSON payload
    std::string jsonPayload = "{\"api_key\":\"" + apiKeyUtf8 + "\"}";
    
    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(L"Seko/1.0", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return false;
    
    // Connect to server
    HINTERNET hConnect = WinHttpConnect(hSession, L"nekoo.ru", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Create request
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/auth/validate-key",
        NULL, WINHTTP_NO_REFERER, 
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Set headers (including no-cache to prevent stale responses)
    std::wstring headers = L"Content-Type: application/json\r\nCache-Control: no-cache\r\nPragma: no-cache\r\n";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    
    // Send request
    bool result = false;
    BOOL sendResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)jsonPayload.c_str(), jsonPayload.length(),
        jsonPayload.length(), 0);
    
    if (!sendResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    BOOL receiveResult = WinHttpReceiveResponse(hRequest, NULL);
    if (!receiveResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    if (sendResult && receiveResult) {
        
        // Read response
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::string response;
        
        do {
            dwSize = 0;
            WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (dwSize == 0) break;
            
            char* pszOutBuffer = new char[dwSize + 1];
            ZeroMemory(pszOutBuffer, dwSize + 1);
            
            if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                response += std::string(pszOutBuffer, dwDownloaded);
            }
            delete[] pszOutBuffer;
        } while (dwSize > 0);
        
        // Parse JSON response (check for "valid": true with or without space)
        if (response.find("\"valid\":true") != std::string::npos || 
            response.find("\"valid\": true") != std::string::npos) {
            result = true;
            
            // Extract username from response
            size_t userPos = response.find("\"username\":");
            if (userPos != std::string::npos) {
                userPos += 11; // Length of "username":
                // Skip any whitespace and quotes
                while (userPos < response.length() && (response[userPos] == ' ' || response[userPos] == '"')) {
                    userPos++;
                }
                size_t endPos = response.find("\"", userPos);
                if (endPos != std::string::npos) {
                    std::string usernameUtf8 = response.substr(userPos, endPos - userPos);
                    int wideSize = MultiByteToWideChar(CP_UTF8, 0, usernameUtf8.c_str(), -1, NULL, 0);
                    username.resize(wideSize - 1);
                    MultiByteToWideChar(CP_UTF8, 0, usernameUtf8.c_str(), -1, &username[0], wideSize);
                }
            }
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}
