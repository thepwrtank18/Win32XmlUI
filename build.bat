@echo off
setlocal

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

mkdir lib

ECHO Build 32-bit static library

g++ -c -std=c++17 -I. -m32 Win32XmlUI.cpp tinyxml2.cpp
if exist "Win32XmlUI_x86.a" (
  move "Win32XmlUI_x86.a" "lib\Win32XmlUI_x86.a"
)

ar rcs lib\Win32XmlUI_x86.a Win32XmlUI.o tinyxml2.o

del Win32XmlUI.o tinyxml2.o

ECHO Build 32-bit DLL

g++ -std=c++17 -I. -shared -m32 -o lib\Win32XmlUI_x86.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,lib\Win32XmlUI_x86.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS
if exist "Win32XmlUI_x86.dll.a" (
  move "Win32XmlUI_x86.dll.a" "lib\Win32XmlUI_x86.dll.a"
)

ECHO Build 64-bit static library

g++ -c -std=c++17 -I. -m64 Win32XmlUI.cpp tinyxml2.cpp
if exist "Win32XmlUI_x64.a" (
  move "Win32XmlUI_x64.a" "lib\Win32XmlUI_x64.a"
)

ar rcs lib\Win32XmlUI_x64.a Win32XmlUI.o tinyxml2.o

del Win32XmlUI.o tinyxml2.o

ECHO Build 64-bit DLL

g++ -std=c++17 -I. -shared -m64 -o lib\Win32XmlUI_x64.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,lib\Win32XmlUI_x64.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS
if exist "Win32XmlUI_x64.dll.a" (
  move "Win32XmlUI_x64.dll.a" "lib\Win32XmlUI_x64.dll.a"
)

echo Library build complete. Use sample/build_sample.bat to build the example client. 
