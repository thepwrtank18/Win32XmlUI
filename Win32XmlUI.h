#pragma once
#include <windows.h>
#include <functional>

#ifdef _WIN32
  #ifdef WIN32XMLUI_STATIC
    #define WIN32XMLUI_API
  #elif defined(WIN32XMLUI_EXPORTS)
    #define WIN32XMLUI_API __declspec(dllexport)
  #else
    #define WIN32XMLUI_API __declspec(dllimport)
  #endif
#else
  #define WIN32XMLUI_API
#endif

WIN32XMLUI_API HWND CreateUIFromXML(const char* xmlPath, HINSTANCE hInstance, int nCmdShow, int resourceId);
WIN32XMLUI_API void RegisterCallback(const char* name, std::function<void(HWND)> func);
WIN32XMLUI_API bool SetElementProperty(HWND hwnd, const char* elementId, const char* propertyName, const char* value);