@echo off
setlocal enabledelayedexpansion

:: Detect host architecture (query machine-level, not process-level which may be emulated)
for /f "tokens=3" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE 2^>nul ^| findstr /i "PROCESSOR_ARCHITECTURE"') do set MACHINE_ARCH=%%a
if /i "%MACHINE_ARCH%"=="ARM64" (
    set BUILD_ARCH=arm64
) else (
    set BUILD_ARCH=x64
)

:: Handle clean command (clean logic lives in the :clean subroutine at the end
:: of this file, shared with the rebuild path)
if "%1"=="clean" (
    call :clean
    exit /b 0
)

:: Ensure zlib-ng submodule is initialized (fresh clones without --recurse-submodules)
:: Note: flat goto structure, not nested blocks - exit /b inside a nested block
:: with trailing commands in the outer block does not propagate the exit code.
if exist lib\zlib-ng\CMakeLists.txt goto :submodule_ok
echo zlib-ng submodule not initialized - running git submodule update...
where git >nul 2>nul
if !ERRORLEVEL! NEQ 0 (
    echo ERROR: lib\zlib-ng is empty and git was not found on PATH.
    echo Install Git for Windows, then run: git submodule update --init lib/zlib-ng
    exit /b 1
)
git submodule update --init lib/zlib-ng
if !ERRORLEVEL! NEQ 0 (
    echo ERROR: Failed to initialize zlib-ng submodule.
    echo Run manually from the repo root: git submodule update --init lib/zlib-ng
    exit /b 1
)
if not exist lib\zlib-ng\CMakeLists.txt (
    echo ERROR: lib\zlib-ng\CMakeLists.txt still missing after submodule update.
    exit /b 1
)
:submodule_ok

:: Handle test command
if "%1"=="test" (
    set IS_TEST=1
    if "%2"=="--build-only" set TEST_BUILD_ONLY=1
    goto :after_mode_check
)

if "%1"=="rebuild" (
    call :clean
    set REBUILD_ZLIBNG=1
)

:: Handle debug mode
set IS_DEBUG=0
if "%1"=="debug" set IS_DEBUG=1
set EXE_NAME=GitSmarter.exe
set FLAGS=/std:c++20 /EHsc /W4 /I include /I lib\zlib-ng /I lib\zlib-ng\build-!BUILD_ARCH!

:: Set debug or release flags
if !IS_DEBUG!==1 (
    set EXE_NAME=GitSmarterDebug.exe
    set FLAGS=!FLAGS! /Od /Zi /DGITSMARTER_DEBUG /DEBUG:FULL /Fe:!EXE_NAME!
    echo Building in Debug mode: !EXE_NAME!
) else (
    set FLAGS=!FLAGS! /O2 /Fe:!EXE_NAME!
    echo Building in Release mode: !EXE_NAME!
)

:after_mode_check

:: Auto-detect Visual Studio (2022 = "2022", 2026 = "18")
set "VS_PATH="
for %%p in (
    "C:\Program Files\Microsoft Visual Studio\18\Enterprise"
    "C:\Program Files\Microsoft Visual Studio\18\Professional"
    "C:\Program Files\Microsoft Visual Studio\18\Community"
    "C:\Program Files\Microsoft Visual Studio\18\BuildTools"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional"
    "C:\Program Files\Microsoft Visual Studio\2022\Community"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
) do (
    if exist "%%~p\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VS_PATH=%%~p"
        goto :found_vs
    )
)
echo ERROR: Visual Studio 2022 or 2026 not found
exit /b 1

:found_vs
echo Target architecture: !BUILD_ARCH!
if not defined VSCMD_VER (
    call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" !BUILD_ARCH! >nul 2>&1
)

:: Create build directory for cached objects
if not exist build mkdir build

:: Compiler settings
set SOURCES=src\main.cpp
set LIBS=user32.lib gdi32.lib d2d1.lib dwrite.lib ole32.lib shell32.lib dwmapi.lib winhttp.lib shlwapi.lib dxguid.lib bcrypt.lib
set ZLIBNG_LIB=lib\zlib-ng\build-!BUILD_ARCH!\Release\zlibstatic.lib

:: Check if zlib-ng needs to be built
set BUILD_ZLIBNG=0
if defined REBUILD_ZLIBNG set BUILD_ZLIBNG=1
if not exist "%ZLIBNG_LIB%" set BUILD_ZLIBNG=1

:: Build zlib-ng if needed
if !BUILD_ZLIBNG!==1 (
    echo Building zlib-ng with CMake for !BUILD_ARCH!...
    pushd lib\zlib-ng
    cmake -B build-!BUILD_ARCH! -A !BUILD_ARCH! -DZLIB_COMPAT=ON -DWITH_GZFILEOP=OFF -DBUILD_SHARED_LIBS=OFF -DZLIB_ENABLE_TESTS=OFF
    if !ERRORLEVEL! NEQ 0 (
        popd
        echo CMake configure failed.
        exit /b 1
    )
    cmake --build build-!BUILD_ARCH! --config Release
    if !ERRORLEVEL! NEQ 0 (
        popd
        echo zlib-ng build failed.
        exit /b 1
    )
    popd
) else (
    echo Using cached zlib-ng for !BUILD_ARCH!...
)

:: Handle test build (flat goto structure - see note above :submodule_ok)
if not defined IS_TEST goto :build_app

:: Verify zlib-ng library exists before attempting test build
if not exist "%ZLIBNG_LIB%" (
    echo ERROR: zlib-ng library not found at %ZLIBNG_LIB%
    echo Run 'build.bat' first to build zlib-ng, then run 'build.bat test'
    exit /b 1
)
echo Building Test Executable...
set TEST_FLAGS=/std:c++20 /EHsc /W4 /I include /I lib\zlib-ng /I lib\zlib-ng\build-!BUILD_ARCH! /DGITSMARTER_TEST_BUILD
set TEST_FLAGS=!TEST_FLAGS! /Od /Zi /Fe:GitSmarterTest.exe
set TEST_LIBS=user32.lib shlwapi.lib shell32.lib ole32.lib winhttp.lib advapi32.lib bcrypt.lib

cl !TEST_FLAGS! test\test_main.cpp %ZLIBNG_LIB% /link !TEST_LIBS!
if !ERRORLEVEL! NEQ 0 (
    echo Test build failed.
    exit /b 1
)
if exist test_main.obj del test_main.obj
if defined TEST_BUILD_ONLY (
    echo Build successful: GitSmarterTest.exe
    exit /b 0
)

:: Regenerate test_repo fixture if missing (generated locally, not committed)
if exist test\fixtures\test_repo\.git\HEAD goto :fixture_ok
echo Test fixture test\fixtures\test_repo missing - regenerating...
where git >nul 2>nul
if !ERRORLEVEL! NEQ 0 (
    echo ERROR: git not found on PATH - cannot regenerate test fixture.
    echo Install Git for Windows, then run: test\fixtures\regen_test_repo.bat
    exit /b 1
)
pushd .
call test\fixtures\regen_test_repo.bat
popd
if not exist test\fixtures\test_repo\.git\HEAD (
    echo ERROR: Test fixture regeneration failed.
    echo Run manually: test\fixtures\regen_test_repo.bat
    exit /b 1
)
:fixture_ok

echo.
echo Running tests...
echo.
.\GitSmarterTest.exe %2 %3 %4
exit /b !ERRORLEVEL!

:build_app

:: Build GitSmarter (always rebuilt)
echo Building GitSmarter...
cl %FLAGS% %SOURCES% %ZLIBNG_LIB% /link %LIBS%

if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b 1
)
if exist main.obj del main.obj
echo Build successful: !EXE_NAME!
exit /b 0

:: Delete all build artifacts and zlib-ng build directories.
:: Called via `call :clean` from both the clean and rebuild paths.
:clean
if exist build rmdir /s /q build
if exist build-x64 rmdir /s /q build-x64
if exist build-arm64 rmdir /s /q build-arm64
if exist lib\zlib-ng\build rmdir /s /q lib\zlib-ng\build
if exist lib\zlib-ng\build-x64 rmdir /s /q lib\zlib-ng\build-x64
if exist lib\zlib-ng\build-arm64 rmdir /s /q lib\zlib-ng\build-arm64
if exist main.obj del main.obj
if exist GitSmarter.exe del GitSmarter.exe
if exist GitSmarterDebug.exe del GitSmarterDebug.exe
if exist GitSmarterTest.exe del GitSmarterTest.exe
echo Cleaned.
exit /b 0
