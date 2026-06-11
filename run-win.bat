@echo off
rem
rem Launch the AcidBadd 303 standalone on Windows.
rem
rem Double-click this file or run it from a terminal. It expects the Release
rem standalone to have been built already (see instructions below).
rem
setlocal

set "HERE=%~dp0"
set "APP=%HERE%build\AcidBadd_artefacts\Release\Standalone\AcidBadd 303.exe"

if not exist "%APP%" (
    echo Standalone not found at:
    echo   %APP%
    echo Build it first:
    echo   cmake -B build
    echo   cmake --build build --config Release
    pause
    exit /b 1
)

echo Launching AcidBadd 303 -- close its window to quit.
start "" "%APP%" %*
endlocal
