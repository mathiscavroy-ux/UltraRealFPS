$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Errors = @()

function Add-CheckError([string]$Message) {
    $script:Errors += $Message
}

function Require-File([string]$RelativePath) {
    $Path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $Path)) {
        Add-CheckError "Fichier manquant: $RelativePath"
    }
    return $Path
}

$Project = Require-File 'UltraRealFPS.uproject'
$BuildCs = Require-File 'Source\UltraRealFPS\UltraRealFPS.Build.cs'
$GameTarget = Require-File 'Source\UltraRealFPS.Target.cs'
$EditorTarget = Require-File 'Source\UltraRealFPSEditor.Target.cs'
$EngineIni = Require-File 'Config\DefaultEngine.ini'
$InputIni = Require-File 'Config\DefaultInput.ini'
$AudioHeader = Require-File 'Source\UltraRealFPS\URFPSAudio.h'
$AudioSource = Require-File 'Source\UltraRealFPS\URFPSAudio.cpp'
$DoorHeader = Require-File 'Source\UltraRealFPS\URFPSDoor.h'
$DoorSource = Require-File 'Source\UltraRealFPS\URFPSDoor.cpp'
$ErrorCollector = Require-File 'COLLECT_BUILD_ERRORS.ps1'

if (Test-Path -LiteralPath $BuildCs) {
    $Text = Get-Content -LiteralPath $BuildCs -Raw
    if ($Text -notmatch '"PhysicsCore"') {
        Add-CheckError 'PhysicsCore absent de UltraRealFPS.Build.cs'
    }
}

foreach ($Target in @($GameTarget, $EditorTarget)) {
    if (Test-Path -LiteralPath $Target) {
        $Text = Get-Content -LiteralPath $Target -Raw
        if ($Text -notmatch 'BuildSettingsVersion\.V7') {
            Add-CheckError "BuildSettingsVersion.V7 absent: $Target"
        }
    }
}

if (Test-Path -LiteralPath $EngineIni) {
    $Text = Get-Content -LiteralPath $EngineIni -Raw
    foreach ($Surface in @('Concrete', 'Metal', 'Wood', 'Flesh')) {
        # PowerShell does not use C-style \" escaping. Build the literal text safely.
        $Needle = 'Name="{0}"' -f $Surface
        if ($Text -notmatch [regex]::Escape($Needle)) {
            Add-CheckError "Surface physique absente: $Surface"
        }
    }
}

if (Test-Path -LiteralPath $InputIni) {
    $Text = Get-Content -LiteralPath $InputIni -Raw
    if ($Text -notmatch 'ActionName="Zeroing"') {
        Add-CheckError 'Input Zeroing absent de DefaultInput.ini'
    }
}

$SourceRoot = Join-Path $Root 'Source'
if (Test-Path -LiteralPath $SourceRoot) {
    $SourceFiles = Get-ChildItem -LiteralPath $SourceRoot -Recurse -File | Where-Object {
        $_.Extension -eq '.h' -or $_.Extension -eq '.cpp'
    }

    $AllSource = ($SourceFiles | ForEach-Object {
        Get-Content -LiteralPath $_.FullName -Raw
    }) -join "`n"

    $Forbidden = @(
        @{ Pattern = 'Engine/SkyAtmosphere\.h'; Message = 'Ancien include invalide Engine/SkyAtmosphere.h detecte' },
        @{ Pattern = 'CrouchedHalfHeight\s*='; Message = 'Acces direct deprecie a CrouchedHalfHeight detecte' },
        @{ Pattern = 'BuildSettingsVersion\.V5'; Message = 'Ancien BuildSettingsVersion.V5 detecte' },
        @{ Pattern = 'ImpactPoint\.IsNearlyZero\(\)\s*\?[^;\n]*:[^;\n]*ImpactPoint'; Message = 'Operateur ternaire FVector/FVector_NetQuantize detecte autour de ImpactPoint' }
    )

    foreach ($Rule in $Forbidden) {
        if ($AllSource -match $Rule.Pattern) {
            Add-CheckError $Rule.Message
        }
    }

    $RequiredSourcePatterns = @(
        @{ Pattern = 'URFPSAudio::PlayGunshot'; Message = 'Couche audio gunshot absente' },
        @{ Pattern = 'AURFPSDoor::Interact'; Message = 'Interaction de porte absente' },
        @{ Pattern = 'UpdateFootsteps'; Message = 'Systeme de pas absent' },
        @{ Pattern = 'AlertFromNoise'; Message = 'Perception acoustique IA absente' }
    )
    foreach ($Rule in $RequiredSourcePatterns) {
        if ($AllSource -notmatch $Rule.Pattern) {
            Add-CheckError $Rule.Message
        }
    }
}

if ($Errors.Count -gt 0) {
    Write-Host ''
    Write-Host '==========================================================' -ForegroundColor Red
    Write-Host '  PRECHECK ECHEC' -ForegroundColor Red
    Write-Host '==========================================================' -ForegroundColor Red
    foreach ($Item in $Errors) {
        Write-Host " - $Item" -ForegroundColor Red
    }
    exit 1
}

Write-Host 'Precheck projet: OK' -ForegroundColor Green
exit 0
