@echo off
setlocal

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

REM Build static library

g++ -c -std=c++17 -I. Win32XmlUI.cpp tinyxml2.cpp
ar rcs lib\Win32XmlUI.a Win32XmlUI.o tinyxml2.o

REM Build DLL

g++ -std=c++17 -I. -shared -o lib\Win32XmlUI.dll Win32XmlUI.cpp tinyxml2.cpp -Wl,--out-implib,Win32XmlUI.dll.a -lgdi32 -lcomctl32 -DWIN32XMLUI_EXPORTS

REM Move the dll.a file to the lib directory
move Win32XmlUI.dll.a lib\Win32XmlUI.dll.a

echo Library build complete. Use sample/build_sample.bat to build the example client. 