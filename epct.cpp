#include <windows.h>
#include <dbt.h>
#include <mmsystem.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <timeapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")

DEFINE_GUID(GUID_DEVINTERFACE_AUDIO_RENDER, 0xE6327CAD, 0xDCEC, 0x4949, 0xAE, 0x8A, 0x99, 0x1E, 0x97, 0x6A, 0x79, 0xD2);

HWND g_hwnd = NULL;
bool g_lastState = false;
bool g_firstRun = true;
WCHAR g_wavPath[MAX_PATH];

#pragma pack(push, 1)
struct WAVHeader {
    char riffID[4] = {'R', 'I', 'F', 'F'};
    uint32_t size;
    char waveID[4] = {'W', 'A', 'V', 'E'};
    char fmtID[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 2;
    uint32_t sampleRate = 44100;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = 16;
    char dataID[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};
#pragma pack(pop)

struct Note {
    int16_t note;
    uint16_t duration;
};

void GenerateWavFile(const WCHAR* filename) {
    const Note melody[] = {
        {76, 150},
        {81, 150},
        {83, 300}
    };
    
    FILE* file;
    if (_wfopen_s(&file, filename, L"wb") != 0 || !file) return;
    
    constexpr size_t BUFFER_SIZE = 1024;
    int16_t buffer[BUFFER_SIZE];
    size_t bufferIndex = 0;
    
    WAVHeader header;
    header.blockAlign = 4;
    header.byteRate = 176400;
    header.dataSize = 105840;
    header.size = header.dataSize + 36;
    
    fwrite(&header, sizeof(header), 1, file);
    
    int noteStartSample = 0;
    const int totalSamples = 26460;
    
    for (const Note& m : melody) {
        const int noteSamples = (44100 * m.duration) / 1000;
        const float freq = 440.0f * pow(2.0f, (m.note - 69) / 12.0f);
        
        for (int i = 0; i < noteSamples; i++) {
            const float time = (float)i / 44100;
            const float progress = (float)i / noteSamples;
            
            float envelope = 1.0f;
            if (progress < 0.1f) 
                envelope = progress * 10;
            else if (progress > 0.8f) 
                envelope = (1.0f - progress) * 5;
            
            const float totalProgress = (float)(noteStartSample + i) / totalSamples;
            const int16_t sample = (int16_t)(16383.5f * envelope * sin(6.28318f * freq * time));
            
            buffer[bufferIndex++] = (int16_t)(sample * (1.0f - totalProgress));
            buffer[bufferIndex++] = (int16_t)(sample * totalProgress);
            
            if (bufferIndex >= BUFFER_SIZE) {
                fwrite(buffer, sizeof(int16_t), bufferIndex, file);
                bufferIndex = 0;
            }
        }
        noteStartSample += noteSamples;
    }
    
    if (bufferIndex > 0) {
        fwrite(buffer, sizeof(int16_t), bufferIndex, file);
    }
    
    fclose(file);
}

bool InitializeAudio() {
    if (!GetTempPathW(MAX_PATH, g_wavPath)) return false;
    wcscat_s(g_wavPath, MAX_PATH, L"EPCT.wav");
    
    if (GetFileAttributesW(g_wavPath) == INVALID_FILE_ATTRIBUTES) {
        GenerateWavFile(g_wavPath);
    }
    return true;
}

bool IsAudioDeviceReady() {
    WAVEOUTCAPS woc;
    return waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR;
}

void PlayStereoSound(bool isConnected) {
    if (!isConnected) return;

    timeBeginPeriod(1);

    for (int i = 0; i < 10; i++) {
        if (IsAudioDeviceReady()) {
            Sleep(100);
            PlaySoundW(g_wavPath, NULL, SND_FILENAME | SND_SYNC);
            break;
        }
        Sleep(100);
    }

    timeEndPeriod(1);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DEVICECHANGE: {
            if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                DEV_BROADCAST_HDR* pHdr = (DEV_BROADCAST_HDR*)lParam;
                if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    DEV_BROADCAST_DEVICEINTERFACE* pDevInf = (DEV_BROADCAST_DEVICEINTERFACE*)pHdr;
                    if (IsEqualGUID(pDevInf->dbcc_classguid, GUID_DEVINTERFACE_AUDIO_RENDER)) {
                        bool currentState = (wParam == DBT_DEVICEARRIVAL);
                        if (g_firstRun) {
                            g_lastState = currentState;
                            g_firstRun = false;
                        } 
                        else if (currentState != g_lastState) {
                            PlayStereoSound(currentState);
                            g_lastState = currentState;
                        }
                    }
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!InitializeAudio()) return 1;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.lpszClassName = L"EPCTClass";
    
    if (!RegisterClassExW(&wc)) return 1;

    g_hwnd = CreateWindowExW(0, L"EPCTClass", L"EPCT",
        WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, NULL, NULL, hInstance, NULL);
    if (!g_hwnd) return 1;

    DEV_BROADCAST_DEVICEINTERFACE_W notificationFilter = {0};
    notificationFilter.dbcc_size = sizeof(notificationFilter);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid = GUID_DEVINTERFACE_AUDIO_RENDER;
    
    HDEVNOTIFY deviceNotify = RegisterDeviceNotificationW(g_hwnd, &notificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!deviceNotify) {
        DestroyWindow(g_hwnd);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterDeviceNotification(deviceNotify);
    return (int)msg.wParam;
}
