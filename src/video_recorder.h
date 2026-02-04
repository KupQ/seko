#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wrl/client.h>
#include <string>
#include <vector>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using Microsoft::WRL::ComPtr;

class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    bool Initialize(const std::wstring& outputPath, UINT width, UINT height, UINT fps = 60, UINT bitrate = 8000000);
    bool WriteFrame(const std::vector<BYTE>& frameData); // Expects BGRA data
    bool Pause();
    bool Resume();
    bool Stop();
    
    bool IsRecording() const { return m_isRecording; }
    bool IsPaused() const { return m_isPaused; }
    std::wstring GetOutputPath() const { return m_outputPath; }

private:
    bool InitializeSinkWriter(const std::wstring& outputPath);
    bool ConfigureInputMediaType();

    ComPtr<IMFSinkWriter> m_pSinkWriter;
    DWORD m_streamIndex;
    
    UINT m_width;
    UINT m_height;
    UINT m_fps;
    UINT m_bitrate;
    
    std::wstring m_outputPath;
    bool m_isRecording;
    bool m_isPaused;
    
    // Time management for Pause/Resume
    LONGLONG m_startTime;           // When recording started (system time)
    LONGLONG m_lastPauseTime;       // When pause started (system time)
    LONGLONG m_totalPausedDuration; // Total duration spent paused
    LONGLONG m_lastFrameTime;       // Timestamp of the last written frame
};
