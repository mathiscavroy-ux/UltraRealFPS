@echo off
setlocal
set "EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%~dp0UltraRealFPS.uproject"
if not exist "%EDITOR%" (
    echo UnrealEditor.exe introuvable.
    pause
    exit /b 1
)
start "" "%EDITOR%" "%PROJECT%"
