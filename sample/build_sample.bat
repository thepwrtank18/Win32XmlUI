@echo off
setlocal

echo If you have Defender enabled, THIS WILL FAIL!!!!!!
echo Defender will lock onto an EXE and not let go, so it can't put the manifest file in.
echo Please put an exclusion for the build folder.

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

REM Add Windows 11 SDK to PATH
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64;%PATH%

ECHO Build resource
windres --target=pe-x86-64 .\resource.rc -O coff -o resource_x64.res
windres --target=pe-i386 .\resource.rc -O coff -o resource_x86.res

ECHO Build 32-bit sample client (static link)
g++ -std=c++17 -I.. -m32 main.cpp resource_x86.res -L..\lib -l:Win32XmlUI_x86.a -lgdi32 -lcomctl32 -DWIN32XMLUI_STATIC -o Win32XmlSample_static_x86.exe -mwindows
ping -n 2 127.0.0.1 >nul
mt -manifest "..\visualstyling.manifest" -outputresource:"Win32XmlSample_static_x86.exe"

ECHO Build 32-bit sample client (DLL link)
g++ -std=c++17 -I.. -m32 main.cpp resource_x86.res -L..\lib -l:Win32XmlUI_x86.dll.a -lgdi32 -lcomctl32 -o Win32XmlSample_dll_x86.exe -mwindows
ping -n 2 127.0.0.1 >nul
mt -manifest "..\visualstyling.manifest" -outputresource:"Win32XmlSample_dll_x86.exe"

REM Copy 32-bit DLL to the sample directory
copy ..\lib\Win32XmlUI_x86.dll .

ECHO Build 64-bit sample client (static link)
g++ -std=c++17 -I.. -m64 main.cpp resource_x64.res -L..\lib -l:Win32XmlUI_x64.a -lgdi32 -lcomctl32 -DWIN32XMLUI_STATIC -o Win32XmlSample_static_x64.exe -mwindows
ping -n 2 127.0.0.1 >nul
mt -manifest "..\visualstyling.manifest" -outputresource:"Win32XmlSample_static_x64.exe"

ECHO Build 64-bit sample client (DLL link)
g++ -std=c++17 -I.. -m64 main.cpp resource_x64.res -L..\lib -l:Win32XmlUI_x64.dll.a -lgdi32 -lcomctl32 -o Win32XmlSample_dll_x64.exe -mwindows
mt -manifest "..\visualstyling.manifest" -outputresource:"Win32XmlSample_dll_x64.exe"

REM Copy 64-bit DLL to the sample directory
copy ..\lib\Win32XmlUI_x64.dll .

echo Sample builds complete

goto :eof