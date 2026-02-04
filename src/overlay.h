#pragma once
#include <windows.h>

enum class CaptureMode {
    Screenshot,
    Video
};

void ShowOverlay(HWND hwndParent, RECT* pRect, CaptureMode mode = CaptureMode::Screenshot);
