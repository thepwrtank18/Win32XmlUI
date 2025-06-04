#include <windows.h>
#include <functional>
#include "../Win32XmlUI.h"

HWND g_mainHwnd = nullptr;
HWND g_example2Hwnd = nullptr;

int ArbitraryFunction() {
    MessageBoxA(nullptr, "Button clicked!", "Info", MB_ICONINFORMATION);
    return 0;
}

int OpenExample2() {
    g_example2Hwnd = CreateUIFromXML(nullptr, GetModuleHandle(nullptr), SW_SHOW, 102);
    return 0;
}

void ChangeButtonText(HWND hwnd) {
    SetElementProperty(hwnd, "button1", "text", "Clicked!");
    SetElementProperty(hwnd, nullptr, "dimension", "1280,720");
    SetElementProperty(hwnd, nullptr, "title", "Example 2 (big)");
    SetElementProperty(hwnd, nullptr, "closeaction", "endprogram");
    SetElementProperty(hwnd, nullptr, "borderstyle", "sizable");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    RegisterCallback("ArbitraryFunction", []([[maybe_unused]] HWND hwnd) { ArbitraryFunction(); });
    RegisterCallback("OpenExample2", []([[maybe_unused]] HWND hwnd) { OpenExample2(); });
    RegisterCallback("ChangeButtonText", [](HWND hwnd) { ChangeButtonText(hwnd); });
    HWND hwnd = CreateUIFromXML(nullptr, hInstance, nCmdShow, 101);
    ::g_mainHwnd = hwnd;
    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create UI from XML.", "Error", MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}