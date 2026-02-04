#include <windows.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <winhttp.h>
#include <commctrl.h>
#include <string>
#include <fstream>
#include <vector>
#include "resource.h"
#include "upload.h"
#include "overlay.h"
#include "toast.h"
#include "settings.h"
#include "video_recorder.h"
#include "video_controls.h"
#include <thread>
#include <atomic>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

// Global variables
HINSTANCE g_hInst;
NOTIFYICONDATA g_nid;
HWND g_hwndMain;
ULONG_PTR g_gdiplusToken;
int g_hotkeyId = 1;
int g_hotkeyRegionId = 2;
int g_hotkeyVideoId = 3;
AppSettings g_settings;
VideoRecorder g_videoRecorder;
VideoControls g_videoControls;
std::atomic<bool> g_isRecordingThreadRunning(false);
std::thread g_recordingThread;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);
void ShowSettingsDialog();
void CaptureScreenshot(bool useRegion = false);
void ShowToast(const std::wstring& url);
std::vector<BYTE> CaptureRegion(const RECT& rect);
std::vector<BYTE> CaptureFullscreen();
void RegisterHotkeys();
void UnregisterHotkeys();
std::wstring GetKeyName(UINT vk, UINT mods);
void CaptureVideo();
void StopVideoRecording();

// System tray menu IDs
#define IDM_CAPTURE_FULL 1001
#define IDM_CAPTURE_REGION 1002
#define IDM_CAPTURE_VIDEO 1003
#define IDM_SETTINGS 1004
#define IDM_EXIT 1005
#define WM_TRAYICON (WM_USER + 1)

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInst = hInstance;
    
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Register window class
    WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SekoClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wc);
    
    // Create hidden window
    g_hwndMain = CreateWindowEx(0, L"SekoClass", L"Seko",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    
    // Load settings
    g_settings = LoadSettings();
    
    // Initialize Video Controls
    g_videoControls.Initialize(g_hInst, NULL); // Parent is NULL for global floating window
    g_videoControls.OnPause = []() { g_videoRecorder.Pause(); g_videoControls.UpdateState(true); };
    g_videoControls.OnResume = []() { g_videoRecorder.Resume(); g_videoControls.UpdateState(false); };
    g_videoControls.OnStop = []() { StopVideoRecording(); };
    
    // Create system tray icon
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hwndMain;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPICON)); // Default icon
    wcscpy_s(g_nid.szTip, L"Seko");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
    
    // Register global hotkeys from settings
    RegisterHotkeys();
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    UnregisterHotkeys();
    GdiplusShutdown(g_gdiplusToken);
    
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            if (wParam == g_hotkeyId) {
                CaptureScreenshot(false);
            } else if (wParam == g_hotkeyRegionId) {
                CaptureScreenshot(true);
            } else if (wParam == g_hotkeyVideoId) {
                CaptureVideo();
            }
            return 0;
            
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, IDM_CAPTURE_FULL, L"Capture Fullscreen (Ctrl+Shift+S)");
                AppendMenu(hMenu, MF_STRING, IDM_CAPTURE_REGION, L"Capture Region (Ctrl+Shift+R)");
                AppendMenu(hMenu, MF_STRING, IDM_CAPTURE_VIDEO, L"Record Video (Ctrl+Shift+V)");
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, IDM_SETTINGS, L"Settings");
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
                
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_CAPTURE_FULL:
                    CaptureScreenshot(false);
                    break;
                case IDM_CAPTURE_REGION:
                    CaptureScreenshot(true);
                    break;
                case IDM_CAPTURE_VIDEO:
                    CaptureVideo();
                    break;
                case IDM_SETTINGS:
                    ShowSettingsDialog();
                    break;
                case IDM_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

std::vector<BYTE> CaptureFullscreen() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreen, w, h);
    HGDIOBJ hOld = SelectObject(hDC, hBmp);
    BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);
    
    Bitmap* bmp = new Bitmap(hBmp, NULL);
    IStream* stream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    
    CLSID clsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &clsid);
    bmp->Save(stream, &clsid);
    
    STATSTG stat;
    stream->Stat(&stat, STATFLAG_DEFAULT);
    ULONG size = (ULONG)stat.cbSize.QuadPart;
    
    std::vector<BYTE> data(size);
    LARGE_INTEGER pos = {0};
    stream->Seek(pos, STREAM_SEEK_SET, NULL);
    ULONG bytesRead;
    stream->Read(data.data(), size, &bytesRead);
    stream->Release();
    
    delete bmp;
    SelectObject(hDC, hOld);
    DeleteObject(hBmp);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    
    return data;
}

std::vector<BYTE> CaptureRegion(const RECT& rect) {
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    
    if (w <= 0 || h <= 0) return std::vector<BYTE>();
    
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreen, w, h);
    HGDIOBJ hOld = SelectObject(hDC, hBmp);
    BitBlt(hDC, 0, 0, w, h, hScreen, rect.left, rect.top, SRCCOPY);
    
    Bitmap* bmp = new Bitmap(hBmp, NULL);
    IStream* stream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    
    CLSID clsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &clsid);
    bmp->Save(stream, &clsid);
    
    STATSTG stat;
    stream->Stat(&stat, STATFLAG_DEFAULT);
    ULONG size = (ULONG)stat.cbSize.QuadPart;
    
    std::vector<BYTE> data(size);
    LARGE_INTEGER pos = {0};
    stream->Seek(pos, STREAM_SEEK_SET, NULL);
    ULONG bytesRead;
    stream->Read(data.data(), size, &bytesRead);
    stream->Release();
    
    delete bmp;
    SelectObject(hDC, hOld);
    DeleteObject(hBmp);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    
    return data;
}

void StopVideoRecording() {
    if (g_videoRecorder.IsRecording()) {
        g_videoRecorder.Stop();
        
        // Hide controls
        g_videoControls.Hide();
        
        // Wait for thread to finish
        if (g_isRecordingThreadRunning) {
            if (g_recordingThread.joinable()) {
                g_recordingThread.join();
            }
            g_isRecordingThreadRunning = false;
        }
        
        std::wstring path = g_videoRecorder.GetOutputPath();
        std::wstring msg = L"Video saved to:\n" + path;
        MessageBox(NULL, msg.c_str(), L"Recording Finished", MB_OK | MB_ICONINFORMATION); // Replace with Upload later
        
        // Ask to open folder?
        ShellExecute(NULL, L"open", L"explorer.exe", (L"/select,\"" + path + L"\"").c_str(), NULL, SW_SHOWDEFAULT);
    }
}

void CaptureVideo() {
    if (g_videoRecorder.IsRecording()) {
        MessageBox(NULL, L"Already recording!", L"Error", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    RECT rect = {0};
    ShowOverlay(g_hwndMain, &rect, CaptureMode::Video);
    
    if (rect.right == 0 && rect.bottom == 0) {
        return; // Cancelled
    }
    
    // Ensure even dimensions
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    if (w % 2 != 0) w++;
    if (h % 2 != 0) h++;
    rect.right = rect.left + w;
    rect.bottom = rect.top + h;
    
    // Temp file path
    wchar_t tempPath[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    std::wstring outputPath = std::wstring(tempPath) + L"nekoos_recording.mp4";
    
    // Initialize Recorder
    if (!g_videoRecorder.Initialize(outputPath, w, h, 30)) { // 30 FPS for GDI safety
        MessageBox(NULL, L"Failed to initialize video recorder", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Show Controls
    g_videoControls.Show(rect.left, rect.bottom + 5);
    
    // Start Recording Thread
    g_isRecordingThreadRunning = true;
    g_recordingThread = std::thread([rect, w, h]() {
        HDC hScreen = GetDC(NULL);
        HDC hDC = CreateCompatibleDC(hScreen);
        HBITMAP hBmp = CreateCompatibleBitmap(hScreen, w, h);
        HGDIOBJ hOld = SelectObject(hDC, hBmp);
        
        while (g_videoRecorder.IsRecording()) {
            auto start = std::chrono::steady_clock::now();
            
            if (!g_videoRecorder.IsPaused()) {
                // Capture Frame
                BitBlt(hDC, 0, 0, w, h, hScreen, rect.left, rect.top, SRCCOPY);
                
                // Get bits
                BITMAPINFOHEADER bi = {0};
                bi.biSize = sizeof(BITMAPINFOHEADER);
                bi.biWidth = w;
                bi.biHeight = -h; // Top-down
                bi.biPlanes = 1;
                bi.biBitCount = 32;
                bi.biCompression = BI_RGB;
                
                std::vector<BYTE> pixels(w * h * 4);
                GetDIBits(hDC, hBmp, 0, h, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                
                // Write to Media Foundation
                g_videoRecorder.WriteFrame(pixels);
            }
            
            // Frame pacing (aim for ~33ms for 30fps)
            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (elapsed < 33) {
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed));
            }
        }
        
        SelectObject(hDC, hOld);
        DeleteObject(hBmp);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
    });
    g_recordingThread.detach(); // Detach to allow UI thread to continue handling messages
}

void CaptureScreenshot(bool useRegion) {
    std::vector<BYTE> imageData;
    
    if (useRegion) {
        RECT rect = {0};
        ShowOverlay(g_hwndMain, &rect);
        
        if (rect.right == 0 && rect.bottom == 0) {
            return; // Cancelled
        }
        
        imageData = CaptureRegion(rect);
    } else {
        imageData = CaptureFullscreen();
    }
    
    if (imageData.empty()) {
        MessageBox(NULL, L"Failed to capture screenshot", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Show "Uploading..." toast immediately
    ShowUploadingToast();
    
    // Upload to nekoo.ru with API key if available
    std::wstring url = UploadToNekoo(imageData, g_settings.apiKey);
    
    if (url.empty()) {
        CloseToast();
        MessageBox(NULL, L"Failed to upload screenshot", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Update toast with URL
    UpdateToastWithUrl(url);
}

void ShowToast(const std::wstring& url) {
    ShowToastNotification(url);
}

void ShowSettingsDialog() {
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), g_hwndMain, SettingsDlgProc);
}

void RegisterHotkeys() {
    RegisterHotKey(g_hwndMain, g_hotkeyId, g_settings.fullscreenModifiers, g_settings.fullscreenKey);
    RegisterHotKey(g_hwndMain, g_hotkeyRegionId, g_settings.regionModifiers, g_settings.regionKey);
    
    // Default video hotkey: Ctrl+Shift+V
    RegisterHotKey(g_hwndMain, g_hotkeyVideoId, MOD_CONTROL | MOD_SHIFT, 'V'); 
}

void UnregisterHotkeys() {
    UnregisterHotKey(g_hwndMain, g_hotkeyId);
    UnregisterHotKey(g_hwndMain, g_hotkeyRegionId);
    UnregisterHotKey(g_hwndMain, g_hotkeyVideoId);
}

std::wstring GetKeyName(UINT vk, UINT mods) {
    std::wstring result;
    
    if (mods & MOD_CONTROL) result += L"Ctrl+";
    if (mods & MOD_SHIFT) result += L"Shift+";
    if (mods & MOD_ALT) result += L"Alt+";
    if (mods & MOD_WIN) result += L"Win+";
    
    wchar_t keyName[50] = {0};
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    
    // Special keys
    switch (vk) {
        case VK_SNAPSHOT: result += L"PrintScreen"; return result;
        case VK_PAUSE: result += L"Pause"; return result;
        case VK_INSERT: result += L"Insert"; return result;
        case VK_DELETE: result += L"Delete"; return result;
        case VK_HOME: result += L"Home"; return result;
        case VK_END: result += L"End"; return result;
        case VK_PRIOR: result += L"PageUp"; return result;
        case VK_NEXT: result += L"PageDown"; return result;
        case VK_ESCAPE: result += L"Esc"; return result;
        case VK_SPACE: result += L"Space"; return result;
        case VK_RETURN: result += L"Enter"; return result;
        case VK_BACK: result += L"Backspace"; return result;
        case VK_TAB: result += L"Tab"; return result;
    }
    
    // F keys
    if (vk >= VK_F1 && vk <= VK_F24) {
        swprintf_s(keyName, L"F%d", vk - VK_F1 + 1);
        result += keyName;
        return result;
    }
    
    // Get key name from system
    if (GetKeyNameText(scanCode << 16, keyName, 50)) {
        result += keyName;
    } else {
        swprintf_s(keyName, L"Key%d", vk);
        result += keyName;
    }
    
    return result;
}

LRESULT CALLBACK HotkeyEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    UINT* pData = (UINT*)dwRefData;
    
    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            UINT vk = (UINT)wParam;
            UINT mods = 0;
            
            // Capture modifiers
            if (GetKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
            if (GetKeyState(VK_SHIFT) & 0x8000) mods |= MOD_SHIFT;
            if (GetKeyState(VK_MENU) & 0x8000) mods |= MOD_ALT;
            
            // Don't capture modifier keys alone
            if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) {
                return 0;
            }
            
            // Store in the data array
            pData[0] = vk;
            pData[1] = mods;
            
            SetWindowText(hwnd, GetKeyName(vk, mods).c_str());
            return 0;
        }
        
        case WM_SETFOCUS:
            SetWindowText(hwnd, L"Press a key...");
            return 0;
        
        case WM_KILLFOCUS:
            if (pData[0] != 0) {
                SetWindowText(hwnd, GetKeyName(pData[0], pData[1]).c_str());
            }
            return 0;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static UINT g_fullscreenData[2] = {0};
static UINT g_regionData[2] = {0};

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // Set dialog icon
            HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPICON));
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

            // Initialize data arrays with current settings
            g_fullscreenData[0] = g_settings.fullscreenKey;
            g_fullscreenData[1] = g_settings.fullscreenModifiers;
            g_regionData[0] = g_settings.regionKey;
            g_regionData[1] = g_settings.regionModifiers;
            
            // Subclass edit controls for custom hotkey capture
            HWND hFullscreen = GetDlgItem(hDlg, IDC_HOTKEY_DISPLAY);
            HWND hRegion = GetDlgItem(hDlg, IDC_HOTKEY_REGION_DISPLAY);
            
            SetWindowSubclass(hFullscreen, HotkeyEditProc, 1, (DWORD_PTR)g_fullscreenData);
            SetWindowSubclass(hRegion, HotkeyEditProc, 2, (DWORD_PTR)g_regionData);
            
            // Display current hotkeys
            SetWindowText(hFullscreen, GetKeyName(g_settings.fullscreenKey, g_settings.fullscreenModifiers).c_str());
            SetWindowText(hRegion, GetKeyName(g_settings.regionKey, g_settings.regionModifiers).c_str());
            
            // Set auto-start checkbox
            CheckDlgButton(hDlg, IDC_AUTOSTART, g_settings.autoStart ? BST_CHECKED : BST_UNCHECKED);
            
            // Set API key field text limit to 256 characters
            SendDlgItemMessage(hDlg, IDC_API_KEY_EDIT, EM_SETLIMITTEXT, 256, 0);
            
            // Load and display API key
            if (!g_settings.apiKey.empty()) {
                SetDlgItemText(hDlg, IDC_API_KEY_EDIT, g_settings.apiKey.c_str());
                SetDlgItemText(hDlg, IDC_API_KEY_STATUS, L"Saved");
            }
            
            return TRUE;
        }
        
        case WM_CTLCOLORDLG:
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(50, 50, 50));
            SetBkColor(hdcStatic, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(30, 30, 30));
            SetBkColor(hdcEdit, RGB(250, 250, 250));
            return (LRESULT)CreateSolidBrush(RGB(250, 250, 250));
        }
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    // Get captured hotkeys from global arrays
                    if (g_fullscreenData[0] != 0) {
                        g_settings.fullscreenKey = g_fullscreenData[0];
                        g_settings.fullscreenModifiers = g_fullscreenData[1];
                    }
                    
                    if (g_regionData[0] != 0) {
                        g_settings.regionKey = g_regionData[0];
                        g_settings.regionModifiers = g_regionData[1];
                    }
                    
                    // Get auto-start
                    g_settings.autoStart = (IsDlgButtonChecked(hDlg, IDC_AUTOSTART) == BST_CHECKED);
                    
                    // Get API key
                    wchar_t apiKeyBuffer[256];
                    GetDlgItemText(hDlg, IDC_API_KEY_EDIT, apiKeyBuffer, 256);
                    g_settings.apiKey = apiKeyBuffer;
                    
                    // Save settings
                    SaveSettings(g_settings);
                    
                    // Re-register hotkeys
                    UnregisterHotkeys();
                    RegisterHotkeys();
                    
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                
                case IDC_API_KEY_VALIDATE: {
                    // Get API key from text box
                    wchar_t apiKeyBuffer[256];
                    GetDlgItemText(hDlg, IDC_API_KEY_EDIT, apiKeyBuffer, 256);
                    std::wstring apiKey = apiKeyBuffer;
                    
                    // Trim whitespace from both ends
                    apiKey.erase(0, apiKey.find_first_not_of(L" \t\r\n"));
                    apiKey.erase(apiKey.find_last_not_of(L" \t\r\n") + 1);
                    
                    if (apiKey.empty()) {
                        SetDlgItemText(hDlg, IDC_API_KEY_STATUS, L"Please enter an API key");
                        return TRUE;
                    }
                    
                    // Validate with server
                    std::wstring username;
                    if (ValidateApiKey(apiKey, username)) {
                        std::wstring successMsg = L"[OK] Valid! Logged in as: " + username;
                        SetDlgItemText(hDlg, IDC_API_KEY_STATUS, successMsg.c_str());
                        
                        // Save immediately
                        g_settings.apiKey = apiKey;
                        SaveSettings(g_settings);
                    } else {
                        SetDlgItemText(hDlg, IDC_API_KEY_STATUS, L"[ERROR] Invalid API key");
                    }
                    return TRUE;
                }
                    
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}


