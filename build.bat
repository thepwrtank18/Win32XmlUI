@echo off
setlocal

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

::REM Build static library
::
::g++ -c -std=c++17 -I. Win32XmlUI.cpp tinyxml2.cpp
::ar rcs lib\Win32XmlUI_x64.a Win32XmlUI.o tinyxml2.o
::
::REM Build DLL
::
::g++ -std=c++17 -I. -shared -o lib\Win32XmlUI_x64.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,Win32XmlUI_x64.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS
::
::REM Move the dll.a file to the lib directory
::move Win32XmlUI_x64.dll.a lib\Win32XmlUI_x64.dll.a

REM Build 32-bit static library

g++ -c -std=c++17 -I. -m32 Win32XmlUI.cpp tinyxml2.cpp
ar rcs lib\Win32XmlUI_x86.a Win32XmlUI.o tinyxml2.o

del Win32XmlUI.o tinyxml2.o

REM Build 32-bit DLL

g++ -std=c++17 -I. -shared -m32 -o lib\Win32XmlUI_x86.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,lib\Win32XmlUI_x86.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS
::move Win32XmlUI_x86.dll.a lib\Win32XmlUI_x86.dll.a

REM Build 64-bit static library

g++ -c -std=c++17 -I. -m64 Win32XmlUI.cpp tinyxml2.cpp
ar rcs lib\Win32XmlUI_x64.a Win32XmlUI.o tinyxml2.o

del Win32XmlUI.o tinyxml2.o

REM Build 64-bit DLL

g++ -std=c++17 -I. -shared -m64 -o lib\Win32XmlUI_x64.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,lib\Win32XmlUI_x64.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS
::move Win32XmlUI_x64.dll.a lib\Win32XmlUI_x64.dll.a

echo Library build complete. Use sample/build_sample.bat to build the example client. 