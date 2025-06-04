#include <windows.h>
#include "Win32XmlUI.h"
#include "tinyxml2.h"
#include <string>
#include <commctrl.h>
#include <map>
#include <functional>
#include <vector>
#include <unordered_map>
#include <richedit.h>
#include <ole2.h>
#include <textserv.h>
#include <objbase.h>
#include <strsafe.h>
#include <tom.h>
#include <regex>

static std::map<std::string, std::function<void()>> g_functionRegistry;

static std::map<HWND, std::string> g_buttonOnClickMap;

struct ControlColorInfo {
    COLORREF fg = CLR_INVALID;
    COLORREF bg = CLR_INVALID;
    HBRUSH hBrush = nullptr;
};

static std::unordered_map<HWND, ControlColorInfo> g_controlColorMap;

struct WindowExtraData {
    COLORREF bgColor;
    int closeAction;
    std::vector<HFONT> createdFonts;
};

static HMODULE g_hRichEdit = nullptr;

namespace Win32XmlUI {


    static int CALLBACK FontEnumProc(const LOGFONTA* lpelfe, const TEXTMETRICA*, DWORD, LPARAM lParam) {
        *(bool*)lParam = true;
        return 0; // Stop enumeration
    }


    // Helper function to check if a font exists on the system
    bool FontExists(const char* fontName) {
        LOGFONTA lf = {0};
        lf.lfCharSet = DEFAULT_CHARSET;
        strncpy(lf.lfFaceName, fontName, LF_FACESIZE - 1);
        HDC hdc = GetDC(NULL);
        bool found = false;
        EnumFontFamiliesExA(hdc, &lf, FontEnumProc, (LPARAM)&found, 0);
        ReleaseDC(NULL, hdc);
        return found;
    }

    HFONT CreateUIFont(const char* fontName, int fontSize, const char* fontType) {
        if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
            char msg[256];
            snprintf(msg, sizeof(msg), "fontType: %s", fontType ? fontType : "(null)");
            MessageBoxA(NULL, msg, "fontType Debug", MB_OK);
        }
        int lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
        const char* requestedFont = nullptr;

        if (fontType && strcmp(fontType, "monospace") == 0) {
            if (fontName && FontExists(fontName)) {
                requestedFont = fontName;
            } else if (FontExists("Consolas")) {
                requestedFont = "Consolas";
            } else if (FontExists("Lucida Console")) {
                requestedFont = "Lucida Console";
            } else {
                requestedFont = "Tahoma";
            }
        } else {
            if (fontName && FontExists(fontName)) {
                requestedFont = fontName;
            } else {
                requestedFont = "Segoe UI";
            }
        }

        HFONT hFont = CreateFontA(
            lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, requestedFont
        );
        if (!hFont) {
            hFont = CreateFontA(
                lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Tahoma"
            );
        }
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
            if (pData) {
                for (HFONT hFont : pData->createdFonts) {
                    DeleteObject(hFont);
                }
                pData->createdFonts.clear();
                for (auto& pair : g_controlColorMap) {
                    if (pair.second.hBrush) {
                        DeleteObject(pair.second.hBrush);
                    }
                }
                g_controlColorMap.clear();
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
        case WM_CTLCOLOREDIT: {
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
        StringCchPrintfA(buf, sizeof(buf), "Resource ID: %d", (int)(uintptr_t)lpszName);
    } else {
        StringCchPrintfA(buf, sizeof(buf), "Resource Name: %s", lpszName);
    }
    if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
        MessageBoxA(nullptr, buf, "Resource Found", MB_OK);
    }
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

    // Preprocess XML to ensure a space before every <br> or <br/> tag if not already present
    static std::string PreprocessXmlBrSpacing(const std::string& xml) {
        std::string out;
        size_t i = 0;
        while (i < xml.size()) {
            size_t br = xml.find("<br", i);
            if (br == std::string::npos) {
                out += xml.substr(i);
                break;
            }
            // Copy up to <br
            out += xml.substr(i, br - i);
            // Check if previous char is space or newline
            if (br == 0 || (xml[br-1] != ' ' && xml[br-1] != '\n' && xml[br-1] != '\r')) {
                out += ' ';
            }
            // Copy <br (and the rest)
            size_t tagEnd = xml.find('>', br);
            if (tagEnd == std::string::npos) {
                out += xml.substr(br);
                break;
            }
            out += xml.substr(br, tagEnd - br + 1);
            i = tagEnd + 1;
        }
        return out;
    }

    // Simple XML to RTF converter for <b>, <i>, <del>, <ins>
    static std::string EscapeRtf(const char* text) {
        std::string out;
        for (const char* p = text; *p; ++p) {
            if (*p == '\\' || *p == '{' || *p == '}')
                out += '\\';
            out += *p;
        }
        return out;
    }

    static std::string XmlToRtf(const tinyxml2::XMLElement* elem) {
        std::string rtf;
        std::function<void(const tinyxml2::XMLNode*)> walk;
        walk = [&](const tinyxml2::XMLNode* node) {
            if (!node) return;
            if (auto text = node->ToText()) {
                rtf += EscapeRtf(text->Value());
            } else if (auto el = node->ToElement()) {
                std::string tag = el->Name();
                if (tag == "b") rtf += "\\b ";
                else if (tag == "i") rtf += "\\i ";
                else if (tag == "del") rtf += "\\strike ";
                else if (tag == "ins") rtf += "\\ul ";
                for (const tinyxml2::XMLNode* child = el->FirstChild(); child; child = child->NextSibling())
                    walk(child);
                if (tag == "b") rtf += "\\b0 ";
                else if (tag == "i") rtf += "\\i0 ";
                else if (tag == "del") rtf += "\\strike0 ";
                else if (tag == "ins") rtf += "\\ul0 ";
            }
        };
        for (const tinyxml2::XMLNode* child = elem->FirstChild(); child; child = child->NextSibling())
            walk(child);
        return rtf;
    }

    // Subclass procedure to block selection and interaction in RichEdit
    LRESULT CALLBACK NoSelectRichEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MOUSEMOVE:
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return 0; // Block interaction
            case WM_NCHITTEST:
                return HTTRANSPARENT;
        }
        return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, msg, wParam, lParam);
    }

    // Helper to extract plain text from XML (strip tags, keep line breaks and <br> tags)
    static std::vector<std::string> ExtractPlainLines(const tinyxml2::XMLElement* elem) {
        std::vector<std::string> lines;
        std::string current;
        std::function<void(const tinyxml2::XMLNode*)> walk;
        walk = [&](const tinyxml2::XMLNode* node) {
            if (!node) return;
            if (auto text = node->ToText()) {
                std::string val = text->Value();
                size_t start = 0, end;
                while ((end = val.find('\n', start)) != std::string::npos) {
                    current += val.substr(start, end - start);
                    lines.push_back(current);
                    current.clear();
                    start = end + 1;
                }
                current += val.substr(start);
            } else if (auto el = node->ToElement()) {
                std::string tag = el->Name();
                if (tag == "br") {
                    lines.push_back(current);
                    current.clear();
                    return;
                }
                for (const tinyxml2::XMLNode* child = el->FirstChild(); child; child = child->NextSibling())
                    walk(child);
            }
        };
        for (const tinyxml2::XMLNode* child = elem->FirstChild(); child; child = child->NextSibling())
            walk(child);
        if (!current.empty()) lines.push_back(current);
        if (lines.empty()) lines.push_back("");
        return lines;
    }

    // Helper to calculate required height for RichEdit content (lines * font height)
    static int CalcRichEditHeightFromLines(HFONT hFont, int numLines) {
        HDC hdcFont = GetDC(NULL);
        HFONT oldFont = (HFONT)SelectObject(hdcFont, hFont);
        TEXTMETRIC tm;
        GetTextMetrics(hdcFont, &tm);
        int px = tm.tmHeight * numLines + 4; // +4 for a little padding
        SelectObject(hdcFont, oldFont);
        ReleaseDC(NULL, hdcFont);
        return px;
    }

    // Helper to calculate required width for RichEdit content (max line width)
    static int CalcRichEditWidthFromLines(HFONT hFont, const std::vector<std::string>& lines) {
        HDC hdcFont = GetDC(NULL);
        HFONT oldFont = (HFONT)SelectObject(hdcFont, hFont);
        int maxWidth = 0;
        for (const auto& line : lines) {
            SIZE sz;
            GetTextExtentPoint32A(hdcFont, line.c_str(), (int)line.length(), &sz);
            if (sz.cx > maxWidth) maxWidth = sz.cx;
        }
        SelectObject(hdcFont, oldFont);
        ReleaseDC(NULL, hdcFont);
        return maxWidth + 8; // +8 for padding
    }

    // Helper to calculate required height for RichEdit content
    static int CalcRichEditHeight(HWND parent, HINSTANCE hInstance, int width, const char* rtf, HFONT hFont) {
        HWND hRich = CreateWindowExA(WS_EX_TOOLWINDOW, "RICHEDIT50W", "", WS_VISIBLE | WS_POPUP | ES_MULTILINE | ES_READONLY,
            -10000, -10000, width, 1000, parent, nullptr, hInstance, nullptr);
        if (!hRich) return 40;
        SendMessage(hRich, WM_SETFONT, (WPARAM)hFont, TRUE);
        SETTEXTEX st = { ST_DEFAULT, 1252 };
        SendMessageA(hRich, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)rtf);
        UpdateWindow(hRich);
        SendMessage(hRich, WM_PAINT, 0, 0);
        // FormatRange to measure
        FORMATRANGE fr = {0};
        HDC hdc = GetDC(hRich);
        RECT rc = {0, 0, width, 10000};
        fr.hdc = fr.hdcTarget = hdc;
        fr.rc = fr.rcPage = rc;
        fr.chrg.cpMin = 0;
        fr.chrg.cpMax = -1;
        LONG heightTwips = (LONG)SendMessage(hRich, EM_FORMATRANGE, FALSE, (LPARAM)&fr);
        SendMessage(hRich, EM_FORMATRANGE, FALSE, 0);
        ReleaseDC(hRich, hdc);
        if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
            char dbgTitle[128];
            snprintf(dbgTitle, sizeof(dbgTitle), "EM_FORMATRANGE: %ld twips", heightTwips);
            SetWindowTextA(parent, dbgTitle);
        }
        DestroyWindow(hRich);
        // EM_FORMATRANGE returns height in twips (1/1440 inch)
        int px = MulDiv(heightTwips, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 1440);
        if (px < 10) {
            HDC hdcFont = GetDC(NULL);
            HFONT oldFont = (HFONT)SelectObject(hdcFont, hFont);
            TEXTMETRIC tm;
            GetTextMetrics(hdcFont, &tm);
            px = tm.tmHeight + 2; // +2 for a little padding
            SelectObject(hdcFont, oldFont);
            ReleaseDC(NULL, hdcFont);
        }
        return px;
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
            // Preprocess for <br> spacing
            xmlContent = PreprocessXmlBrSpacing(xmlContent);
            if (xmlContent.empty() || doc.Parse(xmlContent.c_str()) != tinyxml2::XML_SUCCESS) {
                MessageBoxA(nullptr, "XML is empty or parsing failed.", "Debug", MB_OK);
                return nullptr;
            }
        } else {
            if (doc.LoadFile(xmlPath) != tinyxml2::XML_SUCCESS) {
                MessageBoxA(nullptr, xmlPath ? xmlPath : "(resource)", "Loaded XML Path", MB_OK);
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

        // Default background color (theme color)
        COLORREF bgColor = GetSysColor(COLOR_BTNFACE);
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
        if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
            MessageBoxA(nullptr, "Entering control creation loop", "Debug", MB_OK);
        }
        for (tinyxml2::XMLElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
            std::string tag = child->Name();
            if (tag == "Text") {
                const char* loc = child->Attribute("location");
                const char* text = child->Attribute("text");
                const char* fontName = child->Attribute("font");
                int fontSize = child->IntAttribute("fontsize", 9);
                const char* fontType = child->Attribute("fonttype");
                if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "XML fonttype attribute: %s", fontType ? fontType : "(null)");
                    MessageBoxA(NULL, msg, "XML Attribute Debug (Text)", MB_OK);
                }
                const char* fgStr = child->Attribute("foreground");
                const char* bgStr = child->Attribute("background");
                int x = 0, y = 0;
                if (loc) sscanf(loc, "%d,%d", &x, &y);
                HWND hText = CreateWindowExA(0, "STATIC", text ? text : "",
                    WS_CHILD | WS_VISIBLE,
                    x, y, 300, 20, hwnd, nullptr, hInstance, nullptr);
                if (hText) {
                    HFONT hFont = CreateUIFont(fontName, fontSize, fontType);
                    
                    pData->createdFonts.push_back(hFont);
                    SendMessage(hText, WM_SETFONT, (WPARAM)hFont, TRUE);
                    if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                    // Debug: Show actual font used
                    HFONT hFontUsed = (HFONT)SendMessage(hText, WM_GETFONT, 0, 0);
                    if (hFontUsed) {
                        LOGFONTA lf;
                        if (GetObjectA(hFontUsed, sizeof(lf), &lf)) {
                            MessageBoxA(NULL, lf.lfFaceName, "Actual Font Used (Text)", MB_OK);
                        }
                    }
                    }

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
            } else if (tag == "RichText") {
                if (!g_hRichEdit) g_hRichEdit = LoadLibraryA("Msftedit.dll");
                const char* loc = child->Attribute("location");
                const char* size = child->Attribute("size");
                const char* fontName = child->Attribute("font");
                int fontSize = child->IntAttribute("fontsize", 9);
                const char* fontType = child->Attribute("fonttype");
                if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "XML fonttype attribute: %s", fontType ? fontType : "(null)");
                    MessageBoxA(NULL, msg, "XML Attribute Debug (RichText)", MB_OK);
                }
                int x = 10, y = 10, w = 300, h = 40;
                if (loc && sscanf(loc, "%d,%d", &x, &y) != 2) { x = 10; y = 10; }
                // Determine the actual font to use (same logic as CreateUIFont)
                const char* actualFont = nullptr;
                if (fontType && strcmp(fontType, "monospace") == 0) {
                    if (fontName && FontExists(fontName)) {
                        actualFont = fontName;
                    } else if (FontExists("Consolas")) {
                        actualFont = "Consolas";
                    } else if (FontExists("Lucida Console")) {
                        actualFont = "Lucida Console";
                    } else {
                        actualFont = "Tahoma";
                    }
                } else {
                    if (fontName && FontExists(fontName)) {
                        actualFont = fontName;
                    } else {
                        actualFont = "Segoe UI";
                    }
                }
                HFONT hFont = CreateUIFont(fontName, fontSize, fontType);
                // Compose RTF header with font table and font size, using the actual font
                int fsTwips = fontSize * 2; // RTF font size is in half-points
                std::string rtf = "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 " + std::string(actualFont) + ";}}\\fs" + std::to_string(fsTwips) + " ";
                rtf += XmlToRtf(child);
                rtf += "}";
                // Extract plain lines for sizing
                std::vector<std::string> lines = ExtractPlainLines(child);
                h = CalcRichEditHeightFromLines(hFont, (int)lines.size());
                w = CalcRichEditWidthFromLines(hFont, lines);
                if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                    char dbgTitle[128];
                    snprintf(dbgTitle, sizeof(dbgTitle), "RichText: w=%d h=%d lines=%zu", w, h, lines.size());
                    SetWindowTextA(hwnd, dbgTitle);
                }
                HWND hRich = CreateWindowExA(0, "RICHEDIT50W", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY,
                    x, y, w, h, hwnd, nullptr, hInstance, nullptr);
                if (hRich) {
                    // Subclass to block selection and interaction
                    LONG_PTR oldProc = SetWindowLongPtr(hRich, GWLP_WNDPROC, (LONG_PTR)NoSelectRichEditProc);
                    SetWindowLongPtr(hRich, GWLP_USERDATA, oldProc);
                    pData->createdFonts.push_back(hFont);
                    SendMessage(hRich, WM_SETFONT, (WPARAM)hFont, TRUE);
                    // Debug: Show actual font used
                    HFONT hFontUsed = (HFONT)SendMessage(hRich, WM_GETFONT, 0, 0);
                    if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                        if (hFontUsed) {
                        LOGFONTA lf;
                        if (GetObjectA(hFontUsed, sizeof(lf), &lf)) {
                            MessageBoxA(NULL, lf.lfFaceName, "Actual Font Used (RichText)", MB_OK);
                        }
                    }
                    }
                    SETTEXTEX st = { ST_DEFAULT, 1252 }; // CP 1252 (ANSI)
                    SendMessageA(hRich, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)rtf.c_str());
                    // Set background color using EM_SETBKGNDCOLOR
                    SendMessage(hRich, EM_SETBKGNDCOLOR, 0, pData->bgColor);
                    // Set background and foreground color info for RichEdit
                    ControlColorInfo info;
                    info.bg = pData->bgColor;
                    info.hBrush = CreateSolidBrush(info.bg);
                    info.fg = RGB(0,0,0); // default to black
                    g_controlColorMap[hRich] = info;
                    // Ensure RichEdit is visible and on top
                    SetWindowPos(hRich, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
                }
            } else if (tag == "Button") {
                const char* loc = child->Attribute("location");
                const char* size = child->Attribute("size");
                const char* text = child->Attribute("text");
                const char* onclick = child->Attribute("onclick");
                const char* fontName = child->Attribute("font");
                int fontSize = child->IntAttribute("fontsize", 9);
                const char* fontType = child->Attribute("fonttype");
                if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "XML fonttype attribute: %s", fontType ? fontType : "(null)");
                    MessageBoxA(NULL, msg, "XML Attribute Debug (Button)", MB_OK);
                }
                const char* fgStr = child->Attribute("foreground");
                const char* bgStr = child->Attribute("background");
                int x = 0, y = 0, w = 80, h = 25;
                if (loc) sscanf(loc, "%d,%d", &x, &y);
                if (size) sscanf(size, "%d,%d", &w, &h);
                HWND btnHwnd = CreateWindowExA(0, "BUTTON", text ? text : "Button",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    x, y, w, h, hwnd, nullptr, hInstance, nullptr);

                if (btnHwnd) {
                    HFONT hFont = CreateUIFont(fontName, fontSize, fontType);
                    pData->createdFonts.push_back(hFont);
                    SendMessage(btnHwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
                    // Debug: Show actual font used
                    HFONT hFontUsed = (HFONT)SendMessage(btnHwnd, WM_GETFONT, 0, 0);
                    if (GetCommandLineA() && strstr(GetCommandLineA(), "--Win32XmlUI-Debug")) {
                        if (hFontUsed) {
                            LOGFONTA lf;
                            if (GetObjectA(hFontUsed, sizeof(lf), &lf)) {
                                MessageBoxA(NULL, lf.lfFaceName, "Actual Font Used (Button)", MB_OK);
                            }
                        }
                    }

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