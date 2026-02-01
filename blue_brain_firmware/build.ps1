# Blue Brain - Compilación usando CMD (evita problemas con PowerShell)
# Este script usa CMD.exe que es más compatible con ESP-IDF

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "BLUE BRAIN - COMPILACIÓN" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan

# Cambiar al directorio
Set-Location "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware"

Write-Host "`n🔨 Compilando con ESP-IDF..." -ForegroundColor Cyan
Write-Host "(Abriendo CMD con entorno ESP-IDF configurado)" -ForegroundColor Yellow

# Crear script temporal de CMD
$cmdScript = @"
@echo off
echo Configurando ESP-IDF...
call C:\Espressif\frameworks\esp-idf-v5.5.2\export.bat

if errorlevel 1 (
    echo ERROR: No se pudo configurar ESP-IDF
    pause
    exit /b 1
)

echo.
echo Compilando Blue Brain Firmware...
idf.py build

if errorlevel 1 (
    echo.
    echo ERROR EN LA COMPILACION
    pause
    exit /b 1
) else (
    echo.
    echo ================================
    echo COMPILACION EXITOSA
    echo ================================
    echo.
    echo Para flashear: idf.py -p COMX flash monitor
    pause
)
"@

# Guardar script temporal
$tempScript = "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware\build_temp.bat"
$cmdScript | Out-File -FilePath $tempScript -Encoding ASCII

Write-Host "`n✅ Script creado: build_temp.bat" -ForegroundColor Green
Write-Host "Ejecutando..." -ForegroundColor Cyan

# Ejecutar el script
& cmd.exe /c $tempScript

# Limpiar
Remove-Item $tempScript -ErrorAction SilentlyContinue
