# Blue Brain - Compilación Automática (Sin Interacción)
# Este script compila directamente sin pedir confirmación

$ErrorActionPreference = "Stop"

# Cambiar al directorio del proyecto
Set-Location "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware"

# Configurar ESP-IDF
$env:IDF_PATH = "c:\Espressif\frameworks\esp-idf-v5.5.2"

# Ejecutar export.ps1 en el mismo scope
. "$env:IDF_PATH\export.ps1"

# Compilar
Write-Host "`nCompilando Blue Brain Firmware..." -ForegroundColor Cyan
idf.py build

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✅ COMPILACIÓN EXITOSA" -ForegroundColor Green
}
else {
    Write-Host "`n❌ ERROR EN LA COMPILACIÓN" -ForegroundColor Red
    exit 1
}
