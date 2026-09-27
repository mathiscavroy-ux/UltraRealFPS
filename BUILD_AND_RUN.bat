@echo off
setlocal
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0UltraRealFPS.uproject"
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"

echo ==========================================================
echo   UltraRealFPS - Build + Run Unreal Engine 5.8
echo ==========================================================
echo Project: %PROJECT%
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0VERIFY_PROJECT.ps1"
if errorlevel 1 (
    echo.
    echo Le precheck a detecte un probleme avant compilation.
    pause
    exit /b 1
)

if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo [ERREUR] Unreal Engine 5.8 introuvable dans:
    echo %UE_ROOT%
    echo.
    pause
    exit /b 1
)

call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" UltraRealFPSEditor Win64 Development -Project="%PROJECT%" -WaitMutex
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0COLLECT_BUILD_ERRORS.ps1"
    echo.
    echo ==========================================================
    echo   COMPILATION ECHOUEE - code %RESULT%
    echo ==========================================================
    echo Un extrait a ete cree dans BUILD_ERRORS.txt
    echo Envoie simplement ce fichier dans le chat.
    echo.
    pause
    exit /b %RESULT%
)

echo.
echo ==========================================================
echo   COMPILATION REUSSIE - LANCEMENT DE L'EDITEUR
 echo ==========================================================

if not exist "%EDITOR%" (
    echo UnrealEditor.exe introuvable.
    pause
    exit /b 1
)

start "" "%EDITOR%" "%PROJECT%"
exit /b 0
