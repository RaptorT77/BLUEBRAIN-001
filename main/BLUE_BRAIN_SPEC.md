# BLUE BRAIN SYSTEM SPECIFICATION (v1.0)
> Single Source of Truth for Blue Brain Industrial Node (Phase 1).
> Target Hardware: Seeed Studio XIAO ESP32-S3.

## 1. HARDWARE ABSTRACTION LAYER (HAL)
The firmware must strictly adhere to this pin mapping based on Seeed Studio XIAO ESP32-S3 Pinout.

| Component       | Signal | Pin Name | GPIO Number | Notes |
|-----------------|--------|----------|-------------|-------|
| **I2C Bus** | SDA    | D4       | GPIO 6      | MPU6050 Data (Default I2C) |
|                 | SCL    | D5       | GPIO 7      | MPU6050 Clock (Default I2C) |
| **SPI Display** | MOSI   | D10      | GPIO 10     | ST7789 Data |
|                 | SCLK   | D8       | GPIO 8      | ST7789 Clock |
|                 | CS     | D0       | GPIO 2      | Chip Select (Physically D0) |
|                 | DC     | D1       | GPIO 3      | Data/Command (Physically D1) |
|                 | RST    | D2       | GPIO 4      | Reset (Physically D2) |
| **1-Wire** | DATA   | D3       | GPIO 5      | DS18B20 Temp (Physically D3) |
| **UART Debug** | TX     | D6       | GPIO 43     | Serial Log Output |
|                 | RX     | D7       | GPIO 44     | Serial Input |
| **System** | BOOT   | Button   | GPIO 0      | Internal Button (Not on header). |

## 2. FIRMWARE STATE MACHINE (LIFECYCLE)
The system operates in a cyclic "Wake-Measure-Sleep" pattern to maximize battery life (3-5 years target).

### 2.1. States
1. **STATE_BOOT (Entry Point)**
   - Init NVS, SPIFFS, and Power Management.
   - Check Wakeup Cause:
     - If `DEEP_SLEEP_WAKEUP`: Goto **STATE_MEASURE**.
     - If `POWERON_RESET`: Check GPIO 0 (BOOT Button).
       - If pressed > 3 seconds: Goto **STATE_CONFIG**.
       - Else: Goto **STATE_MEASURE**.

2. **STATE_CONFIG (Maintenance Mode)**
   - **Display:** Show "CONFIG MODE - AP: BlueBrain_Setup".
   - **WiFi:** Start SoftAP + DNS Server (Captive Portal).
   - **Web Server:** Serve `index.html` for parameter setup.
   - **Exit:** User presses "Save & Reboot" -> Restart System.
   - **Timeout:** If no activity for 5 mins -> Goto **STATE_SLEEP**.

3. **STATE_MEASURE (Acquisition)**
   - **Display:** Show "MEASURING...".
   - **Sensors:** Power ON MPU6050 and DS18B20.
   - **Action:**
     - Acquire 4096 samples from MPU6050 (Accel Z-Axis) at configured sample rate.
     - Read Temperature from DS18B20.
   - Goto **STATE_PROCESS**.

4. **STATE_PROCESS (Edge AI)**
   - **Display:** Show "ANALYZING...".
   - **DSP:** Apply Hann Window -> Execute FFT -> Calculate Magnitude.
   - **Features:** Compute RMS, Peak, Kurtosis.
   - **AI:** Run TFLite Inference (Anomaly Detection).
   - Goto **STATE_TRANSMIT**.

5. **STATE_TRANSMIT (Connectivity)**
   - **Display:** Show "SENDING...".
   - **Protocol:** Try ESP-NOW (Primary) to Gateway.
   - **Retry Logic:** 3 attempts with exponential backoff.
   - **Fallback:** If ESP-NOW fails, write `bb_packet_t` to SPIFFS (Backlog).
   - **Hour Meter:** Increment global runtime counter in NVS.
   - Goto **STATE_SLEEP**.

6. **STATE_SLEEP (Power Save)**
   - **Display:** Turn OFF (or Low Power Mode).
   - **Sensors:** Power DOWN.
   - **Action:** Configure RTC Timer for next wakeup (Default: 5 mins).
   - **Deep Sleep:** Enter `esp_deep_sleep_start()`.

## 3. DATA STRUCTURES

### 3.1. ESP-NOW Payload (Binary)
*Constraint: Strictly packed structure, Little Endian.*
```c
typedef struct __attribute__((packed)) {
    uint8_t magic;           // 0xBB (Header)
    uint8_t device_mac[6];   // Sender MAC Address
    uint32_t packet_id;      // Incremental counter
    uint32_t timestamp;      // Relative RTC time
    float vib_rms;           // RMS Velocity (mm/s)
    float vib_peak;          // Peak Acceleration (g)
    float temp_c;            // Motor Temperature
    uint8_t battery_pct;     // Battery %
    uint8_t ai_status;       // 0=OK, 1=WARNING, 2=ALARM
    uint16_t checksum;       // CRC16
} bb_packet_t;