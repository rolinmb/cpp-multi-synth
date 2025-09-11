#undef UNICODE
#undef _UNICODE
#include <windows.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

// Key mapping to notes
struct KeyNote {
    int vk;          // virtual key
    const char* note;
    double freq;
};

KeyNote keyNotes[] = {
    {'Q',"C4",261.63}, {'2',"C#4/Db4",277.18}, {'W',"D4",293.66}, {'3',"D#4/Eb4",311.13},
    {'E',"E4",329.63}, {'R',"F4",349.23}, {'5',"F#4/Gb4",369.99}, {'T',"G4",392.00},
    {'6',"G#4/Ab4",415.30}, {'Y',"A4",440.00}, {'7',"A#4/Bb4",466.16}, {'U',"B4",493.88},
    {'I',"C5",523.25}
};

const int keyCount = sizeof(keyNotes)/sizeof(KeyNote);

// Key press state (volatile for thread safety)
volatile bool keyPressed[sizeof(keyNotes)/sizeof(KeyNote)] = {0};

// Window constants
const int keyWidth = 40;
const int keyHeight = 40;
const int keySpacing = 10;

// Audio constants
const int SAMPLE_RATE = 44100;

double phases[keyCount] = {0};  // global phase for each note
HWAVEOUT hWave;

DWORD WINAPI AudioThread(LPVOID) {
    WAVEFORMATEX wf = {};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = SAMPLE_RATE;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    waveOutOpen(&hWave, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);

    std::vector<short> buffer(SAMPLE_RATE / 20); // 50ms

    while (true) {
        std::fill(buffer.begin(), buffer.end(), 0);

        for (int k = 0; k < keyCount; ++k) {
            if (!keyPressed[k]) continue;
            double freq = keyNotes[k].freq;
            for (size_t i = 0; i < buffer.size(); ++i) {
                phases[k] += 2.0 * M_PI * freq / SAMPLE_RATE;
                if (phases[k] > 2.0 * M_PI) phases[k] -= 2.0 * M_PI;
                buffer[i] += (short)(3000 * sin(phases[k]));
            }
        }

        WAVEHDR wh = {};
        wh.lpData = (LPSTR)buffer.data();
        wh.dwBufferLength = buffer.size() * sizeof(short);
        waveOutPrepareHeader(hWave, &wh, sizeof(wh));
        waveOutWrite(hWave, &wh, sizeof(wh));

        // wait for buffer duration before reusing
        Sleep(50);
        waveOutUnprepareHeader(hWave, &wh, sizeof(wh));
    }
    return 0;
}


// Draw the keys and their states
void DrawKeys(HDC hdc, HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));

    BYTE keyStates[256];
    GetKeyboardState(keyStates);

    for (int i = 0; i < keyCount; ++i) {
        int x = keySpacing + i * (keyWidth + keySpacing);
        int y = 20;

        keyPressed[i] = (keyStates[keyNotes[i].vk] & 0x80) != 0;
        bool pressed = keyPressed[i];

        HBRUSH brush = pressed ? CreateSolidBrush(RGB(0,200,0)) : (HBRUSH)(COLOR_WINDOW+1);
        RECT r = { x, y, x + keyWidth, y + keyHeight };
        FillRect(hdc, &r, brush);
        if (pressed) DeleteObject(brush);

        Rectangle(hdc, x, y, x + keyWidth, y + keyHeight);

        char keyChar[2] = { (char)keyNotes[i].vk, 0 };
        TextOutA(hdc, x + keyWidth/2 - 5, y + 5, keyChar, 1);
        TextOutA(hdc, x + 2, y + keyHeight/2, keyNotes[i].note, lstrlenA(keyNotes[i].note));
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawKeys(hdc, hwnd);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER:
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "cpp-multi-synth";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, "C++ Real-time Synth",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 200, NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    SetTimer(hwnd, 1, 10, NULL); // refresh 100Hz

    // Start audio thread
    CreateThread(NULL, 0, AudioThread, NULL, 0, NULL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
