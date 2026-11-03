param(
    [Parameter(Position=0)][string]$Action = 'build',
    [Parameter(ValueFromRemainingArguments=$true)][string[]]$ExtraArgs
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$pythonPath = Join-Path $repoRoot '.venv\Scripts\python.exe'
if (!(Test-Path -LiteralPath $pythonPath)) {
    & (Join-Path $PSScriptRoot 'setup.ps1')
}
& $pythonPath -B (Join-Path $PSScriptRoot 'station_upload.py') $Action @ExtraArgs
exit $LASTEXITCODE
