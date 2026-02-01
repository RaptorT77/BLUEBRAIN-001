# Mapa de Componentes y Estatus: Blue Brain Firmware

**Fecha:** 28 de Enero, 2026
**Objetivo:** Relacionar la estructura del proyecto con la funcionalidad implementada.

## Resumen del Proyecto
El firmware está diseñado modularmente usando el framework ESP-IDF. Cada funcionalidad principal reside en su propio "componente", desacoplando la lógica y facilitando el mantenimiento.

## Tabla de Relación: Componentes vs. Estado

| Componente (Directorio) | Archivos Clave | Funcionalidad Principal | Estado de Implementación | Notas Técnicas |
| :--- | :--- | :--- | :--- | :--- |
| **`main`** | `main.c` | Orchestrator (Gestor Principal) | 🟢 **COMPLETO** | Gestiona el Dual-Core (C0: Comms, C1: DSP). Maneja colas de RTOS y memoria dinámica. |
| **`components/bb_sensors`** | `bb_sensors.c` | Adquisición de Datos | 🟢 **COMPLETO** | Integra MPU6050 (I2C) y DS18B20 (Bit-Bang). Optimizado para lectura en ráfaga (burst). |
| **`components/bb_dsp_ai`** | `bb_dsp_ai.c` | Procesamiento Digital (Edge AI) | 🟢 **COMPLETO** | Calcula RMS, Peak, P2P, y Crest Factor. Convierte raw data a unidades físicas (G). |
| **`components/bb_connect`** | `bb_connect.c` | Conectividad (WiFi + MQTT) | 🟢 **COMPLETO** | Maneja conexión WiFi robusta y cliente MQTT. Serializa datos a JSON para envío. |
| **`components/bb_storage`** | `bb_storage.c` | Almacenamiento Local | 🟡 **PARCIAL** | Inicialización de SPIFFS montada correctamente y funcional. Falta lógica de lectura/escritura de archivos específicos. |
| **`components/bb_power`** | `bb_power.c` | Gestión de Energía | 🔴 **PENDIENTE** | Estructura básica de Deep Sleep existe. **Falta:** Lectura real de voltaje por ADC (actualmente devuelve valor dummy). |
| **`components/bb_display`** | `bb_display.c` | Interfaz Visual (OLED/TFT) | 🔴 **PENDIENTE** | Solo skeleton. `bb_display_init` solo imprime log. Falta driver SPI y librería gráfica. |
| **`components/bb_web_ui`** | `bb_web_ui.c` | Portal Cautivo / Webserver | 🔴 **PENDIENTE** | Solo skeleton. Falta implementar servidor HTTP y páginas HTML embebidas. |
| **`components/bb_espnow`** | `bb_espnow.c` | Comunicación P2P | 🔴 **PENDIENTE** | Solo skeleton. Planeado para red mesh local o configuración rápida, pero vacío actualmente. |
| **Raíz** | `sdkconfig.defaults`<br>`partitions.csv` | Configuración del Sistema | 🟢 **COMPLETO** | Particiones personalizadas (3MB app) y CPU a 240MHz configurados correctamente. |

---

## Detalle de Funcionalidades Pendientes vs Componentes

Si deseas avanzar, estas son las relaciones directas entre lo que falta y dónde debe programarse:

1.  **"Quiero ver el estado de la batería"**
    *   📂 Trabajar en: `components/bb_power/src/bb_power.c`
    *   📝 Tarea: Implementar `adc_oneshot` para leer el pin de batería del XIAO.

2.  **"Quiero configurar el WiFi sin cambiar código"**
    *   📂 Trabajar en: `components/bb_web_ui/src/bb_web_ui.c`
    *   📝 Tarea: Crear un servidor DNS (Captive Portal) y endpoints HTTP para recibir SSID/Pass.

3.  **"Quiero ver datos en la pantallita"**
    *   📂 Trabajar en: `components/bb_display/src/bb_display.c`
    *   📝 Tarea: Portar o escribir driver para ST7789 sobre SPI.

4.  **"Quiero comunicar sensores entre sí sin WiFi"**
    *   📂 Trabajar en: `components/bb_espnow/src/bb_espnow.c`
    *   📝 Tarea: Inicializar stack ESP-NOW y definir estructura de payloads p2p.
