@echo off
setlocal EnableExtensions

set "REPO_URL=https://github.com/mathiscavroy-ux/UltraRealFPS.git"
set "TARGET_BRANCH=claude/upbeat-knuth-kuzsu8"
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
echo Branche: %TARGET_BRANCH%
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
git clone --branch "%TARGET_BRANCH%" --single-branch "%REPO_URL%" "%PUBLISH_DIR%"
if errorlevel 1 goto :fail

REM Garde-fou : Robocopy remplace les fichiers modifies. Un dossier local plus ancien
REM que la branche ecraserait donc ses corrections. Tout fichier suivi sur la branche
REM doit exister localement avant la copie.
echo [2/5] Verification du dossier local puis copie des fichiers utiles...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$Missing = @(git -C $env:PUBLISH_DIR ls-files | Where-Object { -not (Test-Path -LiteralPath (Join-Path $env:SOURCE_DIR $_)) }); if ($Missing.Count -gt 0) { $Missing | Select-Object -First 12 | ForEach-Object { Write-Host ('  manquant : ' + $_) }; exit 1 }; exit 0"
if errorlevel 1 (
    echo [ERREUR] Ce dossier est plus ancien que la branche %TARGET_BRANCH%.
    echo Les fichiers ci-dessus existent sur GitHub mais pas ici : publier maintenant
    echo remplacerait les corrections de la branche par d'anciens fichiers.
    echo Telecharge le ZIP de la branche, copie son contenu sur ce dossier, puis relance.
    goto :fail
)

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
    if exist "%SOURCE_DIR%\ArtSource\Industrial\SM_Container20_A.glb" (
        echo [ERREUR] Git LFS est obligatoire pour publier le nouveau kit 3D.
        echo Installe Git LFS puis relance PUBLISH_TO_GITHUB.bat.
        goto :gitfail
    )
    if exist "%SOURCE_DIR%\Content\Environment\Industrial\SM_Container20_A.uasset" (
        echo [ERREUR] Git LFS est obligatoire pour publier les assets Unreal.
        echo Installe Git LFS puis relance PUBLISH_TO_GITHUB.bat.
        goto :gitfail
    )
    echo [INFO] Git LFS non installe, mais aucun asset binaire du kit n'a ete detecte.
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

echo [4/5] Push vers %TARGET_BRANCH%...
git push origin HEAD:%TARGET_BRANCH%
if errorlevel 1 goto :gitfail

echo [5/5] Verification terminee.
popd
rmdir /s /q "%PUBLISH_DIR%" >nul 2>nul

echo.
echo ==========================================================
echo   PUBLICATION GITHUB REUSSIE
echo ==========================================================
echo Depot   : mathiscavroy-ux/UltraRealFPS
echo Branche : %TARGET_BRANCH%
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
