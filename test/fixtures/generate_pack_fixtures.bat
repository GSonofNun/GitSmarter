@echo off
REM Generate pack file fixtures with known content for testing
REM Run from the test\fixtures directory
setlocal enabledelayedexpansion

set FIXTURE_DIR=%~dp0packs
echo Generating pack fixtures in %FIXTURE_DIR%

REM Clean up existing fixtures
if exist "%FIXTURE_DIR%" (
    echo Cleaning existing fixtures...
    rmdir /s /q "%FIXTURE_DIR%"
)
mkdir "%FIXTURE_DIR%"

REM Create temp repo
set TEMP_REPO=%FIXTURE_DIR%\temp_repo
mkdir "%TEMP_REPO%"
pushd "%TEMP_REPO%"

echo Initializing test repository...
git init
git config user.email "test@test.com"
git config user.name "Test User"

REM Create 3 commits with known content
echo file1 content> file1.txt
git add file1.txt
git commit -m "First commit"

echo file2 content> file2.txt
git add file2.txt
git commit -m "Second commit"

echo file3 content> file3.txt
git add file3.txt
git commit -m "Third commit"

REM Force pack creation with aggressive gc
echo Creating pack file...
git gc --aggressive

REM Copy pack files to fixture directory
echo Copying pack files...
for %%f in (.git\objects\pack\*.pack) do (
    copy "%%f" "%FIXTURE_DIR%\minimal.pack" >nul
    echo   Created minimal.pack
)
for %%f in (.git\objects\pack\*.idx) do (
    copy "%%f" "%FIXTURE_DIR%\minimal.idx" >nul
    echo   Created minimal.idx
)

REM Record known SHAs for test verification
echo Recording known SHAs...
git rev-parse HEAD > "%FIXTURE_DIR%\known_shas.txt"
git rev-parse HEAD~1 >> "%FIXTURE_DIR%\known_shas.txt"
git rev-parse HEAD~2 >> "%FIXTURE_DIR%\known_shas.txt"
git rev-parse HEAD^{tree} >> "%FIXTURE_DIR%\known_shas.txt"

echo.
echo Known SHAs:
type "%FIXTURE_DIR%\known_shas.txt"

REM Cleanup temp repo
popd
echo.
echo Cleaning up temp repository...
rmdir /s /q "%TEMP_REPO%"

echo.
echo Pack fixtures generated successfully in %FIXTURE_DIR%
echo.
dir "%FIXTURE_DIR%"
