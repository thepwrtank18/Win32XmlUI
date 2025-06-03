#include <windows.h>
#include "Win32XmlUI.h"
#include "tinyxml2.h"
#include <string>
#include <commctrl.h>
#include <map>
#include <functional>
#include <vector>
#include <unordered_map>

static std::map<std::string, std::function<void()>> g_functionRegistry;

static std::map<HWND, std::string> g_buttonOnClickMap;

static std::vector<HFONT> g_createdFonts;

struct ControlColorInfo {
    COLORREF fg = CLR_INVALID;
    COLORREF bg = CLR_INVALID;
    HBRUSH hBrush = nullptr;
};

static std::unordered_map<HWND, ControlColorInfo> g_controlColorMap;

struct WindowExtraData {
    COLORREF bgColor;
    int closeAction;
};

namespace Win32XmlUI {

    
    HFONT CreateUIFont(const char* fontName, int fontSize) {
        int lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
        HFONT hFont = CreateFontA(
            lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, fontName ? fontName : "Segoe UI"
        );
        return hFont;
    }

    
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        WindowExtraData* pData = (WindowExtraData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        switch (uMsg) {
        case WM_ERASEBKGND: {
            COLORREF bgColor = pData ? pData->bgColor : RGB(240,240,240);
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH brush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
            return 1;
        }
        case WM_DESTROY:
            for (HFONT hFont : g_createdFonts) {
                DeleteObject(hFont);
            }
            g_createdFonts.clear();
            for (auto& pair : g_controlColorMap) {
                if (pair.second.hBrush) {
                    DeleteObject(pair.second.hBrush);
                }
            }
            g_controlColorMap.clear();
            if (pData) {
                if (pData->closeAction == 1) { // endprogram
                    PostQuitMessage(0);
                }
                delete pData;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
        case WM_COMMAND: {
            HWND ctrlHwnd = (HWND)lParam;
            if (HIWORD(wParam) == BN_CLICKED && g_buttonOnClickMap.count(ctrlHwnd)) {
                std::string funcName = g_buttonOnClickMap[ctrlHwnd];
                auto it = g_functionRegistry.find(funcName);
                if (it != g_functionRegistry.end()) {
                    it->second();
                }
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HWND ctrlHwnd = (HWND)lParam;
            HDC hdc = (HDC)wParam;
            auto it = g_controlColorMap.find(ctrlHwnd);
            if (it != g_controlColorMap.end()) {
                const ControlColorInfo& info = it->second;
                if (info.fg != CLR_INVALID)
                    SetTextColor(hdc, info.fg);
                if (info.bg != CLR_INVALID) {
                    SetBkColor(hdc, info.bg);
                    if (info.hBrush)
                        return (LRESULT)info.hBrush;
                }
            }
            break;
        }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR lParam) {
    char buf[256];
    if (IS_INTRESOURCE(lpszName)) {
        sprintf(buf, "Resource ID: %d", (int)(uintptr_t)lpszName);
    } else {
        sprintf(buf, "Resource Name: %s", lpszName);
    }
    MessageBoxA(nullptr, buf, "Resource Found", MB_OK);
    return TRUE;
}


    std::string LoadXmlResource(HINSTANCE hInstance, const char* resourceName, int resourceId) {
        EnumResourceNamesA(GetModuleHandle(NULL), "RCDATA", EnumResNameProc, 0);
        HRSRC hRes = FindResourceA(GetModuleHandle(NULL), MAKEINTRESOURCEA(resourceId), "BINARY");
        if (!hRes) {
            MessageBoxA(nullptr, "FindResourceA failed", "Debug", MB_OK);
            return "";
        }
        HGLOBAL hData = LoadResource(hInstance, hRes);
        if (!hData) {
            MessageBoxA(nullptr, "LoadResource failed", "Debug", MB_OK);
            return "";
        }
        DWORD size = SizeofResource(hInstance, hRes);
        const char* data = (const char*)LockResource(hData);
        if (!data) {
            MessageBoxA(nullptr, "LockResource failed", "Debug", MB_OK);
            return "";
        }
        return std::string(data, size);
    }

    HWND CreateUIFromXML(const char* xmlPath, HINSTANCE hInstance, int nCmdShow, int resourceId) {
        tinyxml2::XMLDocument doc;
        std::string xmlContent;
        if (!xmlPath || !*xmlPath) {
            HINSTANCE hExe = GetModuleHandle(NULL);
            xmlContent = LoadXmlResource(hExe, "IDR_XML1", resourceId);

            if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                MessageBoxA(nullptr, xmlContent.c_str(), "Loaded XML", MB_OK);
            }
            
            if (xmlContent.empty() || doc.Parse(xmlContent.c_str()) != tinyxml2::XML_SUCCESS) {
                MessageBoxA(nullptr, "XML is empty or parsing failed.", "Debug", MB_OK);
                return nullptr;
            }
        } else {
            if (doc.LoadFile(xmlPath) != tinyxml2::XML_SUCCESS) {
                return nullptr;
            }
        }

        INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&icc);

        tinyxml2::XMLElement* root = doc.FirstChildElement("Window");
        if (!root) return nullptr;

        // Parse window attributes
        const char* dim = root->Attribute("dimension");
        const char* title = root->Attribute("title");
        const char* border = root->Attribute("borderstyle");
        const char* closeaction = root->Attribute("closeaction");
        int width = 640, height = 480;
        if (dim) sscanf(dim, "%d,%d", &width, &height);
        std::string windowTitle = title ? title : "Win32XmlUI";

        // Default background color (light gray)
        COLORREF bgColor = RGB(240, 240, 240);
        tinyxml2::XMLElement* bgElem = root->FirstChildElement("Background");
        if (bgElem) {
            const char* colorStr = bgElem->Attribute("color");
            if (colorStr) {
                int r = 240, g = 240, b = 240;
                sscanf(colorStr, "%d,%d,%d", &r, &g, &b);
                bgColor = RGB(r, g, b);
            }
        }

        // Parse closeaction
        int closeAction = 0; // default: closewindow
        if (closeaction) {
            if (strcmp(closeaction, "endprogram") == 0) closeAction = 1;
        }

        // Register window class
        const char CLASS_NAME[] = "Win32XmlUIWindowClass";
        static bool registered = false;
        if (!registered) {
            WNDCLASSA wc = {};
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = hInstance;
            wc.lpszClassName = CLASS_NAME;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            RegisterClassA(&wc);
            registered = true;
        }

        // Default style
        DWORD style = WS_OVERLAPPEDWINDOW;
        // Choose window style based on borderstyle attribute
        if (border) {
            std::string borderStyle(border);
            if (borderStyle == "none") {
                style = WS_POPUP;
            }
            else if (borderStyle == "fixedsingle") {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            }
            else if (borderStyle == "fixed3d") {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                // WS_EX_CLIENTEDGE will be added in CreateWindowEx
            }
            else if (borderStyle == "fixeddialog") {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
            }
            else if (borderStyle == "sizable") {
                style = WS_OVERLAPPEDWINDOW;
            }
            else if (borderStyle == "fixedtoolwindow") {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                // WS_EX_TOOLWINDOW will be added in CreateWindowEx
            }
            else if (borderStyle == "sizabletoolwindow") {
                style = WS_OVERLAPPEDWINDOW;
                // WS_EX_TOOLWINDOW will be added in CreateWindowEx
            }
        }

        // Determine extended window style
        DWORD exStyle = 0;
        if (border) {
            std::string borderStyle(border);
            if (borderStyle == "fixed3d") {
                exStyle |= WS_EX_CLIENTEDGE;
            }
            if (borderStyle == "fixedtoolwindow" || borderStyle == "sizabletoolwindow") {
                exStyle |= WS_EX_TOOLWINDOW;
            }
        }

        

        // Create main window
        HWND hwnd = CreateWindowExA(
            exStyle, CLASS_NAME, windowTitle.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, hInstance, nullptr);
        if (!hwnd) return nullptr;

        WindowExtraData* pData = new WindowExtraData{ bgColor, closeAction };
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);

        for (tinyxml2::XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
            std::string tag = child->Name();
            if (tag == "Text") {
                const char* loc = child->Attribute("location");
                const char* text = child->Attribute("text");
                const char* fontName = child->Attribute("font");
                int fontSize = child->IntAttribute("fontsize", 9);
                const char* fgStr = child->Attribute("foreground");
                const char* bgStr = child->Attribute("background");
                int x = 0, y = 0;
                if (loc) sscanf(loc, "%d,%d", &x, &y);
                HWND hText = CreateWindowExA(0, "STATIC", text ? text : "",
                    WS_CHILD | WS_VISIBLE,
                    x, y, 300, 20, hwnd, nullptr, hInstance, nullptr);
                if (hText) {
                    HFONT hFont = CreateUIFont(fontName, fontSize);
                    g_createdFonts.push_back(hFont);
                    SendMessage(hText, WM_SETFONT, (WPARAM)hFont, TRUE);
                    ControlColorInfo info;
                    if (fgStr) {
                        int r, g, b;
                        if (sscanf(fgStr, "%d,%d,%d", &r, &g, &b) == 3)
                            info.fg = RGB(r, g, b);
                    }
                    if (bgStr) {
                        int r, g, b;
                        if (sscanf(bgStr, "%d,%d,%d", &r, &g, &b) == 3) {
                            info.bg = RGB(r, g, b);
                            info.hBrush = CreateSolidBrush(info.bg);
                        }
                    }
                    g_controlColorMap[hText] = info;
                }
            } else if (tag == "Button") {
                const char* loc = child->Attribute("location");
                const char* size = child->Attribute("size");
                const char* text = child->Attribute("text");
                const char* onclick = child->Attribute("onclick");
                const char* fontName = child->Attribute("font");
                int fontSize = child->IntAttribute("fontsize", 9);
                const char* fgStr = child->Attribute("foreground");
                const char* bgStr = child->Attribute("background");
                int x = 0, y = 0, w = 80, h = 25;
                if (loc) sscanf(loc, "%d,%d", &x, &y);
                if (size) sscanf(size, "%d,%d", &w, &h);
                HWND btnHwnd = CreateWindowExA(0, "BUTTON", text ? text : "Button",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    x, y, w, h, hwnd, nullptr, hInstance, nullptr);

                if (btnHwnd) {
                    HFONT hFont = CreateUIFont(fontName, fontSize);
                    g_createdFonts.push_back(hFont);
                    SendMessage(btnHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
                    ControlColorInfo info;
                    if (fgStr) {
                        int r, g, b;
                        if (sscanf(fgStr, "%d,%d,%d", &r, &g, &b) == 3)
                            info.fg = RGB(r, g, b);
                    }
                    if (bgStr) {
                        int r, g, b;
                        if (sscanf(bgStr, "%d,%d,%d", &r, &g, &b) == 3) {
                            info.bg = RGB(r, g, b);
                            info.hBrush = CreateSolidBrush(info.bg);
                        }
                    }
                    g_controlColorMap[btnHwnd] = info;
                }
                if (onclick && btnHwnd) {
                    std::string funcName = onclick;
                    if (funcName.size() > 2 && funcName.substr(funcName.size() - 2) == "()") {
                        funcName = funcName.substr(0, funcName.size() - 2);
                    }
                    g_buttonOnClickMap[btnHwnd] = funcName;
                }
            }
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
        return hwnd;
    }

    void RegisterCallback(const char* name, std::function<void()> func) {
        g_functionRegistry[name] = func;
    }
} 

// Exported functions at global scope
HWND CreateUIFromXML(const char* xmlPath, HINSTANCE hInstance, int nCmdShow, int resourceId) {
    return Win32XmlUI::CreateUIFromXML(xmlPath, hInstance, nCmdShow, resourceId);
}

void RegisterCallback(const char* name, std::function<void()> func) {
    Win32XmlUI::RegisterCallback(name, func);
} 