# 🧠 Blue Brain - Sensor Node (Fase 1)

**Hardware:** Seeed Studio XIAO ESP32-S3  
**Framework:** ESP-IDF v5.5.2  
**Arquitectura:** Dual-Core con FreeRTOS

---

## 📂 Estructura del Repositorio

```
BLUEBRAIN-001/
├── blue_brain_firmware/        ← 🎯 PROYECTO PRINCIPAL (compilar aquí)
│   ├── components/              # Componentes modulares
│   │   ├── bb_sensors/          # MPU6050 + DS18B20
│   │   ├── bb_dsp_ai/           # Procesamiento DSP con esp-dsp
│   │   ├── bb_connect/          # WiFi + MQTT
│   │   ├── bb_power/            # Gestión de energía
│   │   ├── bb_storage/          # SPIFFS
│   │   ├── bb_display/          # Display ST7789 (skeleton)
│   │   ├── bb_web_ui/           # Portal cautivo (skeleton)
│   │   └── bb_espnow/           # ESP-NOW (skeleton)
│   ├── main/                    # Orchestrator principal
│   ├── partitions.csv           # Tabla de particiones
│   └── sdkconfig.defaults       # Configuración del proyecto
├── examples/                    # Ejemplos y código de prueba
│   └── wifi_scanner/            # Ejemplo WiFi Scanner
├── datasheets/                  # PDFs de componentes
└── README.md                    # Este archivo
```

---

## 🚀 Inicio Rápido

### 1. Navegar al Proyecto CORRECTO

```powershell
cd blue_brain_firmware
```

### 2. Configurar Entorno ESP-IDF

```powershell
$env:IDF_PATH = 'c:\Espressif\frameworks\esp-idf-v5.5.2'
& 'C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1'
```

### 3. Compilar

```powershell
idf.py build
```

### 4. Flashear al ESP32-S3

```powershell
idf.py -p COM<X> flash monitor
```

---

## 🎯 Funcionalidades Implementadas

### ✅ Core Funcional (70%)
- **bb_sensors**: Adquisición MPU6050 (I2C) + DS18B20 (1-Wire) ✅
- **bb_dsp_ai**: Procesamiento DSP con esp-dsp (RMS, Peak, CF) ✅
- **bb_connect**: WiFi + MQTT + Telemetría JSON ✅
- **bb_power**: Deep Sleep (básico, falta ADC batería) 🟡
- **bb_storage**: SPIFFS montado (falta lógica app) 🟡

### 🔴 Pendientes
- **bb_display**: Driver ST7789 para display
- **bb_web_ui**: Portal cautivo para configuración WiFi
- **bb_espnow**: Comunicación mesh P2P

---

## 📊 Arquitectura Dual-Core

| Core | Tarea | Prioridad | Stack |
|------|-------|-----------|-------|
| **Core 0** | WiFi/MQTT/Comunicaciones | 3 | 4096 bytes |
| **Core 1** | DSP/AI/Análisis de Vibración | 5 | 8192 bytes |

**Comunicación:** Cola FreeRTOS (`xQueueTelemetry`) de 10 elementos

---

## 🔧 Configuración Hardware

**Pinout definido en `.gravityrules`:**

| Función | GPIO | Alias XIAO | Periférico |
|---------|------|------------|------------|
| I2C SDA | GPIO6 | D4 | MPU6050 |
| I2C SCL | GPIO7 | D5 | MPU6050 |
| SPI MOSI | GPIO10 | D10 | Display ST7789 |
| SPI CLK | GPIO8 | D8 | Display ST7789 |
| Display CS | GPIO2 | D0 | ST7789 |
| Display DC | GPIO3 | D1 | ST7789 |
| Display RST | GPIO4 | D2 | ST7789 |
| 1-Wire | GPIO5 | D3 | DS18B20 |

---

## 📚 Documentación

### Informes del Proyecto
- **Estado del Proyecto:** `status_report.md`
- **Arquitectura Completa:** Ver `.gemini/antigravity/brain/[conversation-id]/`

### Datasheets Incluidas
- XIAO ESP32-S3
- MPU6050 (Acelerómetro/Giroscopio)
- DS18B20 (Sensor de temperatura)
- ST7789 (Display TFT 240x240)

---

## 🛠️ Dependencias

### Librerías ESP-IDF
- **esp-dsp** v1.5.0+ (optimización DSP)
- WiFi, MQTT, NVS, SPIFFS (componentes ESP-IDF estándar)

### Descargas Automáticas
esp-dsp se descarga automáticamente durante la primera compilación via IDF Component Manager.

---

## 📝 Configuración WiFi/MQTT

Editar `blue_brain_firmware/sdkconfig.defaults`:

```
CONFIG_BB_WIFI_SSID="TuRedWiFi"
CONFIG_BB_WIFI_PASS="TuContraseña"
CONFIG_BB_MQTT_URI="mqtt://tu-broker:1883"
```

O usar `idf.py menuconfig` → Blue Brain Configuration

---

## 🚨 Troubleshooting

### Error: "Project not configured"
```powershell
cd blue_brain_firmware
idf.py reconfigure
```

### Error: "Port not found"
Verifica el puerto COM en Administrador de Dispositivos

### Error: "Failed to connect"
1. Mantén presionado BOOT
2. Presiona RESET
3. Suelta RESET, luego BOOT
4. Intenta flashear de nuevo

---

## 📌 Comandos Útiles

```powershell
# Limpiar build
idf.py fullclean

# Ver tamaños de componentes
idf.py size-components

# Configuración del proyecto
idf.py menuconfig

# Solo compilar (sin flashear)
idf.py build

# Solo monitorear
idf.py -p COM<X> monitor

# Flash + Monitor (combinado)
idf.py -p COM<X> flash monitor
```

---

## 🔗 Enlaces

- **GitHub:** [RaptorT77/BLUEBRAIN-001](https://github.com/RaptorT77/BLUEBRAIN-001)
- **ESP-IDF Docs:** [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/)
- **esp-dsp Docs:** [ESP-DSP Component](https://github.com/espressif/esp-dsp)

---

## 📄 Licencia

MIT License

---

**Última Actualización:** 31 de Enero, 2026
