$ErrorActionPreference = 'SilentlyContinue'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$SourceLog = Join-Path $env:LOCALAPPDATA 'UnrealBuildTool\Log.txt'
$Output = Join-Path $Root 'BUILD_ERRORS.txt'

if (-not (Test-Path -LiteralPath $SourceLog)) {
    @(
        'UltraRealFPS - aucune copie du log UnrealBuildTool trouvee.',
        "Chemin attendu: $SourceLog"
    ) | Set-Content -LiteralPath $Output -Encoding UTF8
    exit 0
}

$Lines = Get-Content -LiteralPath $SourceLog
$Selected = New-Object System.Collections.Generic.List[string]
$Selected.Add('UltraRealFPS - extrait automatique des erreurs UnrealBuildTool')
$Selected.Add(('Log source: {0}' -f $SourceLog))
$Selected.Add('')

$MatchIndices = New-Object System.Collections.Generic.HashSet[int]
for ($i = 0; $i -lt $Lines.Count; $i++) {
    if ($Lines[$i] -match '(?i)(fatal error|\berror\s+C\d+|\berror\s*[:\[]|OtherCompilationError|Unhandled exception)') {
        for ($j = [Math]::Max(0, $i - 2); $j -le [Math]::Min($Lines.Count - 1, $i + 5); $j++) {
            [void]$MatchIndices.Add($j)
        }
    }
}

if ($MatchIndices.Count -eq 0) {
    $Selected.Add('Aucune ligne error evidente detectee. Dernieres lignes du log:')
    $Selected.Add('')
    $Start = [Math]::Max(0, $Lines.Count - 45)
    for ($i = $Start; $i -lt $Lines.Count; $i++) {
        $Selected.Add($Lines[$i])
    }
}
else {
    $Ordered = $MatchIndices | Sort-Object
    $Previous = -2
    foreach ($Index in $Ordered) {
        if ($Index -gt ($Previous + 1)) {
            $Selected.Add('---')
        }
        $Selected.Add($Lines[$Index])
        $Previous = $Index
    }
}

$Selected | Set-Content -LiteralPath $Output -Encoding UTF8
Write-Host "Extrait d'erreurs cree: $Output" -ForegroundColor Yellow
exit 0
