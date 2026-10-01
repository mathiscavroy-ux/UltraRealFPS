@echo off
setlocal EnableExtensions
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0UltraRealFPS.uproject"
set "EDITOR_CMD=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "SCRIPT=%~dp0Tools\build_environment_materials.py"
set "SENTINEL=%~dp0Content\UltraRealFPS\Art\Materials\Environment\Industrial\M_Master_IndustrialSurface.uasset"
set "SUCCESS_MARKER=%~dp0Saved\EnvironmentMaterials.ok"
set "NOPAUSE=0"
if /I "%~1"=="/nopause" set "NOPAUSE=1"

echo ==========================================================
echo   UltraRealFPS - Materiaux environnement (master material)
echo ==========================================================

if not exist "%EDITOR_CMD%" (
    echo [ERREUR] UnrealEditor-Cmd.exe introuvable:
    echo %EDITOR_CMD%
    goto :fail
)

if not exist "%SCRIPT%" (
    echo [ERREUR] Script introuvable:
    echo %SCRIPT%
    goto :fail
)

if exist "%SUCCESS_MARKER%" del /q "%SUCCESS_MARKER%" >nul 2>nul

echo [MAT] Generation de M_Master_IndustrialSurface...
"%EDITOR_CMD%" "%PROJECT%" -run=pythonscript -script="%SCRIPT%" -unattended -nop4 -nosplash -NoSound
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo [ERREUR] Unreal a retourne le code %RESULT%.
    goto :fail
)

if not exist "%SENTINEL%" (
    echo [ERREUR] Le script s'est termine sans creer:
    echo %SENTINEL%
    goto :fail
)

if not exist "%~dp0Saved" mkdir "%~dp0Saved" >nul 2>nul
> "%SUCCESS_MARKER%" echo OK

echo.
echo [OK] Master material cree. Le jeu l'utilise automatiquement au prochain lancement.
if "%NOPAUSE%"=="0" pause
exit /b 0

:fail
echo.
echo Les materiaux n'ont pas ete generes. Le jeu garde sa palette de secours.
echo Pour revenir a la palette de secours plus tard: supprimer Content\UltraRealFPS\Art\Materials.
if "%NOPAUSE%"=="0" pause
exit /b 1
