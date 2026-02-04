#include "recording_indicator.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

RecordingIndicator::RecordingIndicator() : m_hWnd(NULL), m_isPaused(false), m_region{0} {
}

RecordingIndicator::~RecordingIndicator() {
    Hide();
}

void RecordingIndicator::Show(const RECT& region) {
    m_region = region;
    
    // Register window class
    static bool registered = false;
    if (!registered) {
        WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"SekoRecordingIndicator";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassEx(&wc);
        registered = true;
    }
    
    // Create layered window for transparency
    m_hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"SekoRecordingIndicator",
        L"",
        WS_POPUP,
        region.left, region.top,
        region.right - region.left, region.bottom - region.top,
        NULL, NULL, GetModuleHandle(NULL), this
    );
    
    if (m_hWnd) {
        // Make window click-through
        SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA);
        ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hWnd);
        
        // Force redraw
        InvalidateRect(m_hWnd, NULL, TRUE);
    }
}

void RecordingIndicator::Hide() {
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

void RecordingIndicator::UpdateState(bool isPaused) {
    m_isPaused = isPaused;
    if (m_hWnd) {
        InvalidateRect(m_hWnd, NULL, TRUE);
    }
}

LRESULT CALLBACK RecordingIndicator::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RecordingIndicator* pThis = NULL;
    
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (RecordingIndicator*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (RecordingIndicator*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            
            // Draw red border (or yellow if paused)
            Color borderColor = (pThis && pThis->m_isPaused) ? 
                Color(255, 255, 200, 0) : Color(255, 255, 50, 50);
            Pen borderPen(borderColor, 4);
            graphics.DrawRectangle(&borderPen, 2, 2, rc.right - 4, rc.bottom - 4);
            
            // Draw REC indicator in top-left
            if (pThis && !pThis->m_isPaused) {
                SolidBrush recBrush(Color(255, 255, 50, 50));
                graphics.FillEllipse(&recBrush, 10, 10, 12, 12);
                
                Font font(L"Segoe UI", 10, FontStyleBold);
                SolidBrush textBrush(Color(255, 255, 255, 255));
                graphics.DrawString(L"REC", -1, &font, PointF(28, 8), &textBrush);
            } else if (pThis && pThis->m_isPaused) {
                Font font(L"Segoe UI", 10, FontStyleBold);
                SolidBrush textBrush(Color(255, 255, 200, 0));
                graphics.DrawString(L"PAUSED", -1, &font, PointF(10, 8), &textBrush);
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}
