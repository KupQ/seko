#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <stdio.h>
#include "overlay.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static HWND g_hwndOverlay = NULL;
static POINT g_ptStart = {0};
static POINT g_ptEnd = {0};
static bool g_bSelecting = false;
static bool g_bDragging = false;
static RECT g_selectedRect = {0};
static bool g_bCancelled = false;
static bool g_bDone = false;
static CaptureMode g_captureMode = CaptureMode::Screenshot;

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ShowOverlay(HWND hwndParent, RECT* pRect, CaptureMode mode) {
    // Register overlay window class
    static bool registered = false;
    if (!registered) {
        WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
        wc.lpfnWndProc = OverlayWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"SekoOverlayClass";
        wc.hCursor = LoadCursor(NULL, IDC_CROSS);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        RegisterClassEx(&wc);
        registered = true;
    }
    
    // Reset state
    // Reset state
    g_captureMode = mode;
    g_bSelecting = false;
    g_bDragging = false;
    g_bCancelled = false;
    g_bDone = false;
    g_ptStart = {0};
    g_ptEnd = {0};
    
    // Get screen dimensions
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    
    // Create fullscreen overlay
    g_hwndOverlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"SekoOverlayClass", L"",
        WS_POPUP,
        0, 0, w, h,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    // Make it semi-transparent
    SetLayeredWindowAttributes(g_hwndOverlay, 0, 100, LWA_ALPHA);
    
    ShowWindow(g_hwndOverlay, SW_SHOW);
    UpdateWindow(g_hwndOverlay);
    SetForegroundWindow(g_hwndOverlay);
    SetCapture(g_hwndOverlay);
    
    // Simple message loop
    MSG msg;
    while (!g_bDone && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_bCancelled) {
        pRect->left = pRect->top = pRect->right = pRect->bottom = 0;
    } else {
        *pRect = g_selectedRect;
    }
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN: {
            g_bDragging = true;
            g_bSelecting = true;
            g_ptStart.x = GET_X_LPARAM(lParam);
            g_ptStart.y = GET_Y_LPARAM(lParam);
            g_ptEnd = g_ptStart;
            SetCapture(hwnd);
            return 0;
        }
            
        case WM_MOUSEMOVE:
            if (g_bDragging) {
                g_ptEnd.x = GET_X_LPARAM(lParam);
                g_ptEnd.y = GET_Y_LPARAM(lParam);
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            return 0;
            
        case WM_LBUTTONUP:
            if (g_bDragging) {
                g_ptEnd.x = GET_X_LPARAM(lParam);
                g_ptEnd.y = GET_Y_LPARAM(lParam);
                
                // Calculate selected rectangle
                g_selectedRect.left = min(g_ptStart.x, g_ptEnd.x);
                g_selectedRect.top = min(g_ptStart.y, g_ptEnd.y);
                g_selectedRect.right = max(g_ptStart.x, g_ptEnd.x);
                g_selectedRect.bottom = max(g_ptStart.y, g_ptEnd.y);
                
                ReleaseCapture();
                g_bDone = true;
                DestroyWindow(hwnd);
            }
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                g_bCancelled = true;
                g_bDone = true;
                ReleaseCapture();
                DestroyWindow(hwnd);
            }
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            
            // Draw instruction text at top (only when not dragging)
            if (!g_bDragging) {
                Font instructFont(L"Segoe UI", 14, FontStyleBold);
                SolidBrush instructTextBrush(Color(255, 255, 255, 255));
                SolidBrush instructBgBrush(Color(200, 0, 0, 0));
                
                const wchar_t* instructText = (g_captureMode == CaptureMode::Video) ? 
                    L"Click and drag to record video (ESC to cancel)" : 
                    L"Click and drag to select region (ESC to cancel)";
                    
                RectF instructRect;
                graphics.MeasureString(instructText, -1, &instructFont, PointF(0, 0), &instructRect);
                
                // Center horizontally at top of screen
                float instructX = (rc.right - instructRect.Width - 40.0f) / 2.0f;
                float instructY = (rc.bottom / 2.0f) - 15.0f;
                
                // Draw rounded background
                
                // Load and draw icon
                HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPICON));
                if (hIcon) {
                    DrawIconEx(hdc, (int)(instructX + 10.0f), (int)(instructY - 2.0f), hIcon, 24, 24, 0, NULL, DI_NORMAL);
                }
                
                // Draw text
                graphics.DrawString(instructText, -1, &instructFont, PointF(instructX + 45.0f, instructY), &instructTextBrush);
            }

            // Draw dark overlay
            SolidBrush darkBrush(Color(100, 0, 0, 0));
            graphics.FillRectangle(&darkBrush, 0, 0, rc.right, rc.bottom);
            
            // Draw selection rectangle
            if (g_bSelecting && g_bDragging) {
                int x = min(g_ptStart.x, g_ptEnd.x);
                int y = min(g_ptStart.y, g_ptEnd.y);
                int w = abs(g_ptEnd.x - g_ptStart.x);
                int h = abs(g_ptEnd.y - g_ptStart.y);
                
                if (w > 0 && h > 0) {
                    // Clear selected area (make it transparent)
                    graphics.SetCompositingMode(CompositingModeSourceCopy);
                    SolidBrush clearBrush(Color(0, 0, 0, 0));
                    graphics.FillRectangle(&clearBrush, x, y, w, h);
                    
                    // Draw border (Purple for Screenshot, Red for Video)
                    graphics.SetCompositingMode(CompositingModeSourceOver);
                    Color borderColor = (g_captureMode == CaptureMode::Video) ? 
                        Color(255, 255, 50, 50) : Color(255, 139, 92, 246);
                    Pen borderPen(borderColor, 2);
                    graphics.DrawRectangle(&borderPen, x, y, w, h);
                    
                    // Draw dimensions
                    wchar_t text[64];
                    _snwprintf_s(text, _TRUNCATE, L"%d x %d", w, h);
                    
                    Font font(L"Segoe UI", 12);
                    SolidBrush textBrush(Color(255, 255, 255, 255));
                    SolidBrush bgBrush(Color(200, 0, 0, 0));
                    
                    RectF textRect;
                    graphics.MeasureString(text, -1, &font, PointF(0, 0), &textRect);
                    graphics.FillRectangle(&bgBrush, (float)x, (float)y - 25, textRect.Width + 10.0f, 20.0f);
                    graphics.DrawString(text, -1, &font, PointF((float)x + 5, (float)y - 22), &textBrush);
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            g_hwndOverlay = NULL;
            g_bDone = true;
            // Don't call PostQuitMessage - it exits the entire app!
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
