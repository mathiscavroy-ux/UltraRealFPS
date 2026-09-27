@echo off
setlocal EnableExtensions

set "REPO_URL=https://github.com/mathiscavroy-ux/UltraRealFPS.git"
REM %~dp0 finit par un antislash. On normalise le dossier pour eviter
REM qu'un chemin cite termine par \ et perturbe l'analyse de Robocopy.
for %%I in ("%~dp0.") do set "SOURCE_DIR=%%~fI"
set "PUBLISH_DIR=%TEMP%\UltraRealFPS_GitPublish"
set "COMMIT_MESSAGE=Publish tested UltraRealFPS build"
if not "%~1"=="" set "COMMIT_MESSAGE=%~1"

echo ==========================================================
echo   UltraRealFPS - publication GitHub de la baseline
echo ==========================================================
echo Source : %SOURCE_DIR%
echo Depot  : %REPO_URL%
echo.

where git >nul 2>nul
if errorlevel 1 (
    echo [ERREUR] Git n'est pas installe ou n'est pas dans le PATH.
    echo Installe Git for Windows ou GitHub Desktop, puis relance ce fichier.
    pause
    exit /b 1
)

if exist "%PUBLISH_DIR%" rmdir /s /q "%PUBLISH_DIR%"

echo [1/5] Clone propre du depot...
git clone "%REPO_URL%" "%PUBLISH_DIR%"
if errorlevel 1 goto :fail

echo [2/5] Copie des fichiers utiles du projet...
robocopy "%SOURCE_DIR%" "%PUBLISH_DIR%" *.* /E /R:1 /W:1 /NFL /NDL /NJH /NJS ^
    /XD ".git" "Binaries" "Intermediate" "Saved" "DerivedDataCache" ".vs" ^
    /XF "BUILD_ERRORS.txt" "*.sln" "*.VC.db" "*.VC.opendb" "*.suo" "*.user" "*.pdb" "*.obj" "*.log" "*.tmp"
set "ROBOCODE=%ERRORLEVEL%"
if %ROBOCODE% GEQ 8 (
    echo [ERREUR] Robocopy a retourne le code %ROBOCODE%.
    goto :fail
)

echo [OK] Copie terminee. Robocopy code %ROBOCODE%.

pushd "%PUBLISH_DIR%"
if errorlevel 1 goto :fail

REM Configure une identite locale uniquement si Git n'en a aucune.
git config user.name >nul 2>nul
if errorlevel 1 git config user.name "mathiscavroy-ux"
git config user.email >nul 2>nul
if errorlevel 1 git config user.email "mathiscavroy-ux@users.noreply.github.com"

git lfs version >nul 2>nul
if errorlevel 1 (
    echo [INFO] Git LFS n'est pas installe.
    echo        Le code source peut etre publie tant qu'aucun gros asset LFS n'est ajoute.
    echo        Avant les vrais .uasset/.umap/audio/textures, installe Git LFS.
) else (
    git lfs install --local >nul 2>nul
)

echo [3/5] Preparation du commit...
git add -A
if errorlevel 1 goto :gitfail

REM git diff --cached --quiet : 0 = aucun changement, 1 = changements presents.
git diff --cached --quiet
set "DIFFCODE=%ERRORLEVEL%"
if "%DIFFCODE%"=="0" (
    echo Aucun changement a committer.
) else if "%DIFFCODE%"=="1" (
    git commit -m "%COMMIT_MESSAGE%"
    if errorlevel 1 goto :gitfail
) else (
    echo [ERREUR] Impossible de verifier les changements Git. Code %DIFFCODE%.
    goto :gitfail
)

echo [4/5] Push vers main...
git push origin HEAD:main
if errorlevel 1 goto :gitfail

echo [5/5] Verification terminee.
popd
rmdir /s /q "%PUBLISH_DIR%" >nul 2>nul

echo.
echo ==========================================================
echo   PUBLICATION GITHUB REUSSIE
echo ==========================================================
echo Depot : mathiscavroy-ux/UltraRealFPS
echo Le dossier de jeu original n'a pas ete transforme ni modifie par Git.
echo.
pause
exit /b 0

:gitfail
popd
:fail
echo.
echo ==========================================================
echo   PUBLICATION GITHUB ECHOUEE
echo ==========================================================
echo Aucun fichier du projet original n'a ete supprime.
echo Si le dossier temporaire existe, il peut etre efface sans risque :
echo %PUBLISH_DIR%
echo.
echo Envoie une capture de cette fenetre dans le chat.
echo.
pause
exit /b 1
