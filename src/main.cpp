#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <string>

// Keys to track
const int keysToCheck[] = { 'Q','2','W','3','E','R','5','T','6','Y','7','U','I' };
const int keyCount = sizeof(keysToCheck) / sizeof(keysToCheck[0]);

// Size for each key rectangle
const int keyWidth = 40;
const int keyHeight = 40;
const int keySpacing = 10;

// Draw the keys and their states
void DrawKeys(HDC hdc) {
    // Get the current state of all keys
    BYTE keyStates[256];
    GetKeyboardState(keyStates);

    for (int i = 0; i < keyCount; ++i) {
        int vk = keysToCheck[i];
        int x = keySpacing + i * (keyWidth + keySpacing);
        int y = 20;

        // Check if key is down (high bit = 0x80)
        bool pressed = (keyStates[vk] & 0x80) != 0;

        // Fill rectangle green if pressed, white if not
        HBRUSH brush = pressed ? CreateSolidBrush(RGB(0, 200, 0)) : (HBRUSH)(COLOR_WINDOW+1);
        RECT r = { x, y, x + keyWidth, y + keyHeight };
        FillRect(hdc, &r, brush);
        if (pressed) DeleteObject(brush);

        // Draw rectangle border
        Rectangle(hdc, x, y, x + keyWidth, y + keyHeight);

        // Draw key character
        char keyChar[2] = { (char)vk, 0 };
        TextOutA(hdc, x + keyWidth/2 - 5, y + 10, keyChar, 1);

        // Draw key state label
        const char* label = pressed ? "DOWN" : "UP";
        TextOutA(hdc, x + 2, y + keyHeight + 5, label, lstrlenA(label));
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear background
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));

        DrawKeys(hdc);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_TIMER:
        // Trigger redraw every tick
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
        0, CLASS_NAME, "cpp-multi-synth",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        700, 150, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    // Refresh 100 times per second
    SetTimer(hwnd, 1, 10, NULL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
