@echo off
rem
rem AcidBadd 303 -- Windows installer.
rem
rem Double-click this file or run it from a terminal. It:
rem   1. builds the Release binaries if they are missing (needs CMake and
rem      Visual Studio's C++ tools on PATH),
rem   2. asks for administrator rights,
rem   3. installs the standalone app to   %ProgramFiles%\AcidBadd 303
rem      and the VST3 plugin to           %CommonProgramFiles%\VST3,
rem   4. creates Start Menu + Desktop shortcuts and registers an uninstaller
rem      that shows up under Settings -> Apps ("Add or remove programs").
rem
rem To remove everything again, run uninstall-win.bat (or uninstall from
rem Settings -> Apps).
rem
setlocal

set "HERE=%~dp0"
set "STANDALONE=%HERE%build\AcidBadd_artefacts\Release\Standalone\AcidBadd 303.exe"
set "VST3SRC=%HERE%build\AcidBadd_artefacts\Release\VST3\AcidBadd 303.vst3"

set "INSTDIR=%ProgramFiles%\AcidBadd 303"
set "VST3DIR=%CommonProgramFiles%\VST3"
set "SHORTCUT=AcidBadd 303.lnk"
set "STARTMENU=%ProgramData%\Microsoft\Windows\Start Menu\Programs"
set "REGKEY=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AcidBadd303"

rem ---- 1) Build first, as the normal user, if the artefacts are missing.
if exist "%STANDALONE%" if exist "%VST3SRC%" goto have_build
echo Build artefacts not found -- building Release first...
cmake -S "%HERE%." -B "%HERE%build"
if errorlevel 1 (
    echo CMake configure failed.
    pause
    exit /b 1
)
cmake --build "%HERE%build" --config Release
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)
if not exist "%STANDALONE%" (
    echo Build finished but the standalone is still missing -- see output above.
    pause
    exit /b 1
)
if not exist "%VST3SRC%" (
    echo Build finished but the VST3 is still missing -- see output above.
    pause
    exit /b 1
)
:have_build

rem ---- 2) Installing to Program Files needs administrator rights.
net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights to install...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

rem ---- 3) Copy the files.
echo Installing standalone app to "%INSTDIR%"...
if not exist "%INSTDIR%" mkdir "%INSTDIR%"
copy /y "%STANDALONE%" "%INSTDIR%\AcidBadd 303.exe" >nul
if errorlevel 1 (
    echo Failed to copy the standalone app.
    pause
    exit /b 1
)
copy /y "%HERE%uninstall-win.bat" "%INSTDIR%\uninstall.bat" >nul
if errorlevel 1 (
    echo Failed to copy the uninstaller.
    pause
    exit /b 1
)

echo Installing VST3 plugin to "%VST3DIR%"...
robocopy "%VST3SRC%" "%VST3DIR%\AcidBadd 303.vst3" /e /njh /njs /ndl /nfl /nc /ns >nul
if errorlevel 8 (
    echo Failed to copy the VST3 plugin.
    pause
    exit /b 1
)

rem ---- 4) Start Menu + Desktop shortcuts.
echo Creating shortcuts...
powershell -NoProfile -Command "$ws = New-Object -ComObject WScript.Shell; foreach ($p in '%STARTMENU%\%SHORTCUT%', '%Public%\Desktop\%SHORTCUT%') { $s = $ws.CreateShortcut($p); $s.TargetPath = '%INSTDIR%\AcidBadd 303.exe'; $s.WorkingDirectory = '%INSTDIR%'; $s.Save() }"

rem ---- 5) Register the uninstaller in "Add or remove programs".
echo Registering uninstaller...
reg add "%REGKEY%" /v DisplayName     /t REG_SZ /d "AcidBadd 303" /f >nul
reg add "%REGKEY%" /v DisplayVersion  /t REG_SZ /d "1.0.0" /f >nul
reg add "%REGKEY%" /v Publisher       /t REG_SZ /d "AcidBadd" /f >nul
reg add "%REGKEY%" /v InstallLocation /t REG_SZ /d "%INSTDIR%" /f >nul
reg add "%REGKEY%" /v DisplayIcon     /t REG_SZ /d "%INSTDIR%\AcidBadd 303.exe" /f >nul
reg add "%REGKEY%" /v UninstallString /t REG_SZ /d "\"%INSTDIR%\uninstall.bat\"" /f >nul
reg add "%REGKEY%" /v NoModify /t REG_DWORD /d 1 /f >nul
reg add "%REGKEY%" /v NoRepair /t REG_DWORD /d 1 /f >nul

echo.
echo Installed:
echo   Standalone : %INSTDIR%\AcidBadd 303.exe
echo   VST3       : %VST3DIR%\AcidBadd 303.vst3
echo   Shortcuts  : Start Menu and Desktop
echo   Uninstall  : %INSTDIR%\uninstall.bat  (also in "Add or remove programs")
echo.
choice /c YN /m "Launch AcidBadd 303 now"
if not errorlevel 2 start "" "%INSTDIR%\AcidBadd 303.exe"
endlocal
