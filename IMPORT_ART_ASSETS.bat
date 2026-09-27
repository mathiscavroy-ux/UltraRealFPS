@echo off
setlocal EnableExtensions
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0UltraRealFPS.uproject"
set "EDITOR_CMD=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "SCRIPT=%~dp0Tools\import_art_assets.py"
set "SENTINEL=%~dp0Content\Environment\Industrial\SM_Container20_A.uasset"
set "NOPAUSE=0"
if /I "%~1"=="/nopause" set "NOPAUSE=1"

echo ==========================================================
echo   UltraRealFPS - Import du kit Art Foundation
echo ==========================================================

if not exist "%EDITOR_CMD%" (
    echo [ERREUR] UnrealEditor-Cmd.exe introuvable:
    echo %EDITOR_CMD%
    goto :fail
)

if not exist "%SCRIPT%" (
    echo [ERREUR] Script d'import introuvable:
    echo %SCRIPT%
    goto :fail
)

if not exist "%~dp0ArtSource\Industrial\SM_Container20_A.glb" (
    echo [ERREUR] Les modeles source ArtSource\Industrial sont absents.
    goto :fail
)

echo [ART] Import automatique GLB vers /Game/Environment/Industrial...
"%EDITOR_CMD%" "%PROJECT%" -run=pythonscript -script="%SCRIPT%" -unattended -nop4 -nosplash -NoSound
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
    echo [ERREUR] L'import Unreal a retourne le code %RESULT%.
    goto :fail
)

if not exist "%SENTINEL%" (
    echo [ERREUR] L'import s'est termine sans creer:
    echo %SENTINEL%
    goto :fail
)

echo.
echo [OK] Kit Art Foundation importe dans Content\Environment\Industrial.
if "%NOPAUSE%"=="0" pause
exit /b 0

:fail
echo.
echo L'import des assets a echoue. Le projet C++ reste intact.
if "%NOPAUSE%"=="0" pause
exit /b 1
