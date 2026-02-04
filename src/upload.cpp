#include <windows.h>
#include <winhttp.h>
#include <vector>
#include <string>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

std::wstring UploadToNekoo(const std::vector<BYTE>& imageData, const std::wstring& apiKey) {
    HINTERNET hSession = WinHttpOpen(L"Nekoo Screenshot/3.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return L"";
    
    HINTERNET hConnect = WinHttpConnect(hSession, L"nekoo.ru",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return L"";
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/screenshot/upload",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    
    // Create multipart form data boundary
    std::string boundary = "----NekooScreenshotBoundary";
    
    // Build multipart body
    std::ostringstream body;
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.png\"\r\n";
    body << "Content-Type: image/png\r\n\r\n";
    body.write((const char*)imageData.data(), imageData.size());
    body << "\r\n--" << boundary << "--\r\n";
    
    std::string bodyStr = body.str();
    
    // Set content type header
    std::wstring contentType = L"Content-Type: multipart/form-data; boundary=" + 
        std::wstring(boundary.begin(), boundary.end());
    
    WinHttpAddRequestHeaders(hRequest,
        contentType.c_str(),
        (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    
    // Add Authorization header if API key is provided
    if (!apiKey.empty()) {
        std::wstring authHeader = L"Authorization: Bearer " + apiKey;
        WinHttpAddRequestHeaders(hRequest,
            authHeader.c_str(),
            (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    }
    
    // Send request
    BOOL result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)bodyStr.c_str(), (DWORD)bodyStr.length(),
        (DWORD)bodyStr.length(), 0);
    
    std::wstring url;
    
    if (result) {
        result = WinHttpReceiveResponse(hRequest, NULL);
        
        if (result) {
            // Read response
            std::string response;
            DWORD dwSize;
            
            while (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                std::vector<char> buffer(dwSize + 1);
                DWORD dwDownloaded;
                
                if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                    buffer[dwDownloaded] = '\0';
                    response += buffer.data();
                }
            }
            
            // Parse JSON response to extract URL
            // Looking for: "url": "https://cdn.nekoo.ru/..."
            size_t urlPos = response.find("\"url\":");
            if (urlPos != std::string::npos) {
                size_t urlStart = response.find("\"", urlPos + 6);
                if (urlStart != std::string::npos) {
                    urlStart++; // Skip opening quote
                    size_t urlEnd = response.find("\"", urlStart);
                    if (urlEnd != std::string::npos) {
                        std::string urlStr = response.substr(urlStart, urlEnd - urlStart);
                        url = std::wstring(urlStr.begin(), urlStr.end());
                        
                        // Copy URL to clipboard
                        if (OpenClipboard(NULL)) {
                            EmptyClipboard();
                            size_t len = (url.length() + 1) * sizeof(wchar_t);
                            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
                            if (hMem) {
                                memcpy(GlobalLock(hMem), url.c_str(), len);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            }
                            CloseClipboard();
                        }
                    }
                }
            }
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return url;
}
