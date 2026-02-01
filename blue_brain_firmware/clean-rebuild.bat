@echo off
REM Script para limpiar ccache y recompilar Blue Brain

echo ================================
echo LIMPIANDO CACHE Y RECOMPILANDO
echo ================================
echo.

cd /d "c:\BLUE-BRAIN-001\BLUEBRAIN-001\blue_brain_firmware"

echo Paso 1: Limpiando ccache...
ccache -C
ccache -z

echo.
echo Paso 2: Eliminando build directory...
if exist build (
    rmdir /s /q build
    echo Build directory eliminado
)

if exist managed_components (
    echo Manteniendo managed_components (ESP-DSP ya descargado)
)

echo.
echo Paso 3: Recompilando...
echo (Esto tomara 2-3 minutos)
echo.

idf.py build

if errorlevel 1 (
    echo.
    echo ==================================
    echo ERROR EN LA COMPILACION
    echo ==================================
    pause
    exit /b 1
) else (
    echo.
    echo ==================================
    echo COMPILACION EXITOSA
    echo ==================================
    pause
)
