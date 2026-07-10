@echo off
cd /d %~dp0

if exist test_repo rmdir /s /q test_repo
mkdir test_repo
cd test_repo

git init --quiet --initial-branch=master
git config user.name "Test User"
git config user.email "test@example.com"
git remote add origin https://github.com/test/repo.git

echo Hello World> file1.txt
git add file1.txt
git commit -m "Initial commit" --quiet

echo Second file content> file2.txt
git add file2.txt
git commit -m "Add second file" --quiet

echo Modified content> file1.txt
git add file1.txt
git commit -m "Modify first file" --quiet

mkdir subdir
echo Subdir file> subdir\file3.txt
git add subdir\file3.txt
git commit -m "Add file in subdirectory" --quiet

echo Executable script> script.sh
git add script.sh
git update-index --chmod=+x script.sh
git commit -m "Add executable file" --quiet

git checkout -b feature --quiet
echo Feature content> feature.txt
git add feature.txt
git commit -m "Add feature file" --quiet

git checkout master --quiet
git merge feature --no-edit --quiet 2>nul

for /f %%i in ('git rev-parse master~5') do set COMMIT1=%%i
for /f %%i in ('git rev-parse master~4') do set COMMIT2=%%i
for /f %%i in ('git rev-parse master~3') do set COMMIT3=%%i
for /f %%i in ('git rev-parse master~2') do set COMMIT4=%%i
for /f %%i in ('git rev-parse master~1') do set COMMIT5=%%i
for /f %%i in ('git rev-parse feature') do set FEATURE_COMMIT=%%i
for /f %%i in ('git rev-parse master') do set MERGE_COMMIT=%%i

echo # Test repository known SHAs> known_shas.txt
echo COMMIT1=%COMMIT1%>> known_shas.txt
echo COMMIT2=%COMMIT2%>> known_shas.txt
echo COMMIT3=%COMMIT3%>> known_shas.txt
echo COMMIT4=%COMMIT4%>> known_shas.txt
echo COMMIT5=%COMMIT5%>> known_shas.txt
echo FEATURE_COMMIT=%FEATURE_COMMIT%>> known_shas.txt
echo MERGE_COMMIT=%MERGE_COMMIT%>> known_shas.txt
echo # MASTER_SHA is same as MERGE_COMMIT - used by test_network.cpp>> known_shas.txt
echo MASTER_SHA=%MERGE_COMMIT%>> known_shas.txt

REM Extract blob SHAs from current index
for /f "tokens=2" %%i in ('git ls-files -s feature.txt') do set BLOB_FEATURE=%%i
for /f "tokens=2" %%i in ('git ls-files -s file1.txt') do set BLOB_FILE1=%%i
for /f "tokens=2" %%i in ('git ls-files -s file2.txt') do set BLOB_FILE2=%%i
for /f "tokens=2" %%i in ('git ls-files -s script.sh') do set BLOB_SCRIPT=%%i
for /f "tokens=2" %%i in ('git ls-files -s subdir/file3.txt') do set BLOB_SUBDIR_FILE3=%%i

REM Get the initial file1.txt blob (from initial commit tree)
for /f "tokens=3" %%i in ('git ls-tree %COMMIT1% file1.txt') do set BLOB_FILE1_INITIAL=%%i

echo # Blob SHAs for indexed files> blob_shas.txt
echo BLOB_FEATURE=%BLOB_FEATURE%>> blob_shas.txt
echo BLOB_FILE1=%BLOB_FILE1%>> blob_shas.txt
echo BLOB_FILE2=%BLOB_FILE2%>> blob_shas.txt
echo BLOB_SCRIPT=%BLOB_SCRIPT%>> blob_shas.txt
echo BLOB_SUBDIR_FILE3=%BLOB_SUBDIR_FILE3%>> blob_shas.txt
echo BLOB_FILE1_INITIAL=%BLOB_FILE1_INITIAL%>> blob_shas.txt

REM Extract tree SHAs
for /f "tokens=2" %%i in ('git cat-file -p %COMMIT1% ^| findstr "^tree"') do set TREE_INITIAL=%%i
for /f "tokens=2" %%i in ('git cat-file -p %COMMIT3% ^| findstr "^tree"') do set TREE_MODIFY=%%i
for /f "tokens=2" %%i in ('git cat-file -p master ^| findstr "^tree"') do set TREE_HEAD=%%i
for /f "tokens=3" %%i in ('git ls-tree master subdir') do set TREE_SUBDIR=%%i

echo # Tree SHAs for known commits> tree_shas.txt
echo TREE_INITIAL=%TREE_INITIAL%>> tree_shas.txt
echo TREE_MODIFY=%TREE_MODIFY%>> tree_shas.txt
echo TREE_HEAD=%TREE_HEAD%>> tree_shas.txt
echo TREE_SUBDIR=%TREE_SUBDIR%>> tree_shas.txt

cd ..
echo Test repository regenerated.
echo Generated: known_shas.txt, blob_shas.txt, tree_shas.txt
