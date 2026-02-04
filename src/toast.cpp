#include <windows.h>
#include <string>
#include "toast.h"

static HWND g_hwndToast = NULL;
static std::wstring g_toastUrl;
static std::wstring g_toastMessage;
static bool g_isUploading = false;
static UINT_PTR g_timerId = 0;

#define TOAST_WIDTH 400
#define TOAST_HEIGHT 100
#define TOAST_MARGIN 20
#define TOAST_TIMER_ID 1

LRESULT CALLBACK ToastWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ShowUploadingToast() {
    g_toastMessage = L"Uploading...";
    g_toastUrl = L"";
    g_isUploading = true;
    
    // Register toast window class
    static bool registered = false;
    if (!registered) {
        WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
        wc.lpfnWndProc = ToastWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"SekoToastClass";
        wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
        RegisterClassEx(&wc);
        registered = true;
    }
    
    // Close existing toast if any
    if (g_hwndToast && IsWindow(g_hwndToast)) {
        DestroyWindow(g_hwndToast);
    }
    
    // Get screen dimensions
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    // Position at bottom-right
    int x = screenW - TOAST_WIDTH - TOAST_MARGIN;
    int y = screenH - TOAST_HEIGHT - TOAST_MARGIN - 40;
    
    // Create toast window
    g_hwndToast = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"SekoToastClass",
        L"",
        WS_POPUP,
        x, y, TOAST_WIDTH, TOAST_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    SetLayeredWindowAttributes(g_hwndToast, 0, 250, LWA_ALPHA);
    ShowWindow(g_hwndToast, SW_SHOWNOACTIVATE);
    UpdateWindow(g_hwndToast);
}

void UpdateToastWithUrl(const std::wstring& url) {
    g_toastUrl = url;
    g_toastMessage = L"Uploaded - Link copied to clipboard";
    g_isUploading = false;
    
    if (g_hwndToast && IsWindow(g_hwndToast)) {
        // Create copy button
        HWND hBtn = CreateWindow(L"BUTTON", L"Copy",
            WS_CHILD | WS_VISIBLE | BS_FLAT,
            TOAST_WIDTH - 90, TOAST_HEIGHT - 40,
            70, 30,
            g_hwndToast, (HMENU)1, GetModuleHandle(NULL), NULL);
        
        HFONT hFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Redraw
        InvalidateRect(g_hwndToast, NULL, TRUE);
        
        // Set auto-dismiss timer
        g_timerId = SetTimer(g_hwndToast, TOAST_TIMER_ID, 5000, NULL);
    }
}

void CloseToast() {
    if (g_hwndToast && IsWindow(g_hwndToast)) {
        DestroyWindow(g_hwndToast);
    }
}

void ShowToastNotification(const std::wstring& url) {
    ShowUploadingToast();
    UpdateToastWithUrl(url);
}

LRESULT CALLBACK ToastWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORBTN: {
            HDC hdcButton = (HDC)wParam;
            SetTextColor(hdcButton, RGB(255, 255, 255));
            SetBkColor(hdcButton, RGB(80, 80, 80));
            return (LRESULT)CreateSolidBrush(RGB(80, 80, 80));
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // White background
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            
            // Light gray border
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, rc.right, rc.bottom);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);
            
            SetBkMode(hdc, TRANSPARENT);
            
            // Title - LARGER for uploading
            if (g_isUploading) {
                SetTextColor(hdc, RGB(40, 40, 40));
                HFONT hFont = CreateFont(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                RECT titleRect = {15, 20, rc.right - 10, 45};
                DrawText(hdc, g_toastMessage.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                
                // Subtitle
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
                
                hFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                SelectObject(hdc, hFont);
                SetTextColor(hdc, RGB(120, 120, 120));
                
                RECT urlRect = {15, 50, rc.right - 10, 70};
                DrawText(hdc, L"Please wait...", -1, &urlRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
            } else {
                // Success state
                SetTextColor(hdc, RGB(50, 50, 50));
                HFONT hFont = CreateFont(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                RECT titleRect = {15, 12, rc.right - 10, 28};
                DrawText(hdc, g_toastMessage.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                
                // URL
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
                
                hFont = CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                SelectObject(hdc, hFont);
                SetTextColor(hdc, RGB(100, 100, 100));
                
                RECT urlRect = {15, 35, rc.right - 100, 55};
                if (!g_toastUrl.empty()) {
                    DrawText(hdc, g_toastUrl.c_str(), -1, &urlRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                }
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                // Copy button clicked
                if (OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    size_t size = (g_toastUrl.length() + 1) * sizeof(wchar_t);
                    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, size);
                    if (h) {
                        memcpy(GlobalLock(h), g_toastUrl.c_str(), size);
                        GlobalUnlock(h);
                        SetClipboardData(CF_UNICODETEXT, h);
                    }
                    CloseClipboard();
                }
                
                // Close toast
                if (g_timerId) {
                    KillTimer(hwnd, g_timerId);
                    g_timerId = 0;
                }
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_TIMER:
            if (wParam == TOAST_TIMER_ID) {
                KillTimer(hwnd, g_timerId);
                g_timerId = 0;
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_LBUTTONDOWN:
            if (!g_isUploading) {
                // Click anywhere to dismiss (only when not uploading)
                if (g_timerId) {
                    KillTimer(hwnd, g_timerId);
                    g_timerId = 0;
                }
                DestroyWindow(hwnd);
            }
            return 0;
        
        case WM_DESTROY:
            g_hwndToast = NULL;
            g_isUploading = false;
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
