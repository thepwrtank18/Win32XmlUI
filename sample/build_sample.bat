@echo off
setlocal

REM Add TDM-GCC to PATH
set PATH=C:\TDM-GCC-64\bin;%PATH%

REM Build resource
windres --target=pe-x86-64 .\resource.rc -O coff -o resource.res

REM Build sample client (static link)
g++ -std=c++17 -I.. main.cpp resource.res -L..\lib -l:Win32XmlUI.a -lgdi32 -lcomctl32 -DWIN32XMLUI_STATIC -o Win32XmlSample_static.exe -mwindows
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_static.exe"

REM Build sample client (DLL link)
g++ -std=c++17 -I.. main.cpp resource.res -L..\lib -l:Win32XmlUI.dll.a -lgdi32 -lcomctl32 -o Win32XmlSample_dll.exe -mwindows
call :retry_mt "..\visualstyling.manifest" "Win32XmlSample_dll.exe"
REM Copy Win32XmlUI.dll to the sample directory
copy ..\lib\Win32XmlUI.dll .

echo Sample builds complete: Win32XmlSample_static.exe and Win32XmlSample_dll.exe 

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
ping -n 2 127.0.0.1 >nul
goto mt_try_again
:mt_fail
echo Failed to update manifest for %EXE% after %RETRIES% attempts.
exit /b 1
:mt_done
exit /b 0 