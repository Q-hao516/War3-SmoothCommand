@echo off
REM ===========================================================================
REM  package.bat - build everything and assemble the installer payload
REM
REM  Steps:
REM    1) build.bat             compile SlotPatch.dll / ijl15_proxy.dll / CavInject.exe
REM    2) sync into payload\    copy build outputs over the shipped binaries
REM    3) optional: build the setup EXE with Inno Setup (ISCC.exe)
REM
REM  Usage: double-click, or run package.bat
REM ===========================================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"

REM ---- 1) build -------------------------------------------------------------
call build.bat
if errorlevel 1 (
    echo [ERROR] build failed, packaging aborted
    exit /b 1
)
echo.

REM ---- 2) sync build outputs into installer\payload -------------------------
set "PAYLOAD=installer\payload"
if not exist "%PAYLOAD%" mkdir "%PAYLOAD%"
echo [sync] copying build outputs into %PAYLOAD%\
copy /Y build\SlotPatch.dll    "%PAYLOAD%\SlotPatch.dll"    >nul
copy /Y build\ijl15_proxy.dll  "%PAYLOAD%\ijl15_proxy.dll"  >nul
copy /Y build\CavInject.exe    "%PAYLOAD%\CavInject.exe"    >nul
echo   %PAYLOAD%\SlotPatch.dll
echo   %PAYLOAD%\ijl15_proxy.dll
echo   %PAYLOAD%\CavInject.exe
echo.

REM ---- 3) installer (optional) ----------------------------------------------
set "ISCC="
for %%p in (
    "D:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "D:\Program Files\Inno Setup 6\ISCC.exe"
    "C:\Program Files\Inno Setup 6\ISCC.exe"
) do (
    if not defined ISCC if exist "%%~p" set "ISCC=%%~p"
)
if not defined ISCC (
    echo [skip] Inno Setup not found - installer EXE not rebuilt.
    echo        Install Inno Setup 6 and re-run package.bat to produce dist\.
    exit /b 0
)
echo [3/3] building installer with %ISCC% ...
pushd installer
"%ISCC%" War3Unlock.iss
set RC=!ERRORLEVEL!
popd
if not "!RC!"=="0" (
    echo [ERROR] ISCC failed with exit code !RC!
    exit /b !RC!
)
echo [OK] installer written to installer\dist\
exit /b 0
