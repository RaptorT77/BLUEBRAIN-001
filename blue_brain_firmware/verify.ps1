# Verificación Pre-Compilación Blue Brain
# Ejecutar ANTES de compilar para asegurar que todo está correcto

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "VERIFICACIÓN PRE-COMPILACIÓN" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan

# 1. Verificar que estamos en el directorio correcto
$currentPath = Get-Location
if ($currentPath.Path -ne "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware") {
    Write-Host "⚠️  ADVERTENCIA: No estás en el directorio correcto" -ForegroundColor Yellow
    Write-Host "   Directorio actual: $($currentPath.Path)" -ForegroundColor Yellow
    Write-Host "   Directorio esperado: c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware" -ForegroundColor Yellow
    Write-Host "`nCambiando al directorio correcto..." -ForegroundColor Cyan
    Set-Location "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware"
}

Write-Host "✅ Directorio correcto" -ForegroundColor Green

# 2. Verificar archivos clave
Write-Host "`n📁 Verificando archivos clave..." -ForegroundColor Cyan

$files = @(
    "components\bb_dsp_ai\src\bb_dsp_ai.c",
    "components\bb_dsp_ai\include\bb_dsp_ai.h",
    "components\bb_dsp_ai\idf_component.yml",
    "components\bb_dsp_ai\CMakeLists.txt"
)

$allFilesOk = $true
foreach ($file in $files) {
    if (Test-Path $file) {
        Write-Host "  ✅ $file" -ForegroundColor Green
    }
    else {
        Write-Host "  ❌ $file - NO ENCONTRADO" -ForegroundColor Red
        $allFilesOk = $false
    }
}

if (-not $allFilesOk) {
    Write-Host "`n❌ Algunos archivos faltan. Verifica el proyecto." -ForegroundColor Red
    exit 1
}

# 3. Verificar contenido crítico de bb_dsp_ai.c
Write-Host "`n🔍 Verificando código bb_dsp_ai.c..." -ForegroundColor Cyan

$bbDspContent = Get-Content "components\bb_dsp_ai\src\bb_dsp_ai.c" -Raw

# Verificar inicialización de magnitude
if ($bbDspContent -match 'float \*magnitude = NULL;') {
    Write-Host "  ✅ Inicialización de magnitude correcta (NULL)" -ForegroundColor Green
}
else {
    Write-Host "  ⚠️  Inicialización de magnitude puede causar warning" -ForegroundColor Yellow
}

# Verificar include de esp_dsp.h
if ($bbDspContent -match '#include "esp_dsp.h"') {
    Write-Host "  ✅ Include de ESP-DSP correcto" -ForegroundColor Green
}
else {
    Write-Host "  ❌ Falta #include ""esp_dsp.h""" -ForegroundColor Red
    $allFilesOk = $false
}

# Verificar uso de dsps_dotprod_f32_ansi
if ($bbDspContent -match 'dsps_dotprod_f32_ansi') {
    Write-Host "  ✅ Función ESP-DSP dsps_dotprod_f32_ansi presente" -ForegroundColor Green
}
else {
    Write-Host "  ⚠️  No se encontró uso de dsps_dotprod_f32_ansi" -ForegroundColor Yellow
}

# 4. Verificar idf_component.yml
Write-Host "`n🔍 Verificando dependencias ESP-DSP..." -ForegroundColor Cyan

$ymlContent = Get-Content "components\bb_dsp_ai\idf_component.yml" -Raw

if ($ymlContent -match 'espressif/esp-dsp') {
    Write-Host "  ✅ Dependencia esp-dsp configurada" -ForegroundColor Green
}
else {
    Write-Host "  ❌ Falta dependencia esp-dsp en idf_component.yml" -ForegroundColor Red
    $allFilesOk = $false
}

# 5. Verificar CMakeLists.txt
Write-Host "`n🔍 Verificando CMakeLists.txt..." -ForegroundColor Cyan

$cmakeContent = Get-Content "components\bb_dsp_ai\CMakeLists.txt" -Raw

if ($cmakeContent -match 'REQUIRES.*esp-dsp') {
    Write-Host "  ✅ esp-dsp en REQUIRES de CMakeLists.txt" -ForegroundColor Green
}
else {
    Write-Host "  ❌ Falta esp-dsp en REQUIRES de CMakeLists.txt" -ForegroundColor Red
    $allFilesOk = $false
}

# 6. Resumen Final
Write-Host "`n==================================" -ForegroundColor Cyan
if ($allFilesOk) {
    Write-Host "✅ VERIFICACIÓN EXITOSA" -ForegroundColor Green
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host "`nEl proyecto está listo para compilar." -ForegroundColor Green
    Write-Host "`nEjecutar:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1" -ForegroundColor White
    Write-Host "o" -ForegroundColor Yellow
    Write-Host "  idf.py build (después de configurar entorno)" -ForegroundColor White
}
else {
    Write-Host "❌ VERIFICACIÓN FALLIDA" -ForegroundColor Red
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host "`nHay problemas que deben corregirse antes de compilar." -ForegroundColor Red
    exit 1
}
