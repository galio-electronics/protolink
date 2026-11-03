$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$envPath = Join-Path $repoRoot '.venv'
$pythonPath = Join-Path $envPath 'Scripts\python.exe'
& py -3 -m venv --without-pip $envPath
if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear .venv. Instalar Python 3 con launcher py.' }
& py -3 -m pip --python $pythonPath install -r (Join-Path $PSScriptRoot 'requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'No se pudo instalar pyserial. Revisar pip/conexion.' }
Write-Host "Entorno listo: $pythonPath"
