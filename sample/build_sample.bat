@echo off
setlocal

echo If you have Defender enabled, THIS WILL FAIL!!!!!!
echo Defender will lock onto an EXE and not let go, so it can't put the manifest file in.
echo Please put an exclusion for the build folder.

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

REM Build resource
windres --target=pe-x86-64 .\resource.rc -O coff -o resource_x64.res
windres --target=pe-i386 .\resource.rc -O coff -o resource_x86.res

REM Build 32-bit sample client (static link)
g++ -std=c++17 -I.. -m32 main.cpp resource_x86.res -L..\lib -l:Win32XmlUI_x86.a -lgdi32 -lcomctl32 -DWIN32XMLUI_STATIC -o Win32XmlSample_static_x86.exe -mwindows
ping -n 2 127.0.0.1 >nul
REM this is retarded
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_static_x86.exe"

REM Build 32-bit sample client (DLL link)
g++ -std=c++17 -I.. -m32 main.cpp resource_x86.res -L..\lib -l:Win32XmlUI_x86.dll.a -lgdi32 -lcomctl32 -o Win32XmlSample_dll_x86.exe -mwindows
ping -n 2 127.0.0.1 >nul
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_dll_x86.exe"
REM Copy 32-bit DLL to the sample directory
copy ..\lib\Win32XmlUI_x86.dll .

REM Build 64-bit sample client (static link)
g++ -std=c++17 -I.. -m64 main.cpp resource_x64.res -L..\lib -l:Win32XmlUI_x64.a -lgdi32 -lcomctl32 -DWIN32XMLUI_STATIC -o Win32XmlSample_static_x64.exe -mwindows
ping -n 2 127.0.0.1 >nul
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_static_x64.exe"

REM Build 64-bit sample client (DLL link)
g++ -std=c++17 -I.. -m64 main.cpp resource_x64.res -L..\lib -l:Win32XmlUI_x64.dll.a -lgdi32 -lcomctl32 -o Win32XmlSample_dll_x64.exe -mwindows
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_dll_x64.exe"
REM Copy 64-bit DLL to the sample directory
copy ..\lib\Win32XmlUI_x64.dll .

echo Sample builds complete

goto :eof

:retry_mt
REM %1 = manifest file, %2 = exe file
setlocal
set "MANIFEST=%~1"
set "EXE=%~2"
set "RETRIES=5"
set "COUNT=0"
:mt_try_again
mt -manifest "%MANIFEST%" -outputresource:"%EXE%;1"
if %ERRORLEVEL%==0 goto mt_done
set /a COUNT+=1
if %COUNT% GEQ %RETRIES% goto mt_fail
REM Wait 1 second before retrying
ping -n 5 127.0.0.1 >nul
goto mt_try_again
:mt_fail
echo Failed to update manifest for %EXE% after %RETRIES% attempts.
exit /b 1
:mt_done
exit /b 0 