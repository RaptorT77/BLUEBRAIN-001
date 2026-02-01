# Blue Brain - Configuración de Hardware

**Proyecto:** Blue Brain Sensor Node (Phase 1)  
**Placa:** Seeed Studio XIAO ESP32-S3  
**Versión Firmware:** v1.0  
**Última Actualización:** 31 de Enero, 2026

---

## 🔧 Placa Principal

### Seeed Studio XIAO ESP32-S3

**Especificaciones:**
- MCU: ESP32-S3 (Dual-core Xtensa LX7 @ 240 MHz)
- Flash: 8 MB
- RAM: 512 KB SRAM
- WiFi: 2.4 GHz 802.11 b/g/n
- Bluetooth: BLE 5.0
- USB: USB-C (Serial + JTAG)
- Dimensiones: 21mm x 17.5mm

**Características Importantes:**
- ⚠️ Los pines D4/D5 **NO son GPIO 4/5**
- ⚠️ Consultar siempre el pinout oficial
- ✅ Soporta Deep Sleep
- ✅ Tiene cargador de batería LiPo integrado

---

## 📌 Pinout XIAO ESP32-S3

### Mapeo de Pines Usado en Este Proyecto

| Pin Label | GPIO Real | Función en Blue Brain | Sensor/Periférico |
|-----------|-----------|----------------------|-------------------|
| **D0**    | GPIO 1    | SPI CS               | ICM-42688 (CS)    |
| **D1**    | GPIO 2    | SPI CS               | Display ST7789 (CS)|
| **D2**    | GPIO 3    | SPI DC               | Display ST7789 (DC)|
| **D3**    | GPIO 4    | OneWire Data         | DS18B20 (Temp)    |
| **D4**    | GPIO 5    | I2C SDA              | MPU6050 (Accel)   |
| **D5**    | GPIO 6    | I2C SCL              | MPU6050 (Accel)   |
| **D8**    | GPIO 7    | IMU INT              | ICM-42688 (Int)   |
| **D9**    | GPIO 8    | SPI CLK              | Bus SPI Compartido|
| **D10**   | GPIO 9    | SPI MISO             | ICM-42688 (MISO)  |
| **(N/A)** | GPIO 10   | SPI MOSI             | Bus SPI Compartido|
| **D6**    | GPIO 43   | UART TX              | Debug             |
| **D7**    | GPIO 44   | UART RX              | Debug             |

**⚠️ CRÍTICO:** Siempre usar el GPIO real en el código, NO el label del pin.

---

## 🌡️ Sensores Conectados

### 1. MPU6050 - Acelerómetro/Giroscopio (I2C)
**Conexión I2C:**
```
VCC  → 3.3V
GND  → GND
SDA  → D4 (GPIO 5) + Pull-up 4.7kΩ
SCL  → D5 (GPIO 6) + Pull-up 4.7kΩ
```

### 2. ICM-42688-P - IMU Alta Precisión (SPI)
**Conexión SPI:**
```
VCC  → 3.3V
GND  → GND
MOSI → GPIO 10
MISO → D10 (GPIO 9)
SCLK → D9 (GPIO 8)
CS   → D0 (GPIO 1)
INT  → D8 (GPIO 7)
```
*Nota: Integrado en firmware pero inactivo por defecto.*

### 3. DS18B20 - Sensor de Temperatura (OneWire)

**Conexión I2C:**
```
VCC  → 3.3V
GND  → GND
SDA  → D4 (GPIO 5) + Pull-up 4.7kΩ a 3.3V
SCL  → D5 (GPIO 6) + Pull-up 4.7kΩ a 3.3V
AD0  → GND (dirección I2C 0x68)
```

**Configuración en Código:**
```c
#define BB_I2C_MASTER_SDA_IO    5      // D4
#define BB_I2C_MASTER_SCL_IO    6      // D5
#define BB_MPU6050_ADDR         0x68   // AD0 = GND
#define BB_I2C_MASTER_FREQ_HZ   400000 // 400 kHz
```

**Rango:** ±16G  
**Frecuencia de Muestreo:** ~1024 muestras por ráfaga  
**Uso:** Detección de vibraciones en maquinaria

---

### 2. DS18B20 - Sensor de Temperatura

**Conexión OneWire:**
```
VCC  → 3.3V
GND  → GND
DATA → D3 (GPIO 4) + Pull-up 4.7kΩ a 3.3V
```

**Configuración en Código:**
```c
#define BB_DS18B20_GPIO    4    // D3
```

**Rango:** -55°C a +125°C  
**Precisión:** ±0.5°C  
**Uso:** Monitoreo de temperatura ambiental

---

## ⚡ Resistencias Pull-up OBLIGATORIAS

**I2C Bus (MPU6050):**
- SDA (GPIO 5) → 4.7kΩ → 3.3V
- SCL (GPIO 6) → 4.7kΩ → 3.3V

**OneWire (DS18B20):**
- DATA (GPIO 4) → 4.7kΩ → 3.3V

⚠️ **SIN estas resistencias, los sensores NO funcionarán.**

---

## 4. Configuración de Red

### WiFi
- **SSID:** `Mega-2.4G-D9B1`
- **PASS:** `AcP6U9qbZg`

### MQTT
```
Broker: mqtt://62.72.6.94:1883
Topic: hcaa/plcs/bluebrain/telemetry
Autenticación: Anónima
QoS: 1
```

### Stack IoT Completo
```
Mosquitto:  62.72.6.94:1883
InfluxDB:   62.72.6.94:8086
Node-RED:   62.72.6.94:1880
Grafana:    62.72.6.94:3000 (presumiblemente)
n8n:        62.72.6.94:5678
```

---

## 📊 Formato de Telemetría (JSON)

```json
{
  "rms": 1.234,      // RMS de vibración (G)
  "peak": 2.456,     // Pico máximo (G)
  "p2p": 2.100,      // Peak-to-Peak (G)
  "crest": 1.99,     // Crest Factor (adimensional)
  "temp": 25.50      // Temperatura (°C)
}
```

**Frecuencia de Publicación:** ~6 segundos

---

## 🔌 Alimentación

**Opciones:**
1. USB-C (5V) → Regulador interno → 3.3V
2. Batería LiPo (3.7V - 4.2V) → Pin BAT
3. Pin 3.3V (solo si se usa fuente externa regulada)

**Consumo Estimado:**
- Activo (WiFi + Sensores): ~150-200 mA
- Deep Sleep: ~15-20 µA (no implementado aún)

---

## 📁 Archivos de Configuración Clave

### Hardware
```
blue_brain_firmware/components/bb_sensors/include/bb_sensors.h
  ├─ BB_I2C_MASTER_SDA_IO = 5
  ├─ BB_I2C_MASTER_SCL_IO = 6
  └─ BB_DS18B20_GPIO = 4

blue_brain_firmware/main/include/hardware_defs.h
  ├─ PIN_I2C_SDA = GPIO_NUM_5
  ├─ PIN_I2C_SCL = GPIO_NUM_6
  └─ PIN_ONE_WIRE = GPIO_NUM_4
```

### Conectividad
```
blue_brain_firmware/sdkconfig
  ├─ CONFIG_BB_WIFI_SSID="HUAWEI-102K7Z"
  ├─ CONFIG_BB_WIFI_PASS="4qv5BOonetony."
  └─ CONFIG_BB_MQTT_URI="mqtt://62.72.6.94:1883"

blue_brain_firmware/components/bb_connect/src/bb_connect.c
  └─ Topic: "hcaa/plcs/bluebrain/telemetry"
```

---

## 🛠️ Herramientas de Desarrollo

**IDE:** Visual Studio Code  
**Framework:** ESP-IDF v5.5.2  
**Python:** 3.11 (en ESP-IDF CMD)  
**Compilador:** Xtensa GCC 14.2.0  
**Puerto Serial:** COM7 (puede variar)  
**Baud Rate:** 115200

**Comandos Esenciales:**
```bash
# Compilar
idf.py build

# Flashear y monitorear
idf.py -p COM7 flash monitor

# Solo monitorear
idf.py -p COM7 monitor

# Salir del monitor
Ctrl + ]
```

---

## 🐛 Troubleshooting Común

### MPU6050 No Detectado
1. ✅ Verificar pull-ups de 4.7kΩ en SDA/SCL
2. ✅ Verificar VCC del sensor (debe medir 3.25-3.35V)
3. ✅ Verificar pines: D4=GPIO5(SDA), D5=GPIO6(SCL)
4. ✅ Verificar pin AD0 del MPU6050 está a GND

### Error "could not open COM7"
- Cerrar todos los monitores seriales (PuTTY, Arduino IDE, etc.)
- Desconectar y reconectar USB

### WiFi No Conecta
- Verificar que el router esté en 2.4 GHz (NO 5 GHz)
- Verificar credenciales en `sdkconfig`

### MQTT No Conecta
- Verificar WiFi conectado primero
- Verificar que 62.72.6.94:1883 sea accesible desde la red

---

## 📚 Documentación de Referencia

**Hardware:**
- [XIAO ESP32-S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [MPU6050 Datasheet](https://invensense.tdk.com/products/motion-tracking/6-axis/mpu-6050/)
- [DS18B20 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)

**Software:**
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/)
- [ESP-DSP Library](https://github.com/espressif/esp-dsp)

---

## 📝 Notas Importantes

1. **NUNCA** usar GPIO 43/44 para periféricos (son UART de debug)
2. **SIEMPRE** usar resistencias pull-up de 4.7kΩ en I2C y OneWire
3. **VERIFICAR** el pinout antes de modificar conexiones
4. **COMPILAR** desde ESP-IDF CMD (NO desde PowerShell normal)
5. **TOPIC MQTT** debe coincidir con configuración de Telegraf

---

**Este archivo es la FUENTE DE VERDAD para la configuración de hardware del proyecto Blue Brain.**

Si encuentras discrepancias entre este archivo y el código, **actualiza este archivo primero** y luego el código.

---

**Última verificación:** 31 de Enero, 2026 - Todos los sensores funcionales con pines correctos.
