#include <windows.h>
#include <functional>
#include "../Win32XmlUI.h"

int ArbitraryFunction() {
    MessageBoxA(nullptr, "Button clicked!", "Info", MB_ICONINFORMATION);
    return 0;
}

int OpenExample2() {
    HWND hwnd = CreateUIFromXML(nullptr, GetModuleHandle(nullptr), SW_SHOW, 102);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    RegisterCallback("ArbitraryFunction", []() { ArbitraryFunction(); });
    RegisterCallback("OpenExample2", []() { OpenExample2(); });
    HWND hwnd = CreateUIFromXML(nullptr, hInstance, nCmdShow, 101);
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