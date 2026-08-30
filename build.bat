@echo off
rem ---------------------------------------------------------------------------
rem  DoomLegacy arcade cabinet -- build for Windows.
rem
rem  Double-click this file, or run it from a command prompt.  It calls
rem  tools\build.ps1, which detects your CPU, finds MSYS2, checks that the
rem  compiler and libraries are installed, and builds.
rem
rem  This wrapper exists so that nobody has to know about PowerShell execution
rem  policy: -ExecutionPolicy Bypass applies to this one invocation only and
rem  changes nothing on the machine.
rem
rem  Arguments are passed straight through:
rem      build.bat -Deps           just say what is needed
rem      build.bat -InstallDeps    install the MSYS2 packages
rem      build.bat -Clean          clean first
rem ---------------------------------------------------------------------------

setlocal
set "PS1=%~dp0tools\build.ps1"

if not exist "%PS1%" (
    echo error: %PS1% not found.
    echo        Run this from the top of a DoomLegacy checkout.
    goto :hold
)

where powershell >nul 2>&1
if errorlevel 1 (
    echo error: PowerShell was not found on this system.
    echo        Windows 7 and later ship with it; on a stripped-down install,
    echo        build from an MSYS2 shell instead - see docs/arcade/building.md
    goto :hold
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" %*
set "RC=%ERRORLEVEL%"

:hold
rem Keep the window open when double-clicked, so the message is readable.
rem cmdcmdline contains /c only when the shell was started for this file.
echo %cmdcmdline% | find /i "/c" >nul && pause
exit /b %RC%
