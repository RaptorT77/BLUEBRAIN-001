@echo off
REM ============================================
REM Blue Brain Firmware - Build Script
REM ============================================

echo.
echo ========================================
echo   BLUE BRAIN FIRMWARE - COMPILACION
echo ========================================
echo.

cd /d "%~dp0"

echo [1/3] Configurando entorno ESP-IDF...
call C:\Espressif\frameworks\esp-idf-v5.5.2\export.bat

if %errorlevel% neq 0 (
    echo.
    echo ERROR: No se pudo configurar el entorno ESP-IDF
    echo.
    echo SOLUCION: Abre "ESP-IDF 5.5 CMD" desde el menu inicio
    echo y ejecuta manualmente:
    echo.
    echo    cd c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware
    echo    idf.py build
    echo.
    pause
    exit /b 1
)

echo.
echo [2/3] Iniciando compilacion...
echo.

idf.py build

if %errorlevel% neq 0 (
    echo.
    echo ERROR: La compilacion fallo
    echo Revisa los errores arriba
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo   COMPILACION EXITOSA!
echo ========================================
echo.
echo Para flashear el firmware:
echo    idf.py flash monitor
echo.
echo O ejecuta: flash.bat
echo.
pause
