#pragma once
#include <windows.h>
#include <string>

// Settings structure
struct AppSettings {
    UINT fullscreenModifiers;
    UINT fullscreenKey;
    UINT regionModifiers;
    UINT regionKey;
    bool autoStart;
    std::wstring apiKey;  // Nekoo API key
    
    // Video Recording Settings
    std::wstring videoSavePath;  // Custom save path for videos
    UINT videoFPS;               // FPS (30-144)
    UINT videoBitrate;           // Bitrate in bps (e.g., 8000000 = 8 Mbps)
    bool videoAutoUpload;        // Auto-upload to nekoo.ru after recording
};

// Load settings from registry
AppSettings LoadSettings();

// Save settings to registry
void SaveSettings(const AppSettings& settings);

// Get auto-start status
bool IsAutoStartEnabled();

// Set auto-start
void SetAutoStart(bool enable);

// Validate API key with nekoo server
bool ValidateApiKey(const std::wstring& apiKey, std::wstring& username);
