#pragma once
#include <windows.h>
#include <functional>

class VideoControls {
public:
    VideoControls();
    ~VideoControls();

    bool Initialize(HINSTANCE hInst, HWND hParent);
    void Show(int x, int y);
    void Hide();
    void UpdateState(bool isPaused);

    // Callbacks
    std::function<void()> OnPause;
    std::function<void()> OnResume;
    std::function<void()> OnStop;

    HWND GetHwnd() const { return m_hWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    HWND m_hWnd;
    HWND m_hBtnPause;
    HWND m_hBtnStop;
    
    bool m_isPaused;
};
