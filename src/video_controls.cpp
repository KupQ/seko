#include "video_controls.h"
#include <string>

// Control IDs
#define ID_BTN_PAUSE 2001
#define ID_BTN_STOP  2002

VideoControls::VideoControls() 
    : m_hWnd(NULL), 
      m_hBtnPause(NULL), 
      m_hBtnStop(NULL), 
      m_isPaused(false) 
{
}

VideoControls::~VideoControls() {
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
    }
}

bool VideoControls::Initialize(HINSTANCE hInst, HWND hParent) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NekooVideoControls";
    
    RegisterClassEx(&wc);
    
    // Create main container window
    m_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"NekooVideoControls",
        L"Controls",
        WS_POPUP | WS_BORDER,
        0, 0, 140, 40,
        hParent,
        NULL,
        hInst,
        this // Pass 'this' pointer to WndProc
    );
    
    if (!m_hWnd) return false;
    
    // Create Pause/Resume button
    m_hBtnPause = CreateWindow(
        L"BUTTON",
        L"Pause",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        5, 5, 60, 30,
        m_hWnd,
        (HMENU)ID_BTN_PAUSE,
        hInst,
        NULL
    );
    
    // Create Stop button
    m_hBtnStop = CreateWindow(
        L"BUTTON",
        L"Stop",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        70, 5, 60, 30,
        m_hWnd,
        (HMENU)ID_BTN_STOP,
        hInst,
        NULL
    );
    
    // Set a nice font
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                            
    SendMessage(m_hBtnPause, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hBtnStop, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    return true;
}

void VideoControls::Show(int x, int y) {
    SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateState(false);
}

void VideoControls::Hide() {
    ShowWindow(m_hWnd, SW_HIDE);
}

void VideoControls::UpdateState(bool isPaused) {
    m_isPaused = isPaused;
    SetWindowText(m_hBtnPause, isPaused ? L"Resume" : L"Pause");
}

LRESULT CALLBACK VideoControls::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    VideoControls* pThis = NULL;
    
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (VideoControls*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (VideoControls*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    
    switch (message) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (pThis) {
                switch (wmId) {
                    case ID_BTN_PAUSE:
                        if (pThis->m_isPaused) {
                            if (pThis->OnResume) pThis->OnResume();
                        } else {
                            if (pThis->OnPause) pThis->OnPause();
                        }
                        break;
                    case ID_BTN_STOP:
                        if (pThis->OnStop) pThis->OnStop();
                        break;
                }
            }
            break;
        }
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
             HDC hdc = (HDC)wParam;
             SetBkMode(hdc, TRANSPARENT);
             return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}
