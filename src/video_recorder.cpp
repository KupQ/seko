#include "video_recorder.h"
#include <chrono>

// Safe release macro
template <class T> void SafeRelease(T **ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

VideoRecorder::VideoRecorder() 
    : m_streamIndex(0), 
      m_width(0), 
      m_height(0), 
      m_fps(60), 
      m_bitrate(8000000), 
      m_isRecording(false), 
      m_isPaused(false),
      m_startTime(0),
      m_lastPauseTime(0),
      m_totalPausedDuration(0),
      m_lastFrameTime(0)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    MFStartup(MF_VERSION);
}

VideoRecorder::~VideoRecorder() {
    if (m_isRecording) {
        Stop();
    }
    MFShutdown();
    CoUninitialize();
}

bool VideoRecorder::Initialize(const std::wstring& outputPath, UINT width, UINT height, UINT fps, UINT bitrate) {
    m_outputPath = outputPath;
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_bitrate = bitrate;
    
    // Reset state
    m_isRecording = false;
    m_isPaused = false;
    m_startTime = 0;
    m_totalPausedDuration = 0;
    
    // Ensure dimensions are even (requirement for some encoders)
    if (m_width % 2 != 0) m_width++;
    if (m_height % 2 != 0) m_height++;

    return InitializeSinkWriter(outputPath);
}

bool VideoRecorder::InitializeSinkWriter(const std::wstring& outputPath) {
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> pAttributes;
    
    // Create attributes for Sink Writer
    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return false;
    
    hr = pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (FAILED(hr)) return false;

    // Create Sink Writer
    hr = MFCreateSinkWriterFromURL(outputPath.c_str(), NULL, pAttributes.Get(), &m_pSinkWriter);
    if (FAILED(hr)) return false;

    // Set up output media type (H.264 Video)
    ComPtr<IMFMediaType> pMediaTypeOut;
    hr = MFCreateMediaType(&pMediaTypeOut);
    if (FAILED(hr)) return false;

    hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, m_bitrate);
    hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    hr = MFSetAttributeSize(pMediaTypeOut.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
    hr = MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_FRAME_RATE, m_fps, 1);
    hr = MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_pSinkWriter->AddStream(pMediaTypeOut.Get(), &m_streamIndex);
    if (FAILED(hr)) return false;

    // Set up input media type (RGB32 / BGRA)
    ComPtr<IMFMediaType> pMediaTypeIn;
    hr = MFCreateMediaType(&pMediaTypeIn);
    if (FAILED(hr)) return false;

    hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    hr = MFSetAttributeSize(pMediaTypeIn.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
    hr = MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_FRAME_RATE, m_fps, 1);
    hr = MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_pSinkWriter->SetInputMediaType(m_streamIndex, pMediaTypeIn.Get(), NULL);
    if (FAILED(hr)) return false;

    // Start Sink Writer
    hr = m_pSinkWriter->BeginWriting();
    if (FAILED(hr)) return false;

    m_isRecording = true;
    m_startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    return true;
}

bool VideoRecorder::WriteFrame(const std::vector<BYTE>& frameData) {
    if (!m_isRecording || m_isPaused || !m_pSinkWriter) return false;

    HRESULT hr = S_OK;
    ComPtr<IMFSample> pSample;
    ComPtr<IMFMediaBuffer> pBuffer;
    
    // Create media buffer
    DWORD frameSize = m_width * m_height * 4; // BGRA
    hr = MFCreateMemoryBuffer(frameSize, &pBuffer);
    if (FAILED(hr)) return false;

    // Copy data to buffer
    BYTE* pData = NULL;
    hr = pBuffer->Lock(&pData, NULL, NULL);
    if (FAILED(hr)) return false;
    
    // Important: Media Foundation expects bottom-up DIB or top-down depending on stride.
    // Screen captures usually come top-down. 
    // We assume frameData is already correct format (RGB32).
    memcpy(pData, frameData.data(), frameSize);
    
    hr = pBuffer->Unlock();
    if (FAILED(hr)) return false;

    hr = pBuffer->SetCurrentLength(frameSize);
    if (FAILED(hr)) return false;

    // Create sample
    hr = MFCreateSample(&pSample);
    if (FAILED(hr)) return false;

    hr = pSample->AddBuffer(pBuffer.Get());
    if (FAILED(hr)) return false;

    // Calculate timestamp
    LONGLONG currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    
    // Adjust for pauses and start time
    // Media Foundation time unit is 100-nanoseconds
    LONGLONG sampleTime = (currentTime - m_startTime - m_totalPausedDuration) * 10000;
    
    hr = pSample->SetSampleTime(sampleTime);
    if (FAILED(hr)) return false;
    
    hr = pSample->SetSampleDuration(10000000 / m_fps); // 1 second / fps in 100ns units
    if (FAILED(hr)) return false;

    // Write sample
    hr = m_pSinkWriter->WriteSample(m_streamIndex, pSample.Get());
    if (FAILED(hr)) return false;
    
    m_lastFrameTime = sampleTime;

    return true;
}

bool VideoRecorder::Pause() {
    if (!m_isRecording || m_isPaused) return false;
    
    m_isPaused = true;
    m_lastPauseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    
    return true;
}

bool VideoRecorder::Resume() {
    if (!m_isRecording || !m_isPaused) return false;
    
    LONGLONG currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    
    m_totalPausedDuration += (currentTime - m_lastPauseTime);
    m_isPaused = false;
    
    return true;
}

bool VideoRecorder::Stop() {
    if (!m_isRecording) return false;
    
    if (m_pSinkWriter) {
        m_pSinkWriter->Finalize();
        m_pSinkWriter.Reset();
    }
    
    m_isRecording = false;
    m_isPaused = false;
    
    return true;
}
