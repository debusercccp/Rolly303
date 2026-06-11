@echo off
rem
rem AcidBadd 303 -- uninstaller.
rem
rem Removes everything run-win.bat installed: the standalone app, the VST3
rem plugin, the shortcuts and the "Add or remove programs" entry. A copy of
rem this file is installed as  %ProgramFiles%\AcidBadd 303\uninstall.bat,
rem so it works both from the project folder and from Settings -> Apps.
rem
setlocal

set "INSTDIR=%ProgramFiles%\AcidBadd 303"
set "VST3=%CommonProgramFiles%\VST3\AcidBadd 303.vst3"
set "SHORTCUT=AcidBadd 303.lnk"
set "STARTMENU=%ProgramData%\Microsoft\Windows\Start Menu\Programs"
set "REGKEY=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\AcidBadd303"

net session >nul 2>&1
if errorlevel 1 (
    echo Requesting administrator rights to uninstall...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

choice /c YN /m "Remove AcidBadd 303 from this computer"
if errorlevel 2 exit /b 0

echo Removing VST3 plugin...
if exist "%VST3%" rd /s /q "%VST3%"

echo Removing shortcuts...
del /q "%STARTMENU%\%SHORTCUT%" 2>nul
del /q "%Public%\Desktop\%SHORTCUT%" 2>nul

echo Removing "Add or remove programs" entry...
reg delete "%REGKEY%" /f >nul 2>&1

echo Removing "%INSTDIR%"...
echo Done -- AcidBadd 303 has been uninstalled.
timeout /t 4 >nul

rem The last line removes the folder that may contain this very script. The
rem (goto) trick ends batch processing first, so deleting ourselves succeeds.
(goto) 2>nul & rd /s /q "%INSTDIR%"
