#pragma once
#include <windows.h>

class RecordingIndicator {
public:
    RecordingIndicator();
    ~RecordingIndicator();

    void Show(const RECT& region);
    void Hide();
    void UpdateState(bool isPaused);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    
    HWND m_hWnd;
    bool m_isPaused;
    RECT m_region;
};
