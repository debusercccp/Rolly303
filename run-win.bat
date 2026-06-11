@echo off
rem
rem AcidBadd 303 -- Windows installer.
rem
rem Double-click this file or run it from a terminal. It:
rem   1. builds the Release binaries if they are missing -- CMake and the
rem      Microsoft C++ build tools are found automatically, and installed
rem      with winget if absent (one-time),
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
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

rem ---- 1) Build first, as the normal user, if the artefacts are missing.
if exist "%STANDALONE%" if exist "%VST3SRC%" goto have_build

echo Build artefacts not found -- building Release first...

rem -- 1a) Locate CMake (PATH, standalone install, or the copy that ships
rem        with the C++ build tools). Install it with winget if missing.
set "CMAKE="
call :find_cmake
if defined CMAKE goto have_cmake
echo CMake not found -- installing it now (one-time)...
winget install --id Kitware.CMake -e --silent --accept-package-agreements --accept-source-agreements
call :find_cmake
if defined CMAKE goto have_cmake
echo Could not find or install CMake automatically.
echo Install it manually from  https://cmake.org/download/  (during setup
echo choose "Add CMake to the PATH"), then run this script again.
pause
exit /b 1
:have_cmake
echo Using CMake: %CMAKE%

rem -- 1b) Make sure the Microsoft C++ build tools (compiler + Windows SDK)
rem        are present; install them with winget if not. Command-line tools
rem        only, no IDE -- but it is a one-time multi-GB download, so this
rem        step can take a while on the first run.
call :find_cxx
if defined HAVE_CXX goto have_cxx
echo C++ build tools not found -- installing them now (one-time, a few GB)...
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --silent --accept-package-agreements --accept-source-agreements --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
call :find_cxx
if defined HAVE_CXX goto have_cxx
echo Could not find or install the C++ build tools automatically.
echo Install them manually from  https://aka.ms/vs/17/release/vs_BuildTools.exe
echo (pick the "Desktop development with C++" workload), then run this
echo script again.
pause
exit /b 1
:have_cxx

"%CMAKE%" -S "%HERE%." -B "%HERE%build"
if errorlevel 1 (
    echo CMake configure failed.
    pause
    exit /b 1
)
"%CMAKE%" --build "%HERE%build" --config Release
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
exit /b 0

rem ---------------------------------------------------------------------
rem Subroutines
rem ---------------------------------------------------------------------

:find_cmake
rem Sets CMAKE to the cmake.exe to use, or leaves it empty if none found.
where cmake >nul 2>&1
if not errorlevel 1 (
    set "CMAKE=cmake"
    goto :eof
)
if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
    set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
    goto :eof
)
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) do set "CMAKE=%%i"
)
goto :eof

:find_cxx
rem Sets HAVE_CXX if the Microsoft C++ compiler toolset is installed.
set "HAVE_CXX="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "HAVE_CXX=%%i"
)
goto :eof
