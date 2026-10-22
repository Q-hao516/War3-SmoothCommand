@echo off
REM ===========================================================================
REM  War3 SmoothCommand - one-click build (32-bit MSVC x86)
REM
REM  Outputs:
REM    build\SlotPatch.dll      slot-hash patch injected into Game.dll
REM    build\ijl15_proxy.dll    ijl15 proxy DLL (works when launching War3.exe)
REM    build\CavInject.exe      portable injector
REM
REM  Usage: double-click, or run build.bat from a command prompt.
REM ===========================================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "VCVARS="

REM ---- 1) locate Visual Studio -------------------------------------------
set "PFX86=%ProgramFiles(x86)%"
set "VSWHERE=%PFX86%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARS (
    for %%p in (
        "D:\Program Files\Microsoft Visual Studio\18\Community"
        "D:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
    ) do (
        if not defined VCVARS if exist "%%~p\VC\Auxiliary\Build\vcvarsall.bat" set "VCVARS=%%~p\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARS (
    echo [ERROR] vcvarsall.bat not found.
    echo         Install Visual Studio with the "Desktop development with C++" workload.
    exit /b 1
)
echo [env] vcvarsall = %VCVARS%

call "%VCVARS%" x86 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvarsall x86 failed
    exit /b 1
)
where cl >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cl.exe not on PATH after vcvarsall
    exit /b 1
)
echo [env] cl:
cl 2>&1 | findstr /C:"Version"
echo.

if not exist build      mkdir build
if not exist build\obj  mkdir build\obj

set CFLAGS=/nologo /O2 /W3 /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS
set FAILED=0

REM ---- 2) SlotPatch.dll ---------------------------------------------------
echo [1/3] building SlotPatch.dll ...
cl %CFLAGS% /LD /Fo:build\obj\SlotPatch.obj ^
   src\slotpatch\SlotPatch.cpp ^
   /link /OUT:build\SlotPatch.dll /MACHINE:X86
if errorlevel 1 (
    echo [FAIL] SlotPatch.dll
    set FAILED=1
) else (
    echo [OK]   build\SlotPatch.dll
)
echo.

REM ---- 3) ijl15 proxy DLL -------------------------------------------------
echo [2/3] building ijl15_proxy.dll ...
cl %CFLAGS% /LD /Fo:build\obj\ijl15_proxy.obj ^
   src\proxy\ijl15_proxy.cpp ^
   /link /OUT:build\ijl15_proxy.dll /MACHINE:X86 /IMPLIB:build\obj\ijl15_proxy.lib /PDB:build\obj\ijl15_proxy.pdb
if errorlevel 1 (
    echo [FAIL] ijl15_proxy.dll
    set FAILED=1
) else (
    echo [OK]   build\ijl15_proxy.dll
)
echo.

REM ---- 4) CavInject.exe ---------------------------------------------------
echo [3/3] building CavInject.exe ...
cl %CFLAGS% /Fo:build\obj\CavInject.obj ^
   src\injector\CavInject.cpp ^
   /link /OUT:build\CavInject.exe /MACHINE:X86
if errorlevel 1 (
    echo [FAIL] CavInject.exe
    set FAILED=1
) else (
    echo [OK]   build\CavInject.exe
)
echo.

REM ---- 5) summary ---------------------------------------------------------
echo ============================================================
if "%FAILED%"=="1" (
    echo [RESULT] build FAILED - see [FAIL] above
    exit /b 1
)
echo [RESULT] all targets built successfully
for %%f in (build\SlotPatch.dll build\ijl15_proxy.dll build\CavInject.exe) do (
    if exist "%%f" ( echo   %%~ff ) else ( echo   %%f  MISSING )
)
echo ============================================================
exit /b 0
